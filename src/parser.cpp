#include "yen/parser.h"
#include <stdexcept>

// ============================================================================
// Core infrastructure
// ============================================================================

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

const Token& Parser::peek() {
    return tokens[current];
}

const Token& Parser::advance() {
    if (current < tokens.size()) current++;
    return tokens[current - 1];
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    error(peek(), message);
}

void Parser::error(const Token& token, const std::string& message) {
    std::cerr << "[line " << token.line << "] Error";
    if (token.type == TokenType::Eof) {
        std::cerr << " at end";
    } else {
        std::cerr << " at '" << token.lexeme << "'";
    }
    std::cerr << ": " << message << std::endl;
    m_hadError = true;
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) return;
        switch (peek().type) {
            case TokenType::Class:
            case TokenType::Func:
            case TokenType::Let:
            case TokenType::Var:
            case TokenType::Const:
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Do:
            case TokenType::Go:
            case TokenType::Loop:
            case TokenType::Print:
            case TokenType::Return:
            case TokenType::Struct:
            case TokenType::Enum:
            case TokenType::Match:
            case TokenType::Switch:
            case TokenType::Import:
            case TokenType::Export:
            case TokenType::Extern:
            case TokenType::Defer:
            case TokenType::Assert:
            case TokenType::Try:
            case TokenType::Throw:
                return;
            default:
                break;
        }
        advance();
    }
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::Eof;
}

// ============================================================================
// Top-level parse
// ============================================================================

std::vector<std::unique_ptr<Statement>> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> statements;
    while (!isAtEnd()) {
        try {
            statements.push_back(statement());
        } catch (const std::runtime_error& e) {
            synchronize();
        }
    }
    return statements;
}

std::unique_ptr<Expression> Parser::parseExpression() {
    return expression();
}

// ============================================================================
// Statement dispatch
// ============================================================================

std::unique_ptr<Statement> Parser::statement() {
    if (match(TokenType::Struct)) return structStatement();
    if (match(TokenType::Extern)) return externBlock();
    if (match(TokenType::Print)) return printStatement();
    if (match(TokenType::Let)) return letStatement(false);   // immutable
    if (match(TokenType::Var)) return letStatement(true);    // mutable
    if (match(TokenType::Const)) return constStatement();
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::Func)) return functionStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::Do)) return doWhileStatement();
    if (match(TokenType::Loop)) return loopStatement();
    if (match(TokenType::Defer)) return deferStatement();
    if (match(TokenType::Assert)) return assertStatement();
    if (match(TokenType::Try)) return tryCatchStatement();
    if (match(TokenType::Throw)) return throwStatement();
    if (match(TokenType::Go)) {
        auto expr = expression();
        consume(TokenType::Semicolon, "Expected ';' after 'go' expression.");
        return std::make_unique<GoStmt>(std::move(expr));
    }
    if (match(TokenType::Break)) {
        consume(TokenType::Semicolon, "Expected ';' after 'break'.");
        return std::make_unique<BreakStmt>();
    }
    if (match(TokenType::Continue)) {
        consume(TokenType::Semicolon, "Expected ';' after 'continue'.");
        return std::make_unique<ContinueStmt>();
    }
    if (match(TokenType::Enum)) return enumStatement();
    if (match(TokenType::Match)) return matchStatement();
    if (match(TokenType::Switch)) return switchStatement();
    if (match(TokenType::Data)) {
        consume(TokenType::Class, "Expected 'class' after 'data'.");
        auto stmt = classStatement();
        if (auto cls = dynamic_cast<ClassStmt*>(stmt.get())) {
            cls->isDataClass = true;
        }
        return stmt;
    }
    if (match(TokenType::Sealed)) {
        consume(TokenType::Class, "Expected 'class' after 'sealed'.");
        auto stmt = classStatement();
        if (auto cls = dynamic_cast<ClassStmt*>(stmt.get())) {
            cls->isSealed = true;
        }
        return stmt;
    }
    if (match(TokenType::Class)) return classStatement();
    if (match(TokenType::Trait)) return traitStatement();
    if (match(TokenType::Impl)) return implStatement();
    if (match(TokenType::Repeat)) return repeatStatement();
    if (match(TokenType::Extend)) return extendStatement();
    // unless: desugar to if(!condition)
    if (match(TokenType::Unless)) {
        consume(TokenType::LParen, "Expected '(' after 'unless'.");
        auto condition = expression();
        consume(TokenType::RParen, "Expected ')' after unless condition.");
        auto body = statement();
        auto negated = std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(condition));
        return std::make_unique<IfStmt>(std::move(negated), std::move(body));
    }
    // until: desugar to while(!condition)
    if (match(TokenType::Until)) {
        consume(TokenType::LParen, "Expected '(' after 'until'.");
        auto condition = expression();
        consume(TokenType::RParen, "Expected ')' after until condition.");
        auto body = statement();
        auto negated = std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(condition));
        return std::make_unique<WhileStmt>(std::move(negated), std::move(body));
    }
    // guard: guard expr else { block } → if(!expr) { block }
    if (match(TokenType::Guard)) {
        auto condition = expression();
        consume(TokenType::Else, "Expected 'else' after guard condition.");
        consume(TokenType::LBrace, "Expected '{' after 'else' in guard.");
        auto elseBlock = blockStatement();
        auto negated = std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(condition));
        return std::make_unique<IfStmt>(std::move(negated), std::move(elseBlock));
    }
    if (match(TokenType::Import)) return importStatement();
    if (match(TokenType::Export)) return exportStatement();

    // Check for block statement vs map literal:
    // If we see '{', we need to determine if it's a map literal or a block.
    // A map literal starts with: { <expr> : ...
    // A block starts with: { <statement> ...
    // We detect map by checking: { (identifier|string|int|float) :
    // If it's a map, we fall through to expressionStatement which will parse it.
    // If it's a block, we handle it here.
    if (check(TokenType::LBrace)) {
        // Look ahead to see if this is a map literal
        bool isMap = false;
        if (current + 2 < tokens.size()) {
            TokenType firstInBrace = tokens[current + 1].type;
            TokenType secondInBrace = tokens[current + 2].type;
            if ((firstInBrace == TokenType::Identifier ||
                 firstInBrace == TokenType::String ||
                 firstInBrace == TokenType::Int ||
                 firstInBrace == TokenType::Float) &&
                secondInBrace == TokenType::Colon) {
                isMap = true;
            }
        }
        // Also check for empty map: {}
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::RBrace) {
            // Could be empty block or empty map - treat as empty block
            // (empty map would need explicit syntax if desired)
        }

        if (!isMap) {
            advance(); // consume '{'
            return blockStatement();
        }
        // If it's a map, fall through to expressionStatement
    }

    return expressionStatement();
}

// ============================================================================
// Simple statements
// ============================================================================

std::unique_ptr<Statement> Parser::printStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after print statement.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression();

    // Check for simple assignment: expr = value;
    if (match(TokenType::Assign)) {
        auto value = expression();
        consume(TokenType::Semicolon, "Expected ';' after assignment.");

        if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignStmt>(varExpr->name, std::move(value));
        } else if (auto getExpr = dynamic_cast<GetExpr*>(expr.get())) {
            return std::make_unique<SetStmt>(
                std::move(getExpr->object),
                std::make_unique<LiteralExpr>(Value(getExpr->name)),
                std::move(value));
        } else if (auto indexExpr = dynamic_cast<IndexExpr*>(expr.get())) {
            return std::make_unique<IndexAssignStmt>(
                std::move(indexExpr->listExpr),
                std::move(indexExpr->indexExpr),
                std::move(value));
        } else {
            error(previous(), "Invalid assignment target.");
            throw std::runtime_error("Invalid assignment target.");
        }
    }

    // Check for postfix ++/--: expr++ or expr--
    if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        bool isIncrement = check(TokenType::PlusPlus);
        advance(); // consume ++ or --
        consume(TokenType::Semicolon, "Expected ';' after increment/decrement.");

        if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<IncrementStmt>(varExpr->name, isIncrement);
        } else {
            error(previous(), "Increment/decrement target must be a variable.");
            throw std::runtime_error("Invalid increment/decrement target.");
        }
    }

    // Check for compound assignment: expr += value; etc.
    if (check(TokenType::PlusEqual) || check(TokenType::MinusEqual) ||
        check(TokenType::StarEqual) || check(TokenType::SlashEqual) ||
        check(TokenType::PercentEqual) ||
        check(TokenType::AmpersandEqual) || check(TokenType::PipeEqual) ||
        check(TokenType::CaretEqual) || check(TokenType::LeftShiftEqual) ||
        check(TokenType::RightShiftEqual) || check(TokenType::DoubleStarEqual)) {

        Token opToken = advance();
        BinaryOp op;
        switch (opToken.type) {
            case TokenType::PlusEqual:       op = BinaryOp::Add; break;
            case TokenType::MinusEqual:      op = BinaryOp::Sub; break;
            case TokenType::StarEqual:       op = BinaryOp::Mul; break;
            case TokenType::SlashEqual:      op = BinaryOp::Div; break;
            case TokenType::PercentEqual:    op = BinaryOp::Mod; break;
            case TokenType::AmpersandEqual:  op = BinaryOp::BitAnd; break;
            case TokenType::PipeEqual:       op = BinaryOp::BitOr; break;
            case TokenType::CaretEqual:      op = BinaryOp::BitXor; break;
            case TokenType::LeftShiftEqual:  op = BinaryOp::Shl; break;
            case TokenType::RightShiftEqual: op = BinaryOp::Shr; break;
            case TokenType::DoubleStarEqual: op = BinaryOp::Pow; break;
            default: op = BinaryOp::Add; break; // unreachable
        }

        auto value = expression();
        consume(TokenType::Semicolon, "Expected ';' after compound assignment.");

        if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<CompoundAssignStmt>(varExpr->name, op, std::move(value));
        } else {
            error(opToken, "Compound assignment target must be a variable.");
            throw std::runtime_error("Invalid compound assignment target.");
        }
    }

    // Check for null-coalescing assignment: expr ?= value;
    // Desugars to: expr = expr ?? value
    if (match(TokenType::QuestionEqual)) {
        auto value = expression();
        consume(TokenType::Semicolon, "Expected ';' after null-coalescing assignment.");
        if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            std::string name = varExpr->name;
            auto varRef = std::make_unique<VariableExpr>(name);
            auto nullCoalesce = std::make_unique<NullCoalesceExpr>(std::move(varRef), std::move(value));
            return std::make_unique<AssignStmt>(name, std::move(nullCoalesce));
        }
        error(previous(), "Left side of '?=' must be a variable.");
        throw std::runtime_error("Invalid null-coalescing assignment target.");
    }

    // Plain expression statement
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

std::unique_ptr<Statement> Parser::assignStatement() {
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::Assign, "Expected '=' after identifier.");
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<AssignStmt>(name, std::move(expr));
}

// ============================================================================
// Variable declarations
// ============================================================================

std::unique_ptr<Statement> Parser::letStatement(bool isMutable) {
    // Destructuring let: let [a, b, c] = expr;
    if (match(TokenType::LBracket)) {
        std::vector<std::string> names;
        do {
            consume(TokenType::Identifier, "Expected variable name in destructuring.");
            names.push_back(tokens[current - 1].lexeme);
        } while (match(TokenType::Comma));
        consume(TokenType::RBracket, "Expected ']' after destructuring names.");
        consume(TokenType::Assign, "Expected '=' after destructuring pattern.");
        auto initializer = expression();
        consume(TokenType::Semicolon, "Expected ';' after destructuring declaration.");
        return std::make_unique<DestructureLetStmt>(std::move(names), std::move(initializer), isMutable);
    }

    // Object destructuring: let {a, b} = expr;
    if (match(TokenType::LBrace)) {
        std::vector<std::string> names;
        do {
            consume(TokenType::Identifier, "Expected variable name in object destructuring.");
            names.push_back(tokens[current - 1].lexeme);
        } while (match(TokenType::Comma));
        consume(TokenType::RBrace, "Expected '}' after object destructuring names.");
        consume(TokenType::Assign, "Expected '=' after object destructuring pattern.");
        auto initializer = expression();
        consume(TokenType::Semicolon, "Expected ';' after object destructuring declaration.");
        return std::make_unique<ObjectDestructureLetStmt>(std::move(names), std::move(initializer), isMutable);
    }

    consume(TokenType::Identifier, "Expected variable name after 'let' or 'var'.");
    std::string name = tokens[current - 1].lexeme;

    // Optional type annotation: let x: int = 10;
    std::optional<std::string> typeAnnotation;
    if (match(TokenType::Colon)) {
        if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
            match(TokenType::Bool) || match(TokenType::Str)) {
            typeAnnotation = tokens[current - 1].lexeme;
        } else {
            error(peek(), "Expected type name after ':'.");
        }
    }

    consume(TokenType::Assign, "Expected '=' after variable name.");
    auto initializer = expression();
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");
    return std::make_unique<LetStmt>(name, std::move(initializer), typeAnnotation, isMutable);
}

std::unique_ptr<Statement> Parser::constStatement() {
    consume(TokenType::Identifier, "Expected constant name after 'const'.");
    std::string name = tokens[current - 1].lexeme;

    // Type annotation is required for const
    consume(TokenType::Colon, "Expected ':' and type annotation after constant name.");

    std::string typeAnnotation;
    if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
        match(TokenType::Bool) || match(TokenType::Str)) {
        typeAnnotation = tokens[current - 1].lexeme;
    } else {
        error(peek(), "Expected type name after ':'.");
        throw std::runtime_error("Expected type name in const declaration.");
    }

    consume(TokenType::Assign, "Expected '=' after type annotation.");
    auto initializer = expression();
    consume(TokenType::Semicolon, "Expected ';' after constant declaration.");
    return std::make_unique<ConstStmt>(name, std::move(initializer), typeAnnotation);
}

// ============================================================================
// Control flow statements
// ============================================================================

std::unique_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::LParen, "Expected '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after if condition.");
    auto thenBranch = statement();
    std::unique_ptr<Statement> elseBranch = nullptr;
    if (match(TokenType::Else)) {
        elseBranch = statement();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Statement> Parser::blockStatement() {
    // Assumes '{' has already been consumed
    std::vector<std::unique_ptr<Statement>> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        stmts.push_back(statement());
    }
    consume(TokenType::RBrace, "Expected '}' to close block.");
    return std::make_unique<BlockStmt>(std::move(stmts));
}

std::unique_ptr<Statement> Parser::forStatement() {
    // Check for destructuring: for [a, b] in expr { ... }
    if (check(TokenType::LBracket)) {
        return forDestructureStatement();
    }

    consume(TokenType::Identifier, "Expected variable name in for loop.");
    std::string varName = tokens[current - 1].lexeme;
    consume(TokenType::In, "Expected 'in' after variable name in for loop.");
    auto iterable = expression();
    consume(TokenType::LBrace, "Expected '{' to start for loop body.");
    auto body = blockStatement();
    return std::make_unique<ForStmt>(varName, std::move(iterable), std::move(body));
}

std::unique_ptr<Statement> Parser::forDestructureStatement() {
    consume(TokenType::LBracket, "Expected '[' for destructuring in for loop.");
    std::vector<std::string> vars;
    do {
        consume(TokenType::Identifier, "Expected variable name in for destructuring.");
        vars.push_back(tokens[current - 1].lexeme);
    } while (match(TokenType::Comma));
    consume(TokenType::RBracket, "Expected ']' after destructuring variables.");
    consume(TokenType::In, "Expected 'in' after destructuring pattern.");
    auto iterable = expression();
    consume(TokenType::LBrace, "Expected '{' to start for loop body.");
    auto body = blockStatement();
    return std::make_unique<ForDestructureStmt>(std::move(vars), std::move(iterable), std::move(body));
}

std::unique_ptr<Statement> Parser::whileStatement() {
    consume(TokenType::LParen, "Expected '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after while condition.");
    auto body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::doWhileStatement() {
    // Already consumed 'do'
    consume(TokenType::LBrace, "Expected '{' after 'do'.");
    auto body = blockStatement();
    consume(TokenType::While, "Expected 'while' after do block.");
    consume(TokenType::LParen, "Expected '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after while condition.");
    consume(TokenType::Semicolon, "Expected ';' after do-while statement.");
    return std::make_unique<DoWhileStmt>(std::move(body), std::move(condition));
}

std::unique_ptr<Statement> Parser::loopStatement() {
    consume(TokenType::LBrace, "Expected '{' after 'loop'.");
    auto body = blockStatement();
    return std::make_unique<LoopStmt>(std::move(body));
}

std::unique_ptr<Statement> Parser::deferStatement() {
    auto stmt = statement();
    return std::make_unique<DeferStmt>(std::move(stmt));
}

std::unique_ptr<Statement> Parser::assertStatement() {
    consume(TokenType::LParen, "Expected '(' after 'assert'.");
    auto condition = expression();

    std::string message;
    if (match(TokenType::Comma)) {
        if (!match(TokenType::String)) {
            error(peek(), "Expected string message after comma in assert.");
        }
        message = tokens[current - 1].lexeme;
    }

    consume(TokenType::RParen, "Expected ')' after assert expression.");
    consume(TokenType::Semicolon, "Expected ';' after assert.");

    return std::make_unique<AssertStmt>(std::move(condition), message, false);
}

// ============================================================================
// Function and return statements
// ============================================================================

std::unique_ptr<Statement> Parser::functionStatement() {
    consume(TokenType::Identifier, "Expected function name.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::LParen, "Expected '(' after function name.");

    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    std::vector<std::unique_ptr<Expression>> paramDefaults;
    bool hadDefault = false;

    if (!check(TokenType::RParen)) {
        do {
            consume(TokenType::Identifier, "Expected parameter name.");
            params.push_back(tokens[current - 1].lexeme);

            // Optional type annotation: func add(a: int, b: int)
            if (match(TokenType::Colon)) {
                if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                    match(TokenType::Bool) || match(TokenType::Str)) {
                    paramTypes.push_back(tokens[current - 1].lexeme);
                } else {
                    error(peek(), "Expected type name after ':'.");
                    paramTypes.push_back("");
                }
            } else {
                paramTypes.push_back("");
            }

            // Optional default value: func foo(x, y = 10)
            if (match(TokenType::Assign)) {
                paramDefaults.push_back(expression());
                hadDefault = true;
            } else {
                if (hadDefault) {
                    error(peek(), "Required parameter cannot follow a parameter with default value.");
                }
                paramDefaults.push_back(nullptr);
            }
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RParen, "Expected ')' after parameters.");

    // Optional return type annotation: func add(...) -> int { ... }
    std::string returnType = "";
    if (match(TokenType::ReturnArrow)) {
        if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
            match(TokenType::Bool) || match(TokenType::Str)) {
            returnType = tokens[current - 1].lexeme;
        } else {
            error(peek(), "Expected return type after '->'.");
        }
    }

    // Abstract methods: func name(params); (no body)
    if (match(TokenType::Semicolon)) {
        return std::make_unique<FunctionStmt>(name, std::move(params), nullptr, std::move(paramTypes), returnType, std::move(paramDefaults));
    }

    consume(TokenType::LBrace, "Expected '{' to open function body.");
    auto body = blockStatement();
    return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body), std::move(paramTypes), returnType, std::move(paramDefaults));
}

std::unique_ptr<Statement> Parser::returnStatement() {
    // Support bare return: return;
    if (check(TokenType::Semicolon)) {
        advance(); // consume ';'
        return std::make_unique<ReturnStmt>(nullptr);
    }
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(expr));
}

// ============================================================================
// Extern block
// ============================================================================

std::unique_ptr<Statement> Parser::externBlock() {
    consume(TokenType::String, "Expected ABI string after 'extern' (e.g., \"C\").");
    std::string abi = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Expected '{' after extern ABI string.");

    std::vector<std::unique_ptr<ExternFunctionDecl>> functions;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        consume(TokenType::Func, "Expected 'func' in extern block.");
        consume(TokenType::Identifier, "Expected function name.");
        std::string funcName = tokens[current - 1].lexeme;

        consume(TokenType::LParen, "Expected '(' after function name.");

        std::vector<std::string> params;
        std::vector<std::string> paramTypes;
        bool isVarArg = false;

        if (!check(TokenType::RParen)) {
            do {
                // Check for varargs (...)
                if (match(TokenType::Dot)) {
                    if (match(TokenType::Dot) && match(TokenType::Dot)) {
                        isVarArg = true;
                        break;
                    } else {
                        error(peek(), "Expected '...' for variadic arguments.");
                    }
                }

                consume(TokenType::Identifier, "Expected parameter name.");
                params.push_back(tokens[current - 1].lexeme);

                // Type annotation required in extern
                consume(TokenType::Colon, "Expected ':' after parameter name in extern function.");
                if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                    match(TokenType::Bool) || match(TokenType::Str) || match(TokenType::Star)) {
                    std::string type = tokens[current - 1].lexeme;

                    // Handle pointer types: *const char, *mut void
                    if (tokens[current - 1].type == TokenType::Star) {
                        if (match(TokenType::Const)) {
                            type = "*const ";
                        } else if (match(TokenType::Mut)) {
                            type = "*mut ";
                        } else {
                            type = "*";
                        }
                        if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                            match(TokenType::Bool) || match(TokenType::Str)) {
                            type += tokens[current - 1].lexeme;
                        } else {
                            error(peek(), "Expected type after pointer qualifier.");
                        }
                    }

                    paramTypes.push_back(type);
                } else {
                    error(peek(), "Expected type name.");
                    paramTypes.push_back("int32");
                }
            } while (match(TokenType::Comma));
        }

        consume(TokenType::RParen, "Expected ')' after parameters.");

        // Return type
        std::string returnType = "void";
        if (match(TokenType::ReturnArrow)) {
            if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                match(TokenType::Bool) || match(TokenType::Str) || match(TokenType::Star)) {
                returnType = tokens[current - 1].lexeme;

                // Handle pointer return types
                if (tokens[current - 1].type == TokenType::Star) {
                    if (match(TokenType::Const)) {
                        returnType = "*const ";
                    } else if (match(TokenType::Mut)) {
                        returnType = "*mut ";
                    } else {
                        returnType = "*";
                    }
                    if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                        match(TokenType::Bool) || match(TokenType::Str)) {
                        returnType += tokens[current - 1].lexeme;
                    }
                }
            }
        }

        consume(TokenType::Semicolon, "Expected ';' after extern function declaration.");

        functions.push_back(std::make_unique<ExternFunctionDecl>(
            funcName, std::move(params), std::move(paramTypes), returnType, isVarArg
        ));
    }

    consume(TokenType::RBrace, "Expected '}' to close extern block.");

    return std::make_unique<ExternBlock>(abi, std::move(functions));
}

// ============================================================================
// Enum, match, switch
// ============================================================================

std::unique_ptr<Statement> Parser::enumStatement() {
    consume(TokenType::Identifier, "Expected enum name.");
    std::string enumName = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Expected '{' after enum name.");

    std::vector<std::string> values;
    std::vector<std::vector<std::string>> variantParams;
    do {
        consume(TokenType::Identifier, "Expected identifier inside enum.");
        values.push_back(tokens[current - 1].lexeme);

        // Check for associated data: Variant(param1, param2)
        std::vector<std::string> params;
        if (match(TokenType::LParen)) {
            if (!check(TokenType::RParen)) {
                do {
                    consume(TokenType::Identifier, "Expected parameter name in enum variant.");
                    params.push_back(tokens[current - 1].lexeme);
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after enum variant parameters.");
        }
        variantParams.push_back(std::move(params));
    } while (match(TokenType::Comma));

    consume(TokenType::RBrace, "Expected '}' after enum values.");
    auto stmt = std::make_unique<EnumStmt>(enumName, values);
    stmt->variantParams = std::move(variantParams);
    return stmt;
}

std::unique_ptr<Statement> Parser::matchStatement() {
    consume(TokenType::LParen, "Expected '(' after 'match'.");
    auto expr = expression();
    consume(TokenType::RParen, "Expected ')' after match expression.");

    consume(TokenType::LBrace, "Expected '{' after 'match (...)'.");

    std::vector<MatchArm> arms;

    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        auto pat = pattern();
        consume(TokenType::Arrow, "Expected '=>' after match pattern.");
        auto body = statement();
        arms.emplace_back(std::move(pat), std::move(body));
    }

    consume(TokenType::RBrace, "Expected '}' at end of match statement.");
    return std::make_unique<MatchStmt>(std::move(expr), std::move(arms));
}

std::unique_ptr<Statement> Parser::switchStatement() {
    consume(TokenType::LParen, "Expected '(' after 'switch'.");
    auto expr = expression();
    consume(TokenType::RParen, "Expected ')' after switch expression.");

    consume(TokenType::LBrace, "Expected '{' after 'switch (...)'.");

    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>> cases;
    std::unique_ptr<Statement> defaultCase = nullptr;

    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        if (match(TokenType::Case)) {
            auto val = expression();
            consume(TokenType::Arrow, "Expected '=>' after case value.");
            auto body = statement();
            cases.emplace_back(std::move(val), std::move(body));
        } else if (match(TokenType::Default)) {
            consume(TokenType::Arrow, "Expected '=>' after 'default'.");
            defaultCase = statement();
        } else {
            error(peek(), "Expected 'case' or 'default' in switch.");
            break;
        }
    }

    consume(TokenType::RBrace, "Expected '}' at end of switch statement.");
    return std::make_unique<SwitchStmt>(std::move(expr), std::move(cases), std::move(defaultCase));
}

// ============================================================================
// Struct and class
// ============================================================================

std::unique_ptr<Statement> Parser::structStatement() {
    consume(TokenType::Identifier, "Expected struct name.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::LBrace, "Expected '{' after struct name.");

    std::vector<std::string> fields;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        consume(TokenType::Identifier, "Expected field name inside struct.");
        fields.push_back(tokens[current - 1].lexeme);
        consume(TokenType::Semicolon, "Expected ';' after struct field.");
    }

    consume(TokenType::RBrace, "Expected '}' to close struct.");
    return std::make_unique<StructStmt>(name, std::move(fields));
}

std::unique_ptr<Statement> Parser::classStatement() {
    consume(TokenType::Identifier, "Expected class name.");
    std::string className = tokens[current - 1].lexeme;

    // Optional inheritance: class Dog extends Animal
    std::string parentName;
    if (match(TokenType::Extends)) {
        consume(TokenType::Identifier, "Expected parent class name after 'extends'.");
        parentName = tokens[current - 1].lexeme;
    }

    // Optional trait implementation: class User impl Serializable, Loggable
    std::vector<std::string> implTraitNames;
    if (match(TokenType::Impl)) {
        do {
            consume(TokenType::Identifier, "Expected trait name after 'impl'.");
            implTraitNames.push_back(tokens[current - 1].lexeme);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::LBrace, "Expected '{' after class name.");

    std::vector<std::string> fields;
    std::vector<AccessModifier> fieldAccess;
    std::vector<std::unique_ptr<FunctionStmt>> methods;
    std::vector<AccessModifier> methodAccess;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> staticFields;
    std::vector<std::unique_ptr<FunctionStmt>> staticMethods;
    std::vector<std::unique_ptr<FunctionStmt>> getters;
    std::vector<std::unique_ptr<FunctionStmt>> setters;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> lazyFields;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        // Check for access modifiers
        AccessModifier access = AccessModifier::Public;
        if (match(TokenType::Pub)) {
            access = AccessModifier::Public;
        } else if (match(TokenType::Priv)) {
            access = AccessModifier::Private;
        }

        // Check for static
        if (match(TokenType::Static)) {
            if (match(TokenType::Func)) {
                auto method = functionStatement();
                staticMethods.push_back(std::unique_ptr<FunctionStmt>(
                    static_cast<FunctionStmt*>(method.release())));
            } else if (match(TokenType::Let)) {
                consume(TokenType::Identifier, "Expected static field name.");
                std::string fieldName = tokens[current - 1].lexeme;
                consume(TokenType::Assign, "Expected '=' after static field name.");
                auto initializer = expression();
                consume(TokenType::Semicolon, "Expected ';' after static field.");
                staticFields.emplace_back(fieldName, std::move(initializer));
            } else {
                error(peek(), "Expected 'func' or 'let' after 'static'.");
                break;
            }
        }
        // Getter: get propName() { ... }
        else if (peek().lexeme == "get" && peek().type == TokenType::Identifier) {
            advance(); // consume 'get'
            consume(TokenType::Identifier, "Expected property name after 'get'.");
            std::string propName = tokens[current - 1].lexeme;
            consume(TokenType::LParen, "Expected '(' after getter name.");
            consume(TokenType::RParen, "Expected ')' after getter params.");
            consume(TokenType::LBrace, "Expected '{' for getter body.");
            auto body = blockStatement();
            auto getter = std::make_unique<FunctionStmt>(propName, std::vector<std::string>{}, std::move(body));
            getters.push_back(std::move(getter));
        }
        // Setter: set propName(value) { ... }
        else if (peek().lexeme == "set" && peek().type == TokenType::Identifier) {
            advance(); // consume 'set'
            consume(TokenType::Identifier, "Expected property name after 'set'.");
            std::string propName = tokens[current - 1].lexeme;
            consume(TokenType::LParen, "Expected '(' after setter name.");
            consume(TokenType::Identifier, "Expected parameter name in setter.");
            std::string param = tokens[current - 1].lexeme;
            consume(TokenType::RParen, "Expected ')' after setter param.");
            consume(TokenType::LBrace, "Expected '{' for setter body.");
            auto body = blockStatement();
            auto setter = std::make_unique<FunctionStmt>(propName, std::vector<std::string>{param}, std::move(body));
            setters.push_back(std::move(setter));
        }
        // Lazy field: lazy let fieldName = expr;
        else if (match(TokenType::Lazy)) {
            consume(TokenType::Let, "Expected 'let' after 'lazy'.");
            consume(TokenType::Identifier, "Expected field name after 'lazy let'.");
            std::string fieldName = tokens[current - 1].lexeme;
            consume(TokenType::Assign, "Expected '=' after lazy field name.");
            auto initializer = expression();
            consume(TokenType::Semicolon, "Expected ';' after lazy field initializer.");
            lazyFields.emplace_back(fieldName, std::move(initializer));
        }
        // Method
        else if (match(TokenType::Func)) {
            auto method = functionStatement();
            methods.push_back(std::unique_ptr<FunctionStmt>(
                static_cast<FunctionStmt*>(method.release())));
            methodAccess.push_back(access);
        }
        // Field
        else if (match(TokenType::Let)) {
            consume(TokenType::Identifier, "Expected field name.");
            fields.push_back(tokens[current - 1].lexeme);
            fieldAccess.push_back(access);
            consume(TokenType::Semicolon, "Expected ';' after field.");
        } else {
            error(peek(), "Expected 'func', 'let', 'lazy', 'static', 'get', 'set', 'pub', or 'priv' inside class.");
            break;
        }
    }

    consume(TokenType::RBrace, "Expected '}' to close class.");
    auto classStmt = std::make_unique<ClassStmt>(className, std::move(fields), std::move(methods), parentName);
    classStmt->fieldAccess = std::move(fieldAccess);
    classStmt->methodAccess = std::move(methodAccess);
    classStmt->staticFields = std::move(staticFields);
    classStmt->staticMethods = std::move(staticMethods);
    classStmt->getters = std::move(getters);
    classStmt->setters = std::move(setters);
    classStmt->lazyFields = std::move(lazyFields);
    classStmt->implTraits = std::move(implTraitNames);
    return classStmt;
}

// ============================================================================
// Trait and Impl statements
// ============================================================================

std::unique_ptr<Statement> Parser::traitStatement() {
    consume(TokenType::Identifier, "Expected trait name.");
    std::string traitName = tokens[current - 1].lexeme;
    consume(TokenType::LBrace, "Expected '{' after trait name.");

    std::vector<std::string> requiredMethods;
    std::vector<std::unique_ptr<FunctionStmt>> defaultMethods;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        consume(TokenType::Func, "Expected 'func' inside trait.");
        auto method = functionStatement();
        auto funcStmt = std::unique_ptr<FunctionStmt>(static_cast<FunctionStmt*>(method.release()));
        if (funcStmt->body == nullptr) {
            // Abstract: func name(params);
            requiredMethods.push_back(funcStmt->name);
        } else {
            // Default implementation
            defaultMethods.push_back(std::move(funcStmt));
        }
    }

    consume(TokenType::RBrace, "Expected '}' to close trait.");
    return std::make_unique<TraitStmt>(traitName, std::move(requiredMethods), std::move(defaultMethods));
}

std::unique_ptr<Statement> Parser::repeatStatement() {
    // Use range() instead of expression() to avoid consuming 'as' (cast operator)
    auto count = range();
    std::string varName;
    if (match(TokenType::As)) {
        consume(TokenType::Identifier, "Expected variable name after 'as' in repeat.");
        varName = tokens[current - 1].lexeme;
    }
    consume(TokenType::LBrace, "Expected '{' after repeat.");
    auto body = blockStatement();
    return std::make_unique<RepeatStmt>(std::move(count), varName, std::move(body));
}

std::unique_ptr<Statement> Parser::extendStatement() {
    consume(TokenType::Identifier, "Expected type name after 'extend'.");
    std::string typeName = tokens[current - 1].lexeme;
    consume(TokenType::LBrace, "Expected '{' after extend type name.");

    std::vector<std::unique_ptr<FunctionStmt>> methods;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        consume(TokenType::Func, "Expected 'func' inside extend block.");
        auto method = functionStatement();
        methods.push_back(std::unique_ptr<FunctionStmt>(
            static_cast<FunctionStmt*>(method.release())));
    }

    consume(TokenType::RBrace, "Expected '}' to close extend block.");
    return std::make_unique<ExtendStmt>(typeName, std::move(methods));
}

std::unique_ptr<Statement> Parser::implStatement() {
    consume(TokenType::Identifier, "Expected trait name after 'impl'.");
    std::string traitName = tokens[current - 1].lexeme;
    consume(TokenType::For, "Expected 'for' after trait name in impl.");
    consume(TokenType::Identifier, "Expected class name after 'for' in impl.");
    std::string className = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Expected '{' after impl declaration.");

    std::vector<std::unique_ptr<FunctionStmt>> methods;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        if (match(TokenType::Func)) {
            auto method = functionStatement();
            methods.push_back(std::unique_ptr<FunctionStmt>(
                static_cast<FunctionStmt*>(method.release())));
        } else {
            error(peek(), "Expected 'func' inside impl block.");
            break;
        }
    }

    consume(TokenType::RBrace, "Expected '}' to close impl.");
    return std::make_unique<ImplStmt>(traitName, className, std::move(methods));
}

// ============================================================================
// Module system
// ============================================================================

std::unique_ptr<Statement> Parser::importStatement() {
    consume(TokenType::String, "Expected module path as string after 'import'.");
    std::string path = tokens[current - 1].lexeme;
    consume(TokenType::Semicolon, "Expected ';' after import path.");
    return std::make_unique<ImportStmt>(path);
}

std::unique_ptr<Statement> Parser::exportStatement() {
    auto stmt = statement();
    return std::make_unique<ExportStmt>(std::move(stmt));
}

// ============================================================================
// Error handling: try/catch and throw
// ============================================================================

std::unique_ptr<Statement> Parser::tryCatchStatement() {
    // try { ... } catch (e) { ... } finally { ... }
    // try { ... } catch (TypeError | ValueError as e) { ... } finally { ... }
    consume(TokenType::LBrace, "Expected '{' after 'try'.");
    auto tryBlock = blockStatement();

    consume(TokenType::Catch, "Expected 'catch' after try block.");
    consume(TokenType::LParen, "Expected '(' after 'catch'.");
    consume(TokenType::Identifier, "Expected identifier in catch clause.");
    std::string firstIdent = tokens[current - 1].lexeme;

    std::string errorVar;
    std::vector<std::string> errorTypes;

    // Check if this is multi-catch: catch (Type1 | Type2 as varName) or catch (Type as varName)
    if (check(TokenType::Pipe) || check(TokenType::As)) {
        // First identifier is a type name
        errorTypes.push_back(firstIdent);

        // Parse additional types separated by |
        while (match(TokenType::Pipe)) {
            consume(TokenType::Identifier, "Expected error type name after '|' in catch.");
            errorTypes.push_back(tokens[current - 1].lexeme);
        }

        // Expect 'as' followed by variable name
        consume(TokenType::As, "Expected 'as' after error type(s) in catch.");
        consume(TokenType::Identifier, "Expected variable name after 'as' in catch.");
        errorVar = tokens[current - 1].lexeme;
    } else {
        // Simple catch: catch (e)
        errorVar = firstIdent;
    }

    consume(TokenType::RParen, "Expected ')' after catch clause.");

    consume(TokenType::LBrace, "Expected '{' after 'catch (...)'.");
    auto catchBlock = blockStatement();

    // Optional finally block
    std::unique_ptr<Statement> finallyBlock = nullptr;
    if (match(TokenType::Finally)) {
        consume(TokenType::LBrace, "Expected '{' after 'finally'.");
        finallyBlock = blockStatement();
    }

    auto stmt = std::make_unique<TryCatchStmt>(std::move(tryBlock), errorVar, std::move(catchBlock), std::move(finallyBlock));
    stmt->errorTypes = std::move(errorTypes);
    return stmt;
}

std::unique_ptr<Statement> Parser::throwStatement() {
    // throw expression;
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after throw expression.");
    return std::make_unique<ThrowStmt>(std::move(expr));
}

// ============================================================================
// Expressions - Precedence climbing
//
// Precedence (lowest to highest):
//  0. ternary      ? :
//  1. pipe         |>
//  1b. compose     >>>
//  1c. null_coalesce ??
//  2. logic_or     ||
//  3. logic_and    &&
//  4. bitwise_or   |
//  5. bitwise_xor  ^
//  6. bitwise_and  &
//  7. equality     == !=
//  8. comparison   < <= > >=
//  9. bitwise_shift << >>
// 10. cast         as
// 11. range        .. ..=
// 12. term         + -
// 13. factor       * / %
// 14. power        **
// 15. unary        ! - ~
// 16. primary
// ============================================================================

std::unique_ptr<Expression> Parser::expression() {
    return ternary();
}

// Ternary: condition ? thenExpr : elseExpr (right-associative)
std::unique_ptr<Expression> Parser::ternary() {
    auto expr = pipe();
    if (match(TokenType::Question)) {
        auto thenExpr = ternary();  // right-associative
        consume(TokenType::Colon, "Expected ':' in ternary expression.");
        auto elseExpr = ternary();
        return std::make_unique<TernaryExpr>(std::move(expr), std::move(thenExpr), std::move(elseExpr));
    }
    return expr;
}

// 1. Pipe operator: expr |> func
std::unique_ptr<Expression> Parser::pipe() {
    auto expr = compose();
    while (match(TokenType::PipeArrow)) {
        auto func = compose();
        expr = std::make_unique<PipeExpr>(std::move(expr), std::move(func));
    }
    return expr;
}

// 1b. Compose operator: f >>> g (function composition)
std::unique_ptr<Expression> Parser::compose() {
    auto expr = null_coalesce();
    while (match(TokenType::Compose)) {
        auto right = null_coalesce();
        expr = std::make_unique<ComposeExpr>(std::move(expr), std::move(right));
    }
    return expr;
}

// Null coalescing: expr ?? default
std::unique_ptr<Expression> Parser::null_coalesce() {
    auto expr = logic_or();
    while (match(TokenType::QuestionQuestion)) {
        auto right = logic_or();
        expr = std::make_unique<NullCoalesceExpr>(std::move(expr), std::move(right));
    }
    return expr;
}

// 2. Logical OR: ||
std::unique_ptr<Expression> Parser::logic_or() {
    auto expr = logic_and();
    while (match(TokenType::OrOr)) {
        auto right = logic_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::Or, std::move(right));
    }
    return expr;
}

// 3. Logical AND: &&
std::unique_ptr<Expression> Parser::logic_and() {
    auto expr = bitwise_or();
    while (match(TokenType::AndAnd)) {
        auto right = bitwise_or();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::And, std::move(right));
    }
    return expr;
}

// 4. Bitwise OR: |
std::unique_ptr<Expression> Parser::bitwise_or() {
    auto expr = bitwise_xor();
    while (match(TokenType::Pipe)) {
        auto right = bitwise_xor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::BitOr, std::move(right));
    }
    return expr;
}

// 5. Bitwise XOR: ^
std::unique_ptr<Expression> Parser::bitwise_xor() {
    auto expr = bitwise_and();
    while (match(TokenType::Caret)) {
        auto right = bitwise_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::BitXor, std::move(right));
    }
    return expr;
}

// 6. Bitwise AND: &
std::unique_ptr<Expression> Parser::bitwise_and() {
    auto expr = equality();
    while (match(TokenType::Ampersand)) {
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::BitAnd, std::move(right));
    }
    return expr;
}

// 7. Equality: == !=
std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();
    while (match(TokenType::EqualEqual) || match(TokenType::BangEqual)) {
        Token op = tokens[current - 1];
        BinaryOp binOp = (op.type == TokenType::EqualEqual) ? BinaryOp::Equal : BinaryOp::NotEqual;
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}

// 8. Comparison: < <= > >= in is not-in
std::unique_ptr<Expression> Parser::comparison() {
    auto expr = bitwise_shift();
    while (true) {
        // Check for 'not in' first (two-token operator)
        // TokenType::Not comes from '!' character; also accept identifier "not"
        if (((check(TokenType::Not)) || (check(TokenType::Identifier) && peek().lexeme == "not")) &&
            current + 1 < tokens.size() && tokens[current + 1].type == TokenType::In) {
            advance(); // consume 'not' or '!'
            advance(); // consume 'in'
            auto right = bitwise_shift();
            expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::NotIn, std::move(right));
            continue;
        }

        if (!(match(TokenType::Less) || match(TokenType::LessEqual) ||
              match(TokenType::Greater) || match(TokenType::GreaterEqual) ||
              match(TokenType::In) || match(TokenType::Is))) {
            break;
        }

        Token op = tokens[current - 1];

        // Handle 'is' operator: expr is TypeName
        if (op.type == TokenType::Is) {
            // Consume the type name (could be Identifier, Int, Float, Bool, Str, None keywords)
            if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                match(TokenType::Bool) || match(TokenType::Str) || match(TokenType::None) ||
                match(TokenType::Func)) {
                std::string typeName = tokens[current - 1].lexeme;
                expr = std::make_unique<IsExpr>(std::move(expr), typeName);
            } else {
                error(peek(), "Expected type name after 'is'.");
            }
            continue;
        }

        BinaryOp binOp;
        switch (op.type) {
            case TokenType::Less:         binOp = BinaryOp::Less; break;
            case TokenType::LessEqual:    binOp = BinaryOp::LessEqual; break;
            case TokenType::Greater:      binOp = BinaryOp::Greater; break;
            case TokenType::GreaterEqual: binOp = BinaryOp::GreaterEqual; break;
            case TokenType::In:           binOp = BinaryOp::In; break;
            default:                      binOp = BinaryOp::Less; break;
        }
        auto right = bitwise_shift();

        // Chained comparisons: 1 < x < 10 → ChainedComparisonExpr{[1,x,10], [<,<]}
        if (check(TokenType::Less) || check(TokenType::LessEqual) ||
            check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
            auto chained = std::make_unique<ChainedComparisonExpr>();
            chained->operands.push_back(std::move(expr));
            chained->operators.push_back(binOp);
            chained->operands.push_back(std::move(right));

            while (check(TokenType::Less) || check(TokenType::LessEqual) ||
                   check(TokenType::Greater) || check(TokenType::GreaterEqual)) {
                Token chainOp = advance();
                BinaryOp chainBinOp;
                switch (chainOp.type) {
                    case TokenType::Less:         chainBinOp = BinaryOp::Less; break;
                    case TokenType::LessEqual:    chainBinOp = BinaryOp::LessEqual; break;
                    case TokenType::Greater:      chainBinOp = BinaryOp::Greater; break;
                    case TokenType::GreaterEqual: chainBinOp = BinaryOp::GreaterEqual; break;
                    default:                      chainBinOp = BinaryOp::Less; break;
                }
                chained->operators.push_back(chainBinOp);
                chained->operands.push_back(bitwise_shift());
            }
            expr = std::move(chained);
            continue;
        }

        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}

// 9. Bitwise shift: << >>
std::unique_ptr<Expression> Parser::bitwise_shift() {
    auto expr = cast();
    while (match(TokenType::LeftShift) || match(TokenType::RightShift)) {
        Token op = tokens[current - 1];
        BinaryOp binOp = (op.type == TokenType::LeftShift) ? BinaryOp::Shl : BinaryOp::Shr;
        auto right = cast();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}

// 10. Cast: expr as Type
std::unique_ptr<Expression> Parser::cast() {
    auto expr = range();

    while (match(TokenType::As)) {
        std::string targetType;
        if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
            match(TokenType::Bool) || match(TokenType::Str)) {
            targetType = tokens[current - 1].lexeme;
        } else {
            error(peek(), "Expected type name after 'as'.");
            throw std::runtime_error("Expected type name in cast expression.");
        }
        expr = std::make_unique<CastExpr>(std::move(expr), targetType);
    }

    return expr;
}

// 11. Range: .. and ..=
std::unique_ptr<Expression> Parser::range() {
    auto expr = term();

    if (match(TokenType::DotDot) || match(TokenType::DotDotEqual)) {
        bool inclusive = tokens[current - 1].type == TokenType::DotDotEqual;
        auto end = term();
        return std::make_unique<RangeExpr>(std::move(expr), std::move(end), inclusive);
    }

    return expr;
}

// 12. Term: + -
std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = tokens[current - 1];
        BinaryOp binOp = (op.type == TokenType::Plus) ? BinaryOp::Add : BinaryOp::Sub;
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}

// 13. Factor: * / %
std::unique_ptr<Expression> Parser::factor() {
    auto expr = power();
    while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
        Token op = tokens[current - 1];
        BinaryOp binOp;
        switch (op.type) {
            case TokenType::Star:    binOp = BinaryOp::Mul; break;
            case TokenType::Slash:   binOp = BinaryOp::Div; break;
            case TokenType::Percent: binOp = BinaryOp::Mod; break;
            default:                 binOp = BinaryOp::Mul; break;
        }
        auto right = power();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}

// 14. Power: ** (right-associative)
std::unique_ptr<Expression> Parser::power() {
    auto expr = unary();
    if (match(TokenType::DoubleStar)) {
        // Right-associative: recurse into power(), not unary()
        auto right = power();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::Pow, std::move(right));
    }
    return expr;
}

// 15. Unary: ! - ~
std::unique_ptr<Expression> Parser::unary() {
    // Don't consume 'not' if followed by 'in' (handled by comparison as 'not in')
    if (check(TokenType::Not)) {
        if (!(current + 1 < tokens.size() && tokens[current + 1].type == TokenType::In)) {
            advance(); // consume '!'
            auto right = unary();
            return std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(right));
        }
    }
    if (match(TokenType::Minus)) {
        auto right = unary();
        return std::make_unique<UnaryExpr>(UnaryOp::Neg, std::move(right));
    }
    if (match(TokenType::Tilde)) {
        auto right = unary();
        return std::make_unique<UnaryExpr>(UnaryOp::BitNot, std::move(right));
    }
    return primary();
}

// ============================================================================
// Primary expressions
// ============================================================================

std::unique_ptr<Expression> Parser::primary() {
    // Walrus operator: let x := expr (assigns and returns value as expression)
    if (match(TokenType::Let)) {
        if (check(TokenType::Identifier) && checkNext(TokenType::ColonEqual)) {
            std::string name = advance().lexeme;
            advance(); // consume ':='
            auto val = expression();
            return std::make_unique<WalrusExpr>(name, std::move(val));
        }
        // Not a walrus expression, backtrack
        current--;
    }

    // Lambda expression: |a, b| a + b  or  |a, b = 10| a + b
    if (match(TokenType::Pipe)) {
        std::vector<std::string> params;
        std::vector<std::unique_ptr<Expression>> defaults;

        if (!check(TokenType::Pipe)) {
            do {
                consume(TokenType::Identifier, "Expected parameter name in lambda.");
                params.push_back(tokens[current - 1].lexeme);
                if (match(TokenType::Assign)) {
                    // Use bitwise_xor() to avoid consuming '|' as bitwise OR
                    defaults.push_back(bitwise_xor());
                } else {
                    defaults.push_back(nullptr);
                }
            } while (match(TokenType::Comma));
        }

        consume(TokenType::Pipe, "Expected '|' after lambda parameters.");

        // Multi-line lambda: |params| { ... }
        if (match(TokenType::LBrace)) {
            auto block = blockStatement();
            auto lambdaExpr = std::make_unique<LambdaExpr>(std::move(params), std::move(block));
            lambdaExpr->parameterDefaults = std::move(defaults);
            return lambdaExpr;
        }

        auto body = expression();

        auto lambdaExpr = std::make_unique<LambdaExpr>(std::move(params), std::move(body));
        lambdaExpr->parameterDefaults = std::move(defaults);
        return lambdaExpr;
    }

    // Number literals with isInteger flag
    if (match(TokenType::Int)) {
        std::string numStr = tokens[current - 1].lexeme;
        double value;
        if (numStr.size() > 2 && numStr[0] == '0' && numStr[1] == 'b') {
            // Binary literal: 0b1010
            value = static_cast<double>(std::stoi(numStr.substr(2), nullptr, 2));
        } else if (numStr.size() > 2 && numStr[0] == '0' && numStr[1] == 'x') {
            // Hex literal: 0xFF
            value = static_cast<double>(std::stoi(numStr, nullptr, 16));
        } else {
            value = std::stod(numStr);
        }
        auto numExpr = std::make_unique<NumberExpr>(value, true);
        return finishAccessAndCall(std::move(numExpr));
    }
    if (match(TokenType::Float)) {
        std::string numStr = tokens[current - 1].lexeme;
        auto numExpr = std::make_unique<NumberExpr>(std::stod(numStr), false);
        return finishAccessAndCall(std::move(numExpr));
    }

    // Boolean literals
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);

    // None literal (null value)
    if (match(TokenType::None)) return std::make_unique<LiteralExpr>(Value());

    // String literals (with interpolation check)
    if (match(TokenType::String)) {
        std::string raw = tokens[current - 1].lexeme;
        if (raw.find("${") != std::string::npos) {
            auto interpExpr = std::make_unique<InterpolatedStringExpr>(raw);
            return finishAccessAndCall(std::move(interpExpr));
        }
        auto strExpr = std::make_unique<LiteralExpr>(Value(raw));
        return finishAccessAndCall(std::move(strExpr));
    }

    // Input expression: input<type>("prompt")
    if (match(TokenType::Input)) {
        std::string inputType = "string";
        if (match(TokenType::Less)) {
            Token typeToken = advance();
            if (typeToken.type == TokenType::Int || typeToken.type == TokenType::IntType) inputType = "int";
            else if (typeToken.type == TokenType::Float || typeToken.type == TokenType::FloatType) inputType = "float";
            else if (typeToken.type == TokenType::Bool || typeToken.type == TokenType::BoolType) inputType = "bool";
            else if (typeToken.type == TokenType::Str || typeToken.type == TokenType::StrType) inputType = "string";
            else error(typeToken, "Expected valid type after '<' in input.");
            consume(TokenType::Greater, "Expected '>' after type in input.");
        }
        consume(TokenType::LParen, "Expected '(' after 'input'.");
        if (!check(TokenType::String)) error(peek(), "Expected string as input prompt.");
        std::string prompt = advance().lexeme;
        consume(TokenType::RParen, "Expected ')' after input prompt.");
        return std::make_unique<InputExpr>(prompt, inputType);
    }

    // 'this' keyword
    if (match(TokenType::This)) {
        auto expr = std::make_unique<ThisExpr>();
        return finishAccessAndCall(std::move(expr));
    }

    // 'super' keyword: super.method(args)
    if (match(TokenType::Super)) {
        consume(TokenType::Dot, "Expected '.' after 'super'.");
        consume(TokenType::Identifier, "Expected method name after 'super.'.");
        std::string methodName = tokens[current - 1].lexeme;
        auto expr = std::make_unique<SuperExpr>(methodName);
        return finishAccessAndCall(std::move(expr));
    }

    // Identifiers and type keywords used as identifiers
    if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
        match(TokenType::Bool) || match(TokenType::Str)) {
        std::string name = tokens[current - 1].lexeme;
        std::unique_ptr<Expression> expr = std::make_unique<VariableExpr>(name);
        return finishAccessAndCall(std::move(expr));
    }

    // Parenthesized expression
    if (match(TokenType::LParen)) {
        auto expr = expression();
        consume(TokenType::RParen, "Expected ')' after expression.");
        return finishAccessAndCall(std::move(expr));
    }

    // List literal: [a, b, c] with optional spread: [a, ...b, c]
    // Also handles list comprehension: [expr for var in iterable] or [expr for var in iterable if condition]
    if (match(TokenType::LBracket)) {
        std::vector<std::unique_ptr<Expression>> elements;
        if (!check(TokenType::RBracket)) {
            // Parse first element (could be spread or regular expression)
            if (match(TokenType::DotDotDot)) {
                auto expr = expression();
                elements.push_back(std::make_unique<SpreadExpr>(std::move(expr)));
            } else {
                elements.push_back(expression());
            }

            // Check for list comprehension: [expr for var in iterable]
            if (elements.size() == 1 && !dynamic_cast<SpreadExpr*>(elements[0].get()) &&
                match(TokenType::For)) {
                auto body = std::move(elements[0]);
                consume(TokenType::Identifier, "Expected variable name after 'for' in list comprehension.");
                std::string varName = tokens[current - 1].lexeme;
                consume(TokenType::In, "Expected 'in' after variable in list comprehension.");
                auto iterable = expression();
                std::unique_ptr<Expression> condition = nullptr;
                if (match(TokenType::If)) {
                    condition = expression();
                }
                consume(TokenType::RBracket, "Expected ']' after list comprehension.");
                auto compExpr = std::make_unique<ListComprehensionExpr>(
                    std::move(body), varName, std::move(iterable), std::move(condition));
                return finishAccessAndCall(std::move(compExpr));
            }

            // Continue parsing remaining list elements
            while (match(TokenType::Comma)) {
                if (match(TokenType::DotDotDot)) {
                    auto expr = expression();
                    elements.push_back(std::make_unique<SpreadExpr>(std::move(expr)));
                } else {
                    elements.push_back(expression());
                }
            }
        }
        consume(TokenType::RBracket, "Expected ']' after list.");
        auto listExpr = std::make_unique<ListExpr>(std::move(elements));
        return finishAccessAndCall(std::move(listExpr));
    }

    // Map literal or block parsed as expression:
    // { key: value, ... }
    // We only reach here if statement() determined it was a map (or expressionStatement
    // is parsing and hits a '{').
    if (match(TokenType::LBrace)) {
        // Check if this is a map literal:
        // If empty {}, treat as empty map in expression context
        if (check(TokenType::RBrace)) {
            advance(); // consume '}'
            std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;
            return std::make_unique<MapExpr>(std::move(pairs));
        }

        // Check if first element followed by colon -> map literal
        bool isMap = false;
        if (current + 1 < tokens.size()) {
            TokenType firstType = tokens[current].type;
            TokenType secondType = tokens[current + 1].type;
            if ((firstType == TokenType::Identifier ||
                 firstType == TokenType::String ||
                 firstType == TokenType::Int ||
                 firstType == TokenType::Float) &&
                secondType == TokenType::Colon) {
                isMap = true;
            }
        }

        if (isMap) {
            // Parse first key:value pair
            auto firstKey = expression();
            consume(TokenType::Colon, "Expected ':' after map key.");
            auto firstValue = expression();

            // Check for map comprehension: {key: value for var in iterable}
            if (match(TokenType::For)) {
                consume(TokenType::Identifier, "Expected variable name after 'for' in map comprehension.");
                std::string varName = tokens[current - 1].lexeme;
                consume(TokenType::In, "Expected 'in' after variable in map comprehension.");
                auto iterable = expression();
                std::unique_ptr<Expression> condition = nullptr;
                if (match(TokenType::If)) {
                    condition = expression();
                }
                consume(TokenType::RBrace, "Expected '}' after map comprehension.");
                return std::make_unique<MapComprehensionExpr>(
                    std::move(firstKey), std::move(firstValue), varName,
                    std::move(iterable), std::move(condition));
            }

            // Regular map literal - continue parsing remaining pairs
            std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;
            pairs.emplace_back(std::move(firstKey), std::move(firstValue));
            while (match(TokenType::Comma)) {
                auto key = expression();
                consume(TokenType::Colon, "Expected ':' after map key.");
                auto value = expression();
                pairs.emplace_back(std::move(key), std::move(value));
            }
            consume(TokenType::RBrace, "Expected '}' after map literal.");
            return std::make_unique<MapExpr>(std::move(pairs));
        }

        // Not a map: this shouldn't normally happen in expression context,
        // but handle gracefully as a parse error.
        error(peek(), "Unexpected '{' in expression context.");
        throw std::runtime_error("Unexpected block in expression.");
    }

    error(peek(), "Expected expression.");
    throw std::runtime_error("Expected expression.");
}

// ============================================================================
// Access, call, index chaining
// ============================================================================

std::unique_ptr<Expression> Parser::finishAccessAndCall(std::unique_ptr<Expression> expr) {
    while (true) {
        if (match(TokenType::QuestionDot)) {
            consume(TokenType::Identifier, "Expected property name after '?.'.");
            auto field = previous().lexeme;
            expr = std::make_unique<OptionalGetExpr>(std::move(expr), field);
        } else if (match(TokenType::Dot)) {
            consume(TokenType::Identifier, "Expected property name after '.'.");
            auto field = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), field);
        } else if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> arguments;
            std::vector<std::string> argNames;
            if (!check(TokenType::RParen)) {
                do {
                    if (match(TokenType::DotDotDot)) {
                        auto arg = expression();
                        arguments.push_back(std::make_unique<SpreadExpr>(std::move(arg)));
                        argNames.push_back("");
                    } else if (check(TokenType::Identifier) && checkNext(TokenType::Colon)) {
                        // Named argument: name: value
                        std::string name = advance().lexeme;
                        advance(); // consume ':'
                        arguments.push_back(expression());
                        argNames.push_back(name);
                    } else {
                        arguments.push_back(expression());
                        argNames.push_back("");
                    }
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after arguments.");
            auto callNode = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
            // Only set argumentNames if there are any named args
            bool hasNamed = false;
            for (const auto& n : argNames) if (!n.empty()) { hasNamed = true; break; }
            if (hasNamed) callNode->argumentNames = std::move(argNames);
            expr = std::move(callNode);
        } else if (match(TokenType::LBracket)) {
            // Check for slice syntax: [start:end], [:end], [start:], [:]
            if (check(TokenType::Colon)) {
                // [:end] or [:]
                advance(); // consume ':'
                std::unique_ptr<Expression> endExpr = nullptr;
                if (!check(TokenType::RBracket)) {
                    endExpr = expression();
                }
                consume(TokenType::RBracket, "Expected ']' after slice.");
                expr = std::make_unique<SliceExpr>(std::move(expr), nullptr, std::move(endExpr));
            } else {
                auto startExpr = expression();
                if (match(TokenType::Colon)) {
                    // [start:end] or [start:]
                    std::unique_ptr<Expression> endExpr = nullptr;
                    if (!check(TokenType::RBracket)) {
                        endExpr = expression();
                    }
                    consume(TokenType::RBracket, "Expected ']' after slice.");
                    expr = std::make_unique<SliceExpr>(std::move(expr), std::move(startExpr), std::move(endExpr));
                } else {
                    // Regular index: [index]
                    consume(TokenType::RBracket, "Expected ']' after index.");
                    expr = std::make_unique<IndexExpr>(std::move(expr), std::move(startExpr));
                }
            }
        } else {
            break;
        }
    }
    return expr;
}

// ============================================================================
// Pattern parsing (for match statements)
// ============================================================================

std::unique_ptr<Pattern> Parser::pattern() {
    auto pat = orPattern();

    // Check for guard: pattern when condition
    if (peek().lexeme == "when") {
        advance(); // consume 'when'
        auto guard = expression();
        return std::make_unique<GuardedPattern>(std::move(pat), std::move(guard));
    }

    return pat;
}

// Or-patterns: pattern1 | pattern2 | pattern3
std::unique_ptr<Pattern> Parser::orPattern() {
    auto pat = primaryPattern();

    if (match(TokenType::Pipe)) {
        std::vector<std::unique_ptr<Pattern>> patterns;
        patterns.push_back(std::move(pat));

        do {
            patterns.push_back(primaryPattern());
        } while (match(TokenType::Pipe));

        return std::make_unique<OrPattern>(std::move(patterns));
    }

    return pat;
}

// Primary patterns
std::unique_ptr<Pattern> Parser::primaryPattern() {
    // Wildcard pattern: _
    if (match(TokenType::Underscore)) {
        return std::make_unique<WildcardPattern>();
    }

    // Number literal pattern (with optional range)
    if (match(TokenType::Int) || match(TokenType::Float)) {
        Token numToken = previous();
        bool isInt = (numToken.type == TokenType::Int);

        // Check for range pattern: number..number or number..=number
        if (match(TokenType::DotDot) || match(TokenType::DotDotEqual)) {
            bool inclusive = previous().type == TokenType::DotDotEqual;
            if (!match(TokenType::Int) && !match(TokenType::Float)) {
                error(peek(), "Expected end value in range pattern.");
            }
            double endValue = std::stod(previous().lexeme);
            double startValue = std::stod(numToken.lexeme);
            return std::make_unique<RangePattern>(
                Value(static_cast<int>(startValue)),
                Value(static_cast<int>(endValue)),
                inclusive
            );
        }

        if (isInt) {
            return std::make_unique<LiteralPattern>(Value(std::stoi(numToken.lexeme)));
        }
        return std::make_unique<LiteralPattern>(Value(std::stod(numToken.lexeme)));
    }

    // String literal pattern
    if (match(TokenType::String)) {
        std::string value = previous().lexeme;
        return std::make_unique<LiteralPattern>(Value(value));
    }

    // Boolean literal patterns
    if (match(TokenType::True)) {
        return std::make_unique<LiteralPattern>(Value(true));
    }
    if (match(TokenType::False)) {
        return std::make_unique<LiteralPattern>(Value(false));
    }

    // Tuple pattern: (pattern1, pattern2, ...)
    if (match(TokenType::LParen)) {
        std::vector<std::unique_ptr<Pattern>> patterns;

        if (!check(TokenType::RParen)) {
            do {
                patterns.push_back(pattern());
            } while (match(TokenType::Comma));
        }

        consume(TokenType::RParen, "Expected ')' after tuple pattern.");

        // Single element without comma is not a tuple
        if (patterns.size() == 1) {
            return std::move(patterns[0]);
        }

        return std::make_unique<TuplePattern>(std::move(patterns));
    }

    // Struct pattern or variable binding: Name or Name { field1, field2 }
    if (match(TokenType::Identifier)) {
        std::string name = previous().lexeme;

        // Check for struct pattern: Name { fields }
        if (match(TokenType::LBrace)) {
            std::vector<std::pair<std::string, std::unique_ptr<Pattern>>> fields;

            if (!check(TokenType::RBrace)) {
                do {
                    consume(TokenType::Identifier, "Expected field name in struct pattern.");
                    std::string fieldName = previous().lexeme;

                    std::unique_ptr<Pattern> fieldPattern;
                    if (match(TokenType::Colon)) {
                        fieldPattern = pattern();
                    } else {
                        // Shorthand: field binds to same name
                        fieldPattern = std::make_unique<VariablePattern>(fieldName);
                    }

                    fields.emplace_back(fieldName, std::move(fieldPattern));
                } while (match(TokenType::Comma));
            }

            consume(TokenType::RBrace, "Expected '}' after struct pattern.");
            return std::make_unique<StructPattern>(name, std::move(fields));
        }

        // Just a variable binding pattern
        return std::make_unique<VariablePattern>(name);
    }

    error(peek(), "Expected pattern.");
    return std::make_unique<WildcardPattern>(); // Fallback
}
