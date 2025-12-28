#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Keywords
    Let, Var, If, Else, While, Loop, Func, Return, True, False, Print, Input, Break,
    Continue, Struct, Class, This, Import, Export, Extern, Const, Mut,
    Enum, Match, Switch, Arrow, Case, Default,
    As, Unsafe, Some, None, Ok, Err, Option, Result,
    Defer, Assert, Pub, Priv, Impl, Trait, Self_,

    // Literals
    Identifier, Number, String,

    // Operators
    Plus, Minus, Star, Slash, Percent,
    Equal, EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    And, Or, Not, AndAnd, OrOr,
    Question,  // ? for try operator
    Pipe,      // | for lambdas
    DoubleStar, // ** for exponentiation
    PlusEqual, MinusEqual, StarEqual, SlashEqual,  // Compound assignment
    Ampersand, BitOr, Caret, Tilde,  // Bitwise operators
    LeftShift, RightShift,  // << >>
    DotDot, DotDotEqual,  // .. and ..= for ranges
    Underscore,  // _ for pattern matching

    // Delimiters
    LParen, RParen,
    LBrace, RBrace,
    Comma, Semicolon, Colon, Assign, LBracket, RBracket, Dot,
    For, In,

    // Special
    Eof, Invalid,

    // Type keywords
    Int, Float, Str, Bool,
    IntType, FloatType, BoolType, StrType

};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, const std::string& lexeme, int line, int column = 0)
        : type(type), lexeme(lexeme), line(line), column(column) {}
};

#endif // TOKEN_H
