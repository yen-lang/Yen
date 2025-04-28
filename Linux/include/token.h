#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    Let, If, Else, While, Func, Return, True, False, Print, Input, Break,
    Continue, Struct, Class, This,

    Identifier, Number, String,

    Plus, Minus, Star, Slash,
    Equal, EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    And, Or, Not, AndAnd, OrOr,
    
    Enum, Match, Switch, Arrow, Case, Default,

    LParen, RParen,
    LBrace, RBrace,
    Comma, Semicolon, Assign, LBracket, RBracket, For, In,

    Eof, Invalid,

    Int, Float, Str, Bool, Dot,
    
    IntType, FloatType, BoolType, StrType  
  
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    Token(TokenType type, const std::string& lexeme, int line)
        : type(type), lexeme(lexeme), line(line) {}
};

#endif // TOKEN_H
