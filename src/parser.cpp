#include "yen/parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

const Token& Parser::peek() {
    return tokens[current];
}

const Token& Parser::advance() {
    if (current < tokens.size()) current++;
    return tokens[current - 1];
}

bool Parser::check(TokenType type) {
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
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Print:
            case TokenType::Return:
                return;
            default:
                // Do nothing.
                ;
        }
        advance();
    }
}

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

std::unique_ptr<Statement> Parser::statement() {
    if (match(TokenType::Struct)) return structStatement();
    if (match(TokenType::Extern)) return externBlock();
    if (match(TokenType::Print)) return printStatement();
    if (match(TokenType::Let)) return letStatement(false);  // immutable
    if (match(TokenType::Var)) return letStatement(true);   // mutable
    if (match(TokenType::Const)) return constStatement();
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::Func)) return functionStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::LBrace)) return blockStatement();
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::Loop)) return loopStatement();
    if (match(TokenType::Defer)) return deferStatement();
    if (match(TokenType::Assert)) return assertStatement();
    if (match(TokenType::Break)) {
        consume(TokenType::Semicolon, "Expected ';' after 'break'.");
        return std::make_unique<BreakStmt>();
    }
    if (match(TokenType::Enum)) return enumStatement();
    if (match(TokenType::Match)) return matchStatement();
    if (match(TokenType::Switch)) return switchStatement();
    if (match(TokenType::Class)) return classStatement();
    if (match(TokenType::Import)) return importStatement();
    if (match(TokenType::Export)) return exportStatement();
    if (match(TokenType::Continue)) {
        consume(TokenType::Semicolon, "Expected ';' after 'continue'.");
        return std::make_unique<ContinueStmt>();
    }

    return expressionStatement();
}

std::unique_ptr<Statement> Parser::printStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after print.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Statement> Parser::expressionStatement() {
    auto expr = expression(); // Parse any expression
    
    // Check if it's an assignment
    if (match(TokenType::Assign)) {
        auto value = expression(); // Parse the right-hand side of the assignment
        consume(TokenType::Semicolon, "Expected ';' after assignment.");

        // Determine the type of the left-hand side expression
        if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
            // It's a simple variable assignment: variable = value;
            return std::make_unique<AssignStmt>(varExpr->name, std::move(value));
        } else if (auto getExpr = dynamic_cast<GetExpr*>(expr.get())) {
            // It's a member assignment: object.field = value;
            return std::make_unique<SetStmt>(std::move(getExpr->object), std::make_unique<LiteralExpr>(Value(getExpr->name)), std::move(value));
        } else if (auto indexExpr = dynamic_cast<IndexExpr*>(expr.get())) {
            // It's an index assignment: list[index] = value;
            return std::make_unique<IndexAssignStmt>(std::move(indexExpr->listExpr), std::move(indexExpr->indexExpr), std::move(value));
        } else {
            error(previous(), "Invalid assignment target.");
            throw std::runtime_error("Invalid assignment target.");
        }
    }

    // If not an assignment, it's just an expression followed by a semicolon
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

std::unique_ptr<Statement> Parser::letStatement(bool isMutable) {
    consume(TokenType::Identifier, "Expected variable name after 'let' or 'var'.");
    std::string name = tokens[current - 1].lexeme;

    // Optional type annotation: let x: int32 = 10;
    std::optional<std::string> typeAnnotation;
    if (match(TokenType::Colon)) {
        // Accept identifier or type keywords (int, float, bool, str, etc.)
        if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
            match(TokenType::Bool) || match(TokenType::Str)) {
            typeAnnotation = tokens[current - 1].lexeme;
        } else {
            error(peek(), "Expected type name after ':'.");
        }
    }

    consume(TokenType::Assign, "Expected '=' after variable name.");
    auto initializer = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
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
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<ConstStmt>(name, std::move(initializer), typeAnnotation);
}

std::unique_ptr<Statement> Parser::loopStatement() {
    // Parse: loop { ... }
    consume(TokenType::LBrace, "Expected '{' after 'loop'.");
    auto body = blockStatement();
    return std::make_unique<LoopStmt>(std::move(body));
}

std::unique_ptr<Statement> Parser::deferStatement() {
    // Parse: defer statement;
    auto stmt = statement();
    return std::make_unique<DeferStmt>(std::move(stmt));
}

std::unique_ptr<Statement> Parser::assertStatement() {
    // Parse: assert(condition) or assert(condition, "message")
    consume(TokenType::LParen, "Expected '(' after 'assert'.");
    auto condition = expression();

    std::string message;
    if (match(TokenType::Comma)) {
        if (!match(TokenType::String)) {
            error(peek(), "Expected string message after comma in assert.");
        }
        message = tokens[current - 1].lexeme;
    }

    consume(TokenType::RParen, "Expected ')' after assert condition.");
    consume(TokenType::Semicolon, "Expected ';' after assert.");

    return std::make_unique<AssertStmt>(std::move(condition), message, false);
}

std::unique_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::LParen, "Expected '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after condition.");
    auto thenBranch = statement();
    std::unique_ptr<Statement> elseBranch = nullptr;
    if (match(TokenType::Else)) {
        elseBranch = statement();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

std::unique_ptr<Statement> Parser::blockStatement() {
    std::vector<std::unique_ptr<Statement>> stmts;
    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        stmts.push_back(statement());
    }
    consume(TokenType::RBrace, "Expected '}' to close the block.");
    return std::make_unique<BlockStmt>(std::move(stmts));
}

std::unique_ptr<Statement> Parser::functionStatement() {
    consume(TokenType::Identifier, "Expected function name.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::LParen, "Expected '(' after function name.");

    std::vector<std::string> params;
    std::vector<std::string> paramTypes;

    if (!check(TokenType::RParen)) {
        do {
            consume(TokenType::Identifier, "Expected parameter name.");
            params.push_back(tokens[current - 1].lexeme);

            // Optional type annotation: func add(a: int32, b: int32)
            if (match(TokenType::Colon)) {
                // Accept identifier or type keywords (int, float, bool, str, etc.)
                if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                    match(TokenType::Bool) || match(TokenType::Str)) {
                    paramTypes.push_back(tokens[current - 1].lexeme);
                } else {
                    error(peek(), "Expected type name after ':'.");
                    paramTypes.push_back(""); // Fallback
                }
            } else {
                paramTypes.push_back(""); // No type annotation
            }
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RParen, "Expected ')' after parameters.");

    // Optional return type annotation: func add(...) -> int32 { ... }
    std::string returnType = "";
    if (match(TokenType::Minus)) {
        if (match(TokenType::Greater)) {
            // We have '->' for return type
            // Accept identifier or type keywords
            if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                match(TokenType::Bool) || match(TokenType::Str)) {
                returnType = tokens[current - 1].lexeme;
            } else {
                error(peek(), "Expected return type after '->'.");
            }
        } else {
            error(peek(), "Expected '>' after '-' for return type annotation.");
        }
    }

    consume(TokenType::LBrace, "Expected '{' to open the function body.");
    auto body = blockStatement();
    return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body), std::move(paramTypes), returnType);
}

std::unique_ptr<Statement> Parser::externBlock() {
    // extern "C" { ... }
    // Parse ABI string
    consume(TokenType::String, "Expected ABI string after 'extern' (e.g., \"C\")");
    std::string abi = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Expected '{' after extern ABI");

    std::vector<std::unique_ptr<ExternFunctionDecl>> functions;

    // Parse function declarations
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        consume(TokenType::Func, "Expected 'func' in extern block");
        consume(TokenType::Identifier, "Expected function name");
        std::string funcName = tokens[current - 1].lexeme;

        consume(TokenType::LParen, "Expected '(' after function name");

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
                        error(peek(), "Expected '...' for varargs");
                    }
                }

                consume(TokenType::Identifier, "Expected parameter name");
                params.push_back(tokens[current - 1].lexeme);

                // Type annotation required in extern
                consume(TokenType::Colon, "Expected ':' after parameter name in extern function");
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
                        // Get the pointee type
                        if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) ||
                            match(TokenType::Bool) || match(TokenType::Str)) {
                            type += tokens[current - 1].lexeme;
                        } else {
                            error(peek(), "Expected type after pointer");
                        }
                    }

                    paramTypes.push_back(type);
                } else {
                    error(peek(), "Expected type name");
                    paramTypes.push_back("int32"); // Fallback
                }
            } while (match(TokenType::Comma));
        }

        consume(TokenType::RParen, "Expected ')' after parameters");

        // Return type
        std::string returnType = "void";
        if (match(TokenType::Minus) && match(TokenType::Greater)) {
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

        consume(TokenType::Semicolon, "Expected ';' after extern function declaration");

        functions.push_back(std::make_unique<ExternFunctionDecl>(
            funcName, std::move(params), std::move(paramTypes), returnType, isVarArg
        ));
    }

    consume(TokenType::RBrace, "Expected '}' to close extern block");

    return std::make_unique<ExternBlock>(abi, std::move(functions));
}

std::unique_ptr<Statement> Parser::returnStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Esperado ';' após return.");
    return std::make_unique<ReturnStmt>(std::move(expr));
}

// EXPRESSIONS
std::unique_ptr<Expression> Parser::expression() { return logic_or(); }
std::unique_ptr<Expression> Parser::logic_or() {
    auto expr = logic_and();
    while (match(TokenType::OrOr)) {
        auto right = logic_and();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::Or, std::move(right));
    }
    return expr;
}
std::unique_ptr<Expression> Parser::logic_and() {
    auto expr = equality();
    while (match(TokenType::AndAnd)) {
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), BinaryOp::And, std::move(right));
    }
    return expr;
}
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
std::unique_ptr<Expression> Parser::comparison() {
    auto expr = cast();
    while (match(TokenType::Less) || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        Token op = tokens[current - 1];
        BinaryOp binOp = BinaryOp::Add;
        switch (op.type) {
            case TokenType::Less: binOp = BinaryOp::Less; break;
            case TokenType::LessEqual: binOp = BinaryOp::LessEqual; break;
            case TokenType::Greater: binOp = BinaryOp::Greater; break;
            case TokenType::GreaterEqual: binOp = BinaryOp::GreaterEqual; break;
            default: break;
        }
        auto right = cast();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::cast() {
    auto expr = range();

    // Handle cast operator: expr as Type
    while (match(TokenType::As)) {
        // Parse the target type
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

std::unique_ptr<Expression> Parser::range() {
    auto expr = term();

    // Handle range operators: .. and ..=
    if (match(TokenType::DotDot) || match(TokenType::DotDotEqual)) {
        bool inclusive = tokens[current - 1].type == TokenType::DotDotEqual;
        auto end = term();
        return std::make_unique<RangeExpr>(std::move(expr), std::move(end), inclusive);
    }

    return expr;
}
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
std::unique_ptr<Expression> Parser::factor() {
    auto expr = unary();
    while (match(TokenType::Star) || match(TokenType::Slash)) {
        Token op = tokens[current - 1];
        BinaryOp binOp = (op.type == TokenType::Star) ? BinaryOp::Mul : BinaryOp::Div;
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
    }
    return expr;
}
std::unique_ptr<Expression> Parser::unary() {
    if (match(TokenType::Not)) {
        auto right = unary();
        return std::make_unique<UnaryExpr>(UnaryOp::Not, std::move(right));
    }
    return primary();
}
std::unique_ptr<Statement> Parser::forStatement() {
    consume(TokenType::Identifier, "Expected variable name in for loop.");
    std::string varName = tokens[current - 1].lexeme;
    consume(TokenType::In, "Expected 'in' after variable name.");
    auto iterable = expression();
    consume(TokenType::LBrace, "Expected '{' to start for loop body.");
    auto body = blockStatement();
    return std::make_unique<ForStmt>(varName, std::move(iterable), std::move(body));
}

std::unique_ptr<Statement> Parser::whileStatement() {
    consume(TokenType::LParen, "Expected '(' after while.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after condition.");
    auto body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::enumStatement() {
    consume(TokenType::Identifier, "Expected enum name.");
    std::string enumName = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Expected '{' after enum name.");

    std::vector<std::string> values;
    do {
        consume(TokenType::Identifier, "Expected identifier inside enum.");
        values.push_back(tokens[current - 1].lexeme);
    } while (match(TokenType::Comma));

    consume(TokenType::RBrace, "Expected '}' after enum values.");
    return std::make_unique<EnumStmt>(enumName, values);
}

std::unique_ptr<Statement> Parser::matchStatement() {
    consume(TokenType::LParen, "Expected '(' after 'match'.");
    auto expr = expression();
    consume(TokenType::RParen, "Expected ')' after expression.");

    consume(TokenType::LBrace, "Expected '{' after 'match (...)'.");

    std::vector<MatchArm> arms;

    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        // Parse pattern
        auto pat = pattern();

        // Expect =>
        consume(TokenType::Arrow, "Expected '=>' after pattern.");

        // Parse body
        auto body = statement();

        // Create match arm
        arms.emplace_back(std::move(pat), std::move(body));
    }

    consume(TokenType::RBrace, "Expected '}' at the end of match.");
    return std::make_unique<MatchStmt>(std::move(expr), std::move(arms));
}

std::unique_ptr<Statement> Parser::switchStatement() {
    consume(TokenType::LParen, "Expected '(' after 'switch'.");
    auto expr = expression();
    consume(TokenType::RParen, "Expected ')' after expression.");

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
            consume(TokenType::Arrow, "Expected '=>' after default.");
            defaultCase = statement();
        } else {
            error(peek(), "Expected 'case' or 'default'.");
        }
    }

    consume(TokenType::RBrace, "Expected '}' at the end of switch.");
    return std::make_unique<SwitchStmt>(std::move(expr), std::move(cases), std::move(defaultCase));
}

std::unique_ptr<Expression> Parser::primary() {
    // Lambda expression: |a, b| a + b
    if (match(TokenType::Pipe)) {
        std::vector<std::string> params;

        // Parse parameters
        if (!check(TokenType::Pipe)) {
            do {
                consume(TokenType::Identifier, "Expected parameter name in lambda.");
                params.push_back(tokens[current - 1].lexeme);
            } while (match(TokenType::Comma));
        }

        consume(TokenType::Pipe, "Expected '|' after lambda parameters.");

        // Parse body (single expression for now)
        auto body = expression();

        return std::make_unique<LambdaExpr>(std::move(params), std::move(body));
    }

    if (match(TokenType::Float) || match(TokenType::Int)) {
        std::string numStr = tokens[current - 1].lexeme;
        return std::make_unique<NumberExpr>(std::stod(numStr));
    }
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);
    // Null removed - use Option<T> instead
    // if (match(TokenType::Null)) return std::make_unique<LiteralExpr>(Value());

    if (match(TokenType::String)) {
        std::string raw = tokens[current - 1].lexeme;
        if (raw.find("${") != std::string::npos) {
            return std::make_unique<InterpolatedStringExpr>(raw);
        }
        return std::make_unique<LiteralExpr>(Value(raw));
    }


    if (match(TokenType::Input)) {
        std::string inputType = "string";
        if (match(TokenType::Less)) {
            Token typeToken = advance();
            if (typeToken.type == TokenType::Int || typeToken.type == TokenType::IntType) inputType = "int";
            else if (typeToken.type == TokenType::Float || typeToken.type == TokenType::FloatType) inputType = "float";
            else if (typeToken.type == TokenType::Bool || typeToken.type == TokenType::BoolType) inputType = "bool";
            else if (typeToken.type == TokenType::Str || typeToken.type == TokenType::StrType) inputType = "string";
            else error(typeToken, "Expected valid type after '<' in input.");
            consume(TokenType::Greater, "Expected '>' after type.");
        }
        consume(TokenType::LParen, "Expected '(' after input.");
        if (!check(TokenType::String)) error(peek(), "Expected string as input prompt.");
        std::string prompt = advance().lexeme;
        consume(TokenType::RParen, "Expected ')' after prompt.");
        return std::make_unique<InputExpr>(prompt, inputType);
    }

    if (match(TokenType::This)) {
        auto expr = std::make_unique<ThisExpr>();
        return finishAccessAndCall(std::move(expr));
    }
    if (match(TokenType::Identifier) || match(TokenType::Int) || match(TokenType::Float) || match(TokenType::Bool) || match(TokenType::Str)) {
        std::string name = tokens[current - 1].lexeme;
        std::unique_ptr<Expression> expr = std::make_unique<VariableExpr>(name);
        return finishAccessAndCall(std::move(expr)); // Use the helper function for chaining
    }

    if (match(TokenType::LParen)) {
        auto expr = expression();
        consume(TokenType::RParen, "Expected ')' after expression.");
        return expr;
    }

    if (match(TokenType::LBracket)) {
        std::vector<std::unique_ptr<Expression>> elements;
        if (!check(TokenType::RBracket)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RBracket, "Expected ']' after list.");
        return std::make_unique<ListExpr>(std::move(elements));
    }
    std::unique_ptr<Expression> expr;

    if (match(TokenType::This)) {
        expr = std::make_unique<ThisExpr>();
    
        while (match(TokenType::Dot)) {  // <<< detect dot
            consume(TokenType::Identifier, "Expected field name after '.'");
            std::string fieldName = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), fieldName); 
        }
        return expr;
    }    
    error(peek(), "Invalid primary expression.");
    return nullptr;
}


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

    consume(TokenType::RBrace, "Expected '}' to close the struct.");
    return std::make_unique<StructStmt>(name, std::move(fields));
}

std::unique_ptr<Statement> Parser::classStatement() {
    consume(TokenType::Identifier, "Expected class name.");
    std::string className = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Expected '{' after class name.");

    std::vector<std::string> fields;
    std::vector<std::unique_ptr<FunctionStmt>> methods;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        if (match(TokenType::Func)) {
            // method
            auto method = functionStatement();
            methods.push_back(std::unique_ptr<FunctionStmt>(static_cast<FunctionStmt*>(method.release())));
        } else if (match(TokenType::Let)) { 
            consume(TokenType::Identifier, "Expected field name.");
            fields.push_back(tokens[current - 1].lexeme);
            consume(TokenType::Semicolon, "Expected ';' after field.");
        } else {
            error(peek(), "Expected 'func' or 'let' inside the class.");
        }
    }

    consume(TokenType::RBrace, "Expected '}' to close the class.");
    return std::make_unique<ClassStmt>(className, std::move(fields), std::move(methods));
}

std::unique_ptr<Statement> Parser::importStatement() {
    consume(TokenType::String, "Expected module path in string.");
    std::string path = tokens[current - 1].lexeme;
    consume(TokenType::Semicolon, "Expected ';' after import path.");
    return std::make_unique<ImportStmt>(path);
}

std::unique_ptr<Statement> Parser::exportStatement() {
    auto stmt = statement();
    return std::make_unique<ExportStmt>(std::move(stmt));
}

std::unique_ptr<Expression> Parser::finishAccessAndCall(std::unique_ptr<Expression> expr) {
    while (true) {
        if (match(TokenType::Dot)) {
            consume(TokenType::Identifier, "Expected name after '.'.");
            auto field = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), field);
        } else if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> arguments;
            if (!check(TokenType::RParen)) {
                do {
                    arguments.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after arguments.");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
        } else if (match(TokenType::LBracket)) {
            auto index = expression();
            consume(TokenType::RBracket, "Expected ']' after index.");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        } else {
            break;
        }
    }
    return expr;
}

// ========== Pattern Parsing ==========

// Parse a pattern with optional guard
std::unique_ptr<Pattern> Parser::pattern() {
    auto pat = orPattern();

    // Check for guard: pattern when condition
    if (peek().lexeme == "when") {
        advance();  // consume 'when'
        auto guard = expression();
        return std::make_unique<GuardedPattern>(std::move(pat), std::move(guard));
    }

    return pat;
}

// Parse or-patterns: pattern1 | pattern2 | pattern3
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

// Parse primary patterns
std::unique_ptr<Pattern> Parser::primaryPattern() {
    // Wildcard pattern: _
    if (match(TokenType::Underscore)) {
        return std::make_unique<WildcardPattern>();
    }

    // Number literal pattern
    if (match(TokenType::Int) || match(TokenType::Float)) {
        double value = std::stod(previous().lexeme);
        // Check if it's a range: number..number or number..=number
        if (match(TokenType::DotDot) || match(TokenType::DotDotEqual)) {
            bool inclusive = previous().type == TokenType::DotDotEqual;
            if (!match(TokenType::Int) && !match(TokenType::Float)) {
                error(peek(), "Expected end value in range pattern.");
            }
            double endValue = std::stod(previous().lexeme);
            return std::make_unique<RangePattern>(
                Value(static_cast<int>(value)),
                Value(static_cast<int>(endValue)),
                inclusive
            );
        }
        // Just a number literal
        return std::make_unique<LiteralPattern>(Value(value));
    }

    // String literal pattern
    if (match(TokenType::String)) {
        std::string value = previous().lexeme;
        return std::make_unique<LiteralPattern>(Value(value));
    }

    // Boolean literal pattern
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

        // If single element without comma, not a tuple
        if (patterns.size() == 1) {
            return std::move(patterns[0]);
        }

        return std::make_unique<TuplePattern>(std::move(patterns));
    }

    // Struct pattern or variable pattern: Name or Name { field1, field2 }
    if (match(TokenType::Identifier)) {
        std::string name = previous().lexeme;

        // Check for struct pattern: Name { fields }
        if (match(TokenType::LBrace)) {
            std::vector<std::pair<std::string, std::unique_ptr<Pattern>>> fields;

            if (!check(TokenType::RBrace)) {
                do {
                    consume(TokenType::Identifier, "Expected field name in struct pattern.");
                    std::string fieldName = previous().lexeme;

                    // Optional colon for explicit binding: field: pattern
                    std::unique_ptr<Pattern> fieldPattern;
                    if (match(TokenType::Colon)) {
                        fieldPattern = pattern();
                    } else {
                        // Shorthand: field is same as field: field
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
    return std::make_unique<WildcardPattern>();  // Fallback
}

bool Parser::isAtEnd() {
    return peek().type == TokenType::Eof;
}
