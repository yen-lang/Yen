#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Palavras-chave
    Let, If, Else, While, Func, Return, True, False, Print, Input, Break,
    Continue, Struct, Class, This,

    // Identificadores e literais
    Identifier, Number, String,

    // Operadores
    Plus, Minus, Star, Slash,
    Equal, EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    And, Or, Not, AndAnd, OrOr,
    
    // === Estruturas avançadas ===
    Enum, Match, Switch, Arrow, Case, Default,

    // Símbolos
    LParen, RParen,
    LBrace, RBrace,
    Comma, Semicolon, Assign, LBracket, RBracket, For, In,

    // Especial
    Eof, Invalid,

    // Casting
    Int, Float, Str, Bool, Dot,
    
    // Tipos para input<T>
    IntType, FloatType, BoolType, StrType  // <<< novos tipos
  
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;

    Token(TokenType type, const std::string& lexeme, int line)
        : type(type), lexeme(lexeme), line(line) {}
};

#endif // TOKEN_H
