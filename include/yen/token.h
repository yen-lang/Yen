#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Variable declarations
    Let, Var, Const, Mut,

    // Control flow
    If, Else, While, Do, Loop, For, In,
    Break, Continue,
    Match, Switch, Case, Default,
    Defer, Assert,
    Go,  // go expr; (goroutine-style concurrency)

    // Error handling
    Try, Catch, Throw, Finally,

    // Functions and types
    Func, Return,
    Struct, Class, Enum,
    Extern, Unsafe,
    As,
    Pub, Priv, Impl, Trait, Self_,
    This,
    Extends, Super, Static, Is,
    Unless, Until, Guard, Repeat, Extend,
    Data, Sealed, Lazy,

    // Module system
    Import, Export,

    // Option and Result
    Option, Some, None,
    Result, Ok, Err,

    // Literals
    Identifier, Number, String,
    True, False,

    // I/O
    Print, Input,

    // Arithmetic operators
    Plus, Minus, Star, Slash, Percent,
    DoubleStar,  // ** exponentiation
    PlusPlus, MinusMinus,  // ++ -- (increment/decrement)
    PlusEqual, MinusEqual, StarEqual, SlashEqual, PercentEqual,  // Compound assignment
    DoubleStarEqual,   // **= exponent assignment
    AmpersandEqual,    // &=
    PipeEqual,         // |=
    CaretEqual,        // ^=
    LeftShiftEqual,    // <<=
    RightShiftEqual,   // >>=

    // Comparison operators
    EqualEqual, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,

    // Logical operators
    And, Or, Not, AndAnd, OrOr,

    // Bitwise operators
    Ampersand,   // & (bitwise AND)
    Pipe,        // | (bitwise OR / lambda delimiter)
    Caret,       // ^ (bitwise XOR)
    Tilde,       // ~ (bitwise NOT)
    LeftShift,   // <<
    RightShift,  // >>

    // Special operators
    Question,      // ? (ternary operator)
    QuestionQuestion, // ?? (null coalescing)
    QuestionDot,   // ?. (optional chaining)
    DotDot,        // .. (exclusive range)
    DotDotEqual,   // ..= (inclusive range)
    DotDotDot,     // ... (spread operator)
    Underscore,    // _ (wildcard)
    PipeArrow,     // |> (pipe operator)
    ReturnArrow,   // -> (return type annotation)
    Arrow,         // => (match arm / fat arrow)
    Compose,       // >>> (function composition)
    ColonEqual,    // := (walrus operator)
    QuestionEqual, // ?= (null-coalescing assignment)

    // Delimiters
    LParen, RParen,     // ( )
    LBrace, RBrace,     // { }
    LBracket, RBracket, // [ ]
    Comma, Semicolon, Colon, Dot,
    Assign,             // =

    // Special
    Eof, Invalid,

    // Type keywords
    Int, Float, Str, Bool,
    IntType, FloatType, BoolType, StrType,

    // Compatibility aliases (kept for existing code)
    BitOr = Pipe
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
