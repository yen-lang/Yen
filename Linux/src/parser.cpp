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
        throw std::runtime_error("Erro na linha " + std::to_string(peek().line) + ": " + message);
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
        consume(TokenType::Semicolon, "Esperado ';' após 'break'.");
        return std::make_unique<BreakStmt>();
    }
    if (match(TokenType::Enum)) return enumStatement();
    if (match(TokenType::Match)) return matchStatement();
    if (match(TokenType::Switch)) return switchStatement();
    if (match(TokenType::Class)) return classStatement();
    if (match(TokenType::Continue)) {
        consume(TokenType::Semicolon, "Esperado ';' após 'continue'.");
        return std::make_unique<ContinueStmt>();
    }

    // Agora corrigido:
    if (check(TokenType::Identifier)) {
        std::string name = peek().lexeme;
        advance(); // consome o nome
    
        // >>>> AQUI: se vier ( ) então é chamada de função, não variável
        if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Esperado ')' após argumentos da função.");
            consume(TokenType::Semicolon, "Esperado ';' após chamada de função.");
            return std::make_unique<ExpressionStmt>(
                std::make_unique<CallExpr>(
                    std::make_unique<VariableExpr>(name), // <- Wrappa name aqui
                    std::move(args)
                )
            );
        }
    
        // Se for indexação tipo p["nome"]
        if (match(TokenType::LBracket)) {
            auto index = expression();
            consume(TokenType::RBracket, "Esperado ']'.");
    
            if (match(TokenType::Assign)) {
                auto value = expression();
                consume(TokenType::Semicolon, "Esperado ';' no final da atribuição.");
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
                consume(TokenType::Semicolon, "Esperado ';' após expressão.");
                return std::make_unique<ExpressionStmt>(std::move(expr));
            }
        }
    
        // Se for atribuição normal ou atribuição em propriedade
        if (match(TokenType::Assign)) {
            auto value = expression();
            consume(TokenType::Semicolon, "Esperado ';' após expressão.");

            // Se veio indexação anterior (tipo p["nome"])
            if (match(TokenType::LBracket)) {
                auto index = expression();
                consume(TokenType::RBracket, "Esperado ']' após índice.");
                return std::make_unique<SetStmt>(
                    std::make_unique<VariableExpr>(name),
                    std::move(index),
                    std::move(value)
                );
            }

            // Atribuição normal de variável
            return std::make_unique<AssignStmt>(name, std::move(value));
        }

    
        // Variável isolada (não chamada)
        auto expr = std::make_unique<VariableExpr>(name);
        consume(TokenType::Semicolon, "Esperado ';' após expressão.");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }
    
    throw std::runtime_error("Esperado identificador ou expressão.");
}

std::unique_ptr<Statement> Parser::printStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Esperado ';' após print.");
    return std::make_unique<PrintStmt>(std::move(expr));
}

std::unique_ptr<Statement> Parser::assignStatement() {
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::Assign, "Esperado '=' após identificador.");
    auto expr = expression();
    consume(TokenType::Semicolon, "Esperado ';' após expressão.");
    return std::make_unique<AssignStmt>(name, std::move(expr));
}

std::unique_ptr<Statement> Parser::letStatement() {
    consume(TokenType::Identifier, "Esperado nome da variável após 'let'.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::Assign, "Esperado '=' após o nome da variável.");
    auto initializer  = expression();
    consume(TokenType::Semicolon, "Esperado ';' após a expressão.");
    return std::make_unique<LetStmt>(name, std::move(initializer    ));
}

std::unique_ptr<Statement> Parser::ifStatement() {
    consume(TokenType::LParen, "Esperado '(' após 'if'.");
    auto condition = expression();
    consume(TokenType::RParen, "Esperado ')' após condição.");
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
    consume(TokenType::RBrace, "Esperado '}' para fechar o bloco.");
    return std::make_unique<BlockStmt>(std::move(stmts));
}

std::unique_ptr<Statement> Parser::functionStatement() {
    consume(TokenType::Identifier, "Esperado nome da função.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::LParen, "Esperado '(' após nome da função.");

    std::vector<std::string> params;
    if (!check(TokenType::RParen)) {
        do {
            consume(TokenType::Identifier, "Esperado nome do parâmetro.");
            params.push_back(tokens[current - 1].lexeme);
        } while (match(TokenType::Comma));
    }

    consume(TokenType::RParen, "Esperado ')' após parâmetros.");
    consume(TokenType::LBrace, "Esperado '{' para abrir o corpo da função.");
    auto body = blockStatement();
    return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
}

std::unique_ptr<Statement> Parser::returnStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Esperado ';' após return.");
    return std::make_unique<ReturnStmt>(std::move(expr));
}

// EXPRESSÕES
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
    consume(TokenType::Identifier, "Esperado nome da variável no for.");
    std::string varName = tokens[current - 1].lexeme;
    consume(TokenType::In, "Esperado 'in' após nome.");
    auto iterable = expression();
    consume(TokenType::LBrace, "Esperado '{' para corpo do for.");
    auto body = blockStatement();
    return std::make_unique<ForStmt>(varName, std::move(iterable), std::move(body));
}
std::unique_ptr<Statement> Parser::whileStatement() {
    consume(TokenType::LParen, "Esperado '(' após while.");
    auto condition = expression();
    consume(TokenType::RParen, "Esperado ')' após condição.");
    auto body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::enumStatement() {
    consume(TokenType::Identifier, "Esperado nome do enum.");
    std::string enumName = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Esperado '{' após nome do enum.");

    std::vector<std::string> values;
    do {
        consume(TokenType::Identifier, "Esperado identificador no enum.");
        values.push_back(tokens[current - 1].lexeme);
    } while (match(TokenType::Comma));

    consume(TokenType::RBrace, "Esperado '}' após valores do enum.");
    return std::make_unique<EnumStmt>(enumName, values);
}

std::unique_ptr<Statement> Parser::matchStatement() {
    consume(TokenType::LParen, "Esperado '(' após 'match'.");
    auto expr = expression();
    consume(TokenType::RParen, "Esperado ')' após expressão.");

    consume(TokenType::LBrace, "Esperado '{' após 'match (...)'.");

    std::vector<std::pair<std::string, std::unique_ptr<Statement>>> arms;

    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        std::string value;
        consume(TokenType::Identifier, "Esperado nome do valor de enum.");
        value = tokens[current - 1].lexeme;
        
        while (match(TokenType::Dot)) {
            consume(TokenType::Identifier, "Esperado nome do campo após '.'.");
            std::string fieldName = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), fieldName);
        }  
        consume(TokenType::Arrow, "Esperado '=>' após o valor.");
        auto body = statement();

        arms.emplace_back(value, std::move(body));
    }

    consume(TokenType::RBrace, "Esperado '}' ao final do match.");
    return std::make_unique<MatchStmt>(std::move(expr), std::move(arms));
}

std::unique_ptr<Statement> Parser::switchStatement() {
    consume(TokenType::LParen, "Esperado '(' após 'switch'.");
    auto expr = expression();
    consume(TokenType::RParen, "Esperado ')' após expressão.");

    consume(TokenType::LBrace, "Esperado '{' após 'switch (...)'.");

    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>> cases;
    std::unique_ptr<Statement> defaultCase = nullptr;

    while (!check(TokenType::RBrace) && !check(TokenType::Eof)) {
        if (match(TokenType::Case)) {
            auto val = expression();
            consume(TokenType::Arrow, "Esperado '=>' após valor.");
            auto body = statement();
            cases.emplace_back(std::move(val), std::move(body));
        } else if (match(TokenType::Default)) {
            consume(TokenType::Arrow, "Esperado '=>' após default.");
            defaultCase = statement();
        } else {
            throw std::runtime_error("Esperado 'case' ou 'default'.");
        }
    }

    consume(TokenType::RBrace, "Esperado '}' ao final do switch.");
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
            else throw std::runtime_error("Esperado tipo válido após '<' em input.");
            consume(TokenType::Greater, "Esperado '>' após tipo.");
        }
        consume(TokenType::LParen, "Esperado '(' após input.");
        if (!check(TokenType::String)) throw std::runtime_error("Esperado string como prompt do input.");
        std::string prompt = advance().lexeme;
        consume(TokenType::RParen, "Esperado ')' após prompt.");
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
                consume(TokenType::Identifier, "Esperado nome do campo após '.'");
                auto fieldName = tokens[current - 1].lexeme;
                auto fieldExpr = std::make_unique<LiteralExpr>(fieldName);
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(fieldExpr));
            } else if (match(TokenType::LBracket)) {
                auto index = expression();
                consume(TokenType::RBracket, "Esperado ']' após índice.");
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
            } else {
                break;
            }
        }
        // Suporte a chamada de função
        if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Esperado ')' após argumentos.");
            auto callee = std::make_unique<VariableExpr>(name);
            return std::make_unique<CallExpr>(std::move(callee), std::move(args));
        }

        // Suporte a indexação tipo p["nome"]
        while (match(TokenType::LBracket)) {
            auto index = expression();
            consume(TokenType::RBracket, "Esperado ']' após índice.");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        }

        return expr;
    }

    if (match(TokenType::LParen)) {
        auto expr = expression();
        consume(TokenType::RParen, "Esperado ')' após expressão.");
        return expr;
    }

    if (match(TokenType::LBracket)) {
        std::vector<std::unique_ptr<Expression>> elements;
        if (!check(TokenType::RBracket)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RBracket, "Esperado ']' após lista.");
        return std::make_unique<ListExpr>(std::move(elements));
    }
    std::unique_ptr<Expression> expr;

    if (match(TokenType::This)) {
        expr = std::make_unique<ThisExpr>();
    
        while (match(TokenType::Dot)) {  // <<< detecta o ponto
            consume(TokenType::Identifier, "Esperado nome do campo após '.'");
            std::string fieldName = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), fieldName); 
        }
        return expr;
    }    
    throw std::runtime_error("Expressão primária inválida na linha " + std::to_string(peek().line));
}


std::unique_ptr<Statement> Parser::structStatement() {
    consume(TokenType::Identifier, "Esperado nome da struct.");
    std::string name = tokens[current - 1].lexeme;
    consume(TokenType::LBrace, "Esperado '{' após o nome da struct.");

    std::vector<std::string> fields;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        consume(TokenType::Identifier, "Esperado nome do campo da struct.");
        fields.push_back(tokens[current - 1].lexeme);
        consume(TokenType::Semicolon, "Esperado ';' após o campo da struct.");
    }

    consume(TokenType::RBrace, "Esperado '}' para fechar a struct.");
    return std::make_unique<StructStmt>(name, std::move(fields));
}

std::unique_ptr<Statement> Parser::classStatement() {
    consume(TokenType::Identifier, "Esperado nome da classe.");
    std::string className = tokens[current - 1].lexeme;

    consume(TokenType::LBrace, "Esperado '{' após nome da classe.");

    std::vector<std::string> fields;
    std::vector<std::unique_ptr<FunctionStmt>> methods;

    while (!check(TokenType::RBrace) && !isAtEnd()) {
        if (match(TokenType::Func)) {
            // método
            auto method = functionStatement();
            methods.push_back(std::unique_ptr<FunctionStmt>(static_cast<FunctionStmt*>(method.release())));
        } else if (match(TokenType::Let)) { 
            // <<< ADICIONA ESSA LINHA
            consume(TokenType::Identifier, "Esperado nome do campo.");
            fields.push_back(tokens[current - 1].lexeme);
            consume(TokenType::Semicolon, "Esperado ';' após campo.");
        } else {
            throw std::runtime_error("Esperado 'func' ou 'let' dentro da classe.");
        }
    }

    consume(TokenType::RBrace, "Esperado '}' para fechar a classe.");
    return std::make_unique<ClassStmt>(className, std::move(fields), std::move(methods));
}

std::unique_ptr<Expression> Parser::finishAccessAndCall(std::unique_ptr<Expression> expr) {
    while (true) {
        if (match(TokenType::Dot)) {
            consume(TokenType::Identifier, "Esperado nome após '.'");
            auto field = previous().lexeme;
            expr = std::make_unique<GetExpr>(std::move(expr), field);
        } else if (match(TokenType::LParen)) {
            std::vector<std::unique_ptr<Expression>> arguments;
            if (!check(TokenType::RParen)) {
                do {
                    arguments.push_back(expression());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Esperado ')' após argumentos.");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
        } else if (match(TokenType::LBracket)) {
            auto index = expression();
            consume(TokenType::RBracket, "Esperado ']' após índice.");
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