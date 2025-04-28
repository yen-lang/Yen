#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <string>
#include <vector>

class Lexer {
    std::string source;
    std::vector<Token> tokens;
    int start = 0, current = 0, line = 1;

public:
    explicit Lexer(const std::string& src);
    std::vector<Token> tokenize();

private:
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    void addToken(TokenType type, const std::string& lexeme = "");
    void scanToken();

    void string();
    void number();
    void identifier();
    bool match(char expected);
    void skipComment();
    void skipBlockComment();  // suport to /* ... */

};

#endif // LEXER_H
