#include "parser.h"
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
    if (!match(type)) {
        throw std::runtime_error("Error at line " + std::to_string(peek().line) + ": " + message);
    }
}

std::vector<std::unique_ptr<Statement>> Parser::parse() {
    std::vector<std::unique_ptr<Statement>> statements;
    while (!check(TokenType::Eof)) {
        statements.push_back(statement());
    }
    return statements;
}

std::unique_ptr<Statement> Parser::statement() {
    if (match(TokenType::Struct)) return structStatement();
    if (match(TokenType::Print)) return printStatement();
    if (match(TokenType::Let)) return letStatement();
    if (match(TokenType::If)) return ifStatement();
    if (match(TokenType::Func)) return functionStatement();
    if (match(TokenType::Return)) return returnStatement();
    if (match(TokenType::LBrace)) return blockStatement();
    if (match(TokenType::For)) return forStatement();
    if (match(TokenType::While)) return whileStatement();
    if (match(TokenType::Break)) {
        consume(TokenType::Semicolon, "Expected ';' after 'break'.");
        return std::make_unique<BreakStmt>();
    }
    if (match(TokenType::Enum)) return enumStatement();
    if (match(TokenType::Match)) return matchStatement();
    if (match(TokenType::Switch)) return switchStatement();
    if (match(TokenType::Class)) return classStatement();
    if (match(TokenType::Continue)) {
        consume(TokenType::Semicolon, "Expected ';' after 'continue'.");
        return std::make_unique<ContinueStmt>();
    }

    // Now corrected:
    if (check(TokenType::Identifier)) {
        std::string name = peek().lexeme;
        advance(); // consume the name

        // If comes ( ), then it's a function call, not just a variable
        if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after function arguments.");
            consume(TokenType::Semicolon, "Expected ';' after function call.");
            return std::make_unique<ExpressionStmt>(
                std::make_unique<CallExpr>(
                    std::make_unique<VariableExpr>(name),
                    std::move(args)
                )
            );
        }

        // If it is an indexing operation like p["name"]
        if (match(TokenType::LBracket)) {
            auto index = expression();
            consume(TokenType::RBracket, "Expected ']' after index.");

            if (match(TokenType::Assign)) {
                auto value = expression();
                consume(TokenType::Semicolon, "Expected ';' after assignment.");
                return std::make_unique<IndexAssignStmt>(
                    std::make_unique<VariableExpr>(name),
                    std::move(index),
                    std::move(value)
                );
            } else {
                auto expr = std::make_unique<IndexExpr>(
                    std::make_unique<VariableExpr>(name),
                    std::move(index)
                );
                consume(TokenType::Semicolon, "Expected ';' after expression.");
                return std::make_unique<ExpressionStmt>(std::move(expr));
            }
        }

        // If it's a normal assignment or a property assignment
        if (match(TokenType::Assign)) {
            auto value = expression();
            consume(TokenType::Semicolon, "Expected ';' after expression.");

            // If previous indexation like p["name"]
            if (match(TokenType::LBracket)) {
                auto index = expression();
                consume(TokenType::RBracket, "Expected ']' after index.");
                return std::make_unique<SetStmt>(
                    std::make_unique<VariableExpr>(name),
                    std::move(index),
                    std::move(value)
                );
            }

            // Normal variable assignment
            return std::make_unique<AssignStmt>(name, std::move(value));
        }

        // Isolated variable (not a function call)
        auto expr = std::make_unique<VariableExpr>(name);
        consume(TokenType::Semicolon, "Expected ';' after expression.");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    throw std::runtime_error("Expected identifier or expression.");
}

std::unique_ptr<Statement> Parser::printStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after print.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Statement> Parser::assignStatement() {
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::Assign, "Expected '=' after identifier.");
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<AssignStmt>(name, std::move(expr));
}

std::unique_ptr<Statement> Parser::letStatement() {
    consume(TokenType::Identifier, "Expected variable name after 'let'.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::Assign, "Expected '=' after variable name.");
    auto initializer = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<LetStmt>(name, std::move(initializer));
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
    if (!check(TokenType::RParen)) {
        do {
            consume(TokenType::Identifier, "Expected parameter name.");
            params.push_back(tokens[current - 1].lexeme);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RParen, "Expected ')' after parameters.");
    consume(TokenType::LBrace, "Expected '{' to open the function body.");
    auto body = blockStatement();
    return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
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
    auto expr = term();
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
        auto right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), binOp, std::move(right));
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

    std::vector<std::pair<std::string, std::unique_ptr<Statement>>> arms;

    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        std::string value;
        consume(TokenType::Identifier, "Expected enum value name.");
        value = tokens[current - 1].lexeme;
        
        while (match(TokenType::Dot)) {
            consume(TokenType::Identifier, "Expected field name after '.'.");
            std::string fieldName = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), fieldName);
        }
        consume(TokenType::Arrow, "Expected '=>' after value.");
        auto body = statement();

        arms.emplace_back(value, std::move(body));
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
            throw std::runtime_error("Expected 'case' or 'default'.");
        }
    }

    consume(TokenType::RBrace, "Expected '}' at the end of switch.");
    return std::make_unique<SwitchStmt>(std::move(expr), std::move(cases), std::move(defaultCase));
}

std::unique_ptr<Expression> Parser::primary() {
    if (match(TokenType::Number)) {
        std::string numStr = tokens[current - 1].lexeme;
        if (numStr.find('.') != std::string::npos) {
            return std::make_unique<LiteralExpr>(Value(std::stod(numStr)));
        } else {
            return std::make_unique<LiteralExpr>(Value(std::stoi(numStr)));
        }
    }
    if (match(TokenType::True)) return std::make_unique<LiteralExpr>(true);
    if (match(TokenType::False)) return std::make_unique<LiteralExpr>(false);


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
            else throw std::runtime_error("Expected valid type after '<' in input.");
            consume(TokenType::Greater, "Expected '>' after type.");
        }
        consume(TokenType::LParen, "Expected '(' after input.");
        if (!check(TokenType::String)) throw std::runtime_error("Expected string as input prompt.");
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
        while (true) {
            if (match(TokenType::Dot)) {
                consume(TokenType::Identifier, "Expected field name after '.'");
                auto fieldName = tokens[current - 1].lexeme;
                auto fieldExpr = std::make_unique<LiteralExpr>(fieldName);
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(fieldExpr));
            } else if (match(TokenType::LBracket)) {
                auto index = expression();
                consume(TokenType::RBracket, "Expected ']' after index.");
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
            } else {
                break;
            }
        }
        // Function call support
        if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expected ')' after arguments.");
            auto callee = std::make_unique<VariableExpr>(name);
            return std::make_unique<CallExpr>(std::move(callee), std::move(args));
        }

        // Support for p["name"] type indexing
        while (match(TokenType::LBracket)) {
            auto index = expression();
            consume(TokenType::RBracket, "Expected ']' after index.");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        }

        return expr;
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
    throw std::runtime_error("Invalid primary expression in line " + std::to_string(peek().line));
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
            throw std::runtime_error("Expected 'func' or 'let' inside the class.");
        }
    }

    consume(TokenType::RBrace, "Expected '}' to close the class.");
    return std::make_unique<ClassStmt>(className, std::move(fields), std::move(methods));
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

bool Parser::isAtEnd() {
    return current >= tokens.size();
}
