#ifndef PARSER_H
#define PARSER_H

#include "value.h"
#include "ast.h"
#include <vector>
#include <memory>
#include "token.h"  // ⬅ define Token e TokenType
#include <iostream>
#include <string>


class Parser {
    const std::vector<Token>& tokens;
    size_t current = 0;
    bool isAtEnd();
    Token previous() const {
        return tokens[current - 1];
    }

public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Statement>> parse();

private:
    const Token& peek();
    const Token& advance();
    bool match(TokenType type);
    bool check(TokenType type);
    bool checkNext(TokenType type) {
        if (current + 1 >= tokens.size()) return false;
        return tokens[current + 1].type == type;
    }
    void consume(TokenType type, const std::string& message);
    std::unique_ptr<Statement> statement();
    std::unique_ptr<Statement> printStatement();
    std::unique_ptr<Statement> assignStatement();
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> primary();
    std::unique_ptr<Statement> letStatement();
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> blockStatement();
    std::unique_ptr<Expression> logic_or();
    std::unique_ptr<Expression> logic_and();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Statement> functionStatement();
    std::unique_ptr<Statement> returnStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> enumStatement();
    std::unique_ptr<Statement> matchStatement();
    std::unique_ptr<Statement> switchStatement();
    std::unique_ptr<Statement> structStatement();
    std::unique_ptr<Statement> classStatement();
    std::unique_ptr<Expression> finishAccessAndCall(std::unique_ptr<Expression> expr);
};





#endif // PARSER_H
