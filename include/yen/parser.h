#ifndef PARSER_H
#define PARSER_H

#include "yen/value.h"
#include "yen/ast.h"
#include <vector>
#include <memory>
#include "yen/token.h"
#include <iostream>
#include <string>

class Parser {
    const std::vector<Token>& tokens;
    size_t current = 0;
    bool m_hadError = false;

    bool isAtEnd();
    Token previous() const {
        return tokens[current - 1];
    }

public:
    explicit Parser(const std::vector<Token>& tokens);
    std::vector<std::unique_ptr<Statement>> parse();
    std::unique_ptr<Expression> parseExpression();
    bool hadError() const { return m_hadError; }

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
    void error(const Token& token, const std::string& message);
    void synchronize();

    std::unique_ptr<Statement> statement();
    std::unique_ptr<Statement> expressionStatement();
    std::unique_ptr<Statement> printStatement();
    std::unique_ptr<Statement> assignStatement();
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> ternary();
    std::unique_ptr<Expression> pipe();
    std::unique_ptr<Expression> compose();
    std::unique_ptr<Expression> null_coalesce();
    std::unique_ptr<Expression> logic_or();
    std::unique_ptr<Expression> logic_and();
    std::unique_ptr<Expression> bitwise_or();
    std::unique_ptr<Expression> bitwise_xor();
    std::unique_ptr<Expression> bitwise_and();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> bitwise_shift();
    std::unique_ptr<Expression> cast();
    std::unique_ptr<Expression> range();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> power();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> primary();
    std::unique_ptr<Statement> letStatement(bool isMutable = false);
    std::unique_ptr<Statement> constStatement();
    std::unique_ptr<Statement> loopStatement();
    std::unique_ptr<Statement> deferStatement();
    std::unique_ptr<Statement> assertStatement();
    std::unique_ptr<Statement> ifStatement();
    std::unique_ptr<Statement> blockStatement();
    std::unique_ptr<Statement> functionStatement();
    std::unique_ptr<Statement> externBlock();
    std::unique_ptr<Statement> returnStatement();
    std::unique_ptr<Statement> forStatement();
    std::unique_ptr<Statement> whileStatement();
    std::unique_ptr<Statement> doWhileStatement();
    std::unique_ptr<Statement> enumStatement();
    std::unique_ptr<Statement> matchStatement();
    std::unique_ptr<Statement> switchStatement();
    std::unique_ptr<Statement> structStatement();
    std::unique_ptr<Statement> classStatement();
    std::unique_ptr<Statement> importStatement();
    std::unique_ptr<Statement> exportStatement();
    std::unique_ptr<Statement> tryCatchStatement();
    std::unique_ptr<Statement> throwStatement();
    std::unique_ptr<Statement> forDestructureStatement();
    std::unique_ptr<Statement> traitStatement();
    std::unique_ptr<Statement> implStatement();
    std::unique_ptr<Statement> repeatStatement();
    std::unique_ptr<Statement> extendStatement();
    std::unique_ptr<Expression> finishAccessAndCall(std::unique_ptr<Expression> expr);

    // Pattern parsing
    std::unique_ptr<Pattern> pattern();
    std::unique_ptr<Pattern> orPattern();
    std::unique_ptr<Pattern> primaryPattern();
};

#endif // PARSER_H
