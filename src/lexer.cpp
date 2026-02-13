#include "yen/lexer.h"
#include <cctype>
#include <iostream>
#include <unordered_map>

Lexer::Lexer(const std::string& src) : source(src) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    int column = current - lineStart + 1;
    tokens.emplace_back(TokenType::Eof, "", line, column);
    return tokens;
}

bool Lexer::isAtEnd() const {
    return current >= static_cast<int>(source.size());
}

char Lexer::advance() {
    return source[current++];
}

char Lexer::peek() const {
    return isAtEnd() ? '\0' : source[current];
}

char Lexer::peekNext() const {
    return (current + 1 >= static_cast<int>(source.size())) ? '\0' : source[current + 1];
}

void Lexer::addToken(TokenType type, const std::string& lexeme) {
    int column = start - lineStart + 1;
    tokens.emplace_back(type, lexeme, line, column);
}

void Lexer::addToken(TokenType type) {
    int column = start - lineStart + 1;
    tokens.emplace_back(type, source.substr(start, current - start), line, column);
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

void Lexer::skipComment() {
    while (peek() != '\n' && !isAtEnd()) advance();
}

void Lexer::skipBlockComment() {
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == '/') {
            advance(); // consume '*'
            advance(); // consume '/'
            return;
        }
        if (peek() == '\n') {
            line++;
            advance();
            lineStart = current;
        } else {
            advance();
        }
    }
    std::cerr << "Unterminated block comment at end of file\n";
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LParen); break;
        case ')': addToken(TokenType::RParen); break;
        case '{': addToken(TokenType::LBrace); break;
        case '}': addToken(TokenType::RBrace); break;
        case ',': addToken(TokenType::Comma); break;
        case ';': addToken(TokenType::Semicolon); break;
        case ':':
            addToken(match('=') ? TokenType::ColonEqual : TokenType::Colon);
            break;
        case '[': addToken(TokenType::LBracket); break;
        case ']': addToken(TokenType::RBracket); break;
        case '?':
            if (match('?')) {
                addToken(TokenType::QuestionQuestion);  // ??
            } else if (match('.')) {
                addToken(TokenType::QuestionDot);       // ?.
            } else if (match('=')) {
                addToken(TokenType::QuestionEqual);     // ?=
            } else {
                addToken(TokenType::Question);          // ?
            }
            break;
        case '~': addToken(TokenType::Tilde); break;
        case '^':
            addToken(match('=') ? TokenType::CaretEqual : TokenType::Caret);
            break;

        case '.':
            if (peek() == '.' && peekNext() == '.') {
                advance();  // consume second '.'
                advance();  // consume third '.'
                addToken(TokenType::DotDotDot);        // ...
            } else if (peek() == '.') {
                advance();  // consume second '.'
                if (match('=')) {
                    addToken(TokenType::DotDotEqual);  // ..=
                } else {
                    addToken(TokenType::DotDot);       // ..
                }
            } else {
                addToken(TokenType::Dot);              // .
            }
            break;

        case '+':
            if (match('+')) {
                addToken(TokenType::PlusPlus);    // ++
            } else if (match('=')) {
                addToken(TokenType::PlusEqual);   // +=
            } else {
                addToken(TokenType::Plus);        // +
            }
            break;

        case '-':
            if (match('>')) {
                addToken(TokenType::ReturnArrow, "->");  // ->
            } else if (match('-')) {
                addToken(TokenType::MinusMinus);         // --
            } else if (match('=')) {
                addToken(TokenType::MinusEqual);         // -=
            } else {
                addToken(TokenType::Minus);              // -
            }
            break;

        case '*':
            if (match('*')) {
                if (match('=')) {
                    addToken(TokenType::DoubleStarEqual);  // **=
                } else {
                    addToken(TokenType::DoubleStar);       // **
                }
            } else if (match('=')) {
                addToken(TokenType::StarEqual);    // *=
            } else {
                addToken(TokenType::Star);         // *
            }
            break;

        case '%':
            addToken(match('=') ? TokenType::PercentEqual : TokenType::Percent);
            break;

        case '/':
            if (peek() == '/') {
                advance();
                skipComment();
                return;
            } else if (peek() == '*') {
                advance();
                skipBlockComment();
                return;
            } else if (match('=')) {
                addToken(TokenType::SlashEqual);
            } else {
                addToken(TokenType::Slash);
            }
            break;

        case '<':
            if (match('<')) {
                if (match('=')) {
                    addToken(TokenType::LeftShiftEqual);  // <<=
                } else {
                    addToken(TokenType::LeftShift);       // <<
                }
            } else if (match('=')) {
                addToken(TokenType::LessEqual);    // <=
            } else {
                addToken(TokenType::Less);         // <
            }
            break;

        case '>':
            if (match('>')) {
                if (match('>')) {
                    addToken(TokenType::Compose);        // >>>
                } else if (match('=')) {
                    addToken(TokenType::RightShiftEqual); // >>=
                } else {
                    addToken(TokenType::RightShift);      // >>
                }
            } else if (match('=')) {
                addToken(TokenType::GreaterEqual); // >=
            } else {
                addToken(TokenType::Greater);      // >
            }
            break;

        case '!':
            addToken(match('=') ? TokenType::BangEqual : TokenType::Not);
            break;

        case '&':
            if (match('&')) {
                addToken(TokenType::AndAnd);       // &&
            } else if (match('=')) {
                addToken(TokenType::AmpersandEqual);  // &=
            } else {
                addToken(TokenType::Ampersand);    // &
            }
            break;

        case '|':
            if (match('|')) {
                addToken(TokenType::OrOr);         // ||
            } else if (match('>')) {
                addToken(TokenType::PipeArrow, "|>");  // |> (pipe operator)
            } else if (match('=')) {
                addToken(TokenType::PipeEqual);    // |=
            } else {
                addToken(TokenType::Pipe);         // |
            }
            break;

        case '=':
            if (peek() == '>') {
                advance();
                addToken(TokenType::Arrow, "=>");
            } else if (peek() == '=') {
                advance();
                addToken(TokenType::EqualEqual, "==");
            } else {
                addToken(TokenType::Assign, "=");
            }
            break;

        case '\n':
            line++;
            lineStart = current;
            break;

        case ' ':
        case '\r':
        case '\t': break;

        case '"':
            // Check for triple-quoted string: """..."""
            if (peek() == '"' && peekNext() == '"') {
                advance(); // consume second "
                advance(); // consume third "
                tripleString();
            } else {
                string('"');
            }
            break;
        case '\'': string('\''); break;
        case '#': skipComment(); break;

        default:
            if (std::isdigit(c)) {
                number();
            } else if (std::isalpha(c) || c == '_') {
                identifier();
            } else {
                std::cerr << "Invalid character at line " << line << ": '" << c << "'\n";
                addToken(TokenType::Invalid);
            }
            break;
    }
}

void Lexer::identifier() {
    // Check for raw string: r"..."
    if (source[start] == 'r' && current == start + 1 && peek() == '"') {
        advance(); // consume "
        rawString();
        return;
    }

    while (std::isalnum(peek()) || peek() == '_') advance();
    std::string text = source.substr(start, current - start);

    // Check if it's just underscore (wildcard pattern)
    if (text == "_") {
        addToken(TokenType::Underscore);
        return;
    }

    static std::unordered_map<std::string, TokenType> keywords = {
        // Control flow
        {"let", TokenType::Let}, {"var", TokenType::Var},
        {"if", TokenType::If}, {"else", TokenType::Else},
        {"while", TokenType::While}, {"do", TokenType::Do}, {"loop", TokenType::Loop}, {"for", TokenType::For}, {"in", TokenType::In}, {"go", TokenType::Go},
        {"break", TokenType::Break}, {"continue", TokenType::Continue},
        {"match", TokenType::Match}, {"switch", TokenType::Switch},
        {"case", TokenType::Case}, {"default", TokenType::Default},
        {"defer", TokenType::Defer}, {"assert", TokenType::Assert},

        // Error handling
        {"try", TokenType::Try}, {"catch", TokenType::Catch}, {"throw", TokenType::Throw}, {"finally", TokenType::Finally},

        // Functions and types
        {"func", TokenType::Func}, {"return", TokenType::Return},
        {"struct", TokenType::Struct}, {"enum", TokenType::Enum},
        {"extern", TokenType::Extern}, {"const", TokenType::Const}, {"mut", TokenType::Mut},
        {"as", TokenType::As}, {"unsafe", TokenType::Unsafe},
        {"pub", TokenType::Pub}, {"priv", TokenType::Priv},
        {"impl", TokenType::Impl}, {"trait", TokenType::Trait},
        {"self", TokenType::Self_},

        // Literals
        {"true", TokenType::True}, {"false", TokenType::False},
        {"print", TokenType::Print}, {"input", TokenType::Input},

        // Option and Result
        {"Option", TokenType::Option}, {"Some", TokenType::Some}, {"None", TokenType::None},
        {"Result", TokenType::Result}, {"Ok", TokenType::Ok}, {"Err", TokenType::Err},

        // Type keywords
        {"int", TokenType::Int},
        {"float", TokenType::Float},
        {"bool", TokenType::Bool},
        {"str", TokenType::Str},
        {"_int", TokenType::IntType},
        {"_float", TokenType::FloatType},
        {"_bool", TokenType::BoolType},
        {"_str", TokenType::StrType},
        {"class", TokenType::Class},
        {"this", TokenType::This},
        {"import", TokenType::Import},
        {"export", TokenType::Export},
        {"extends", TokenType::Extends},
        {"super", TokenType::Super},
        {"static", TokenType::Static},
        {"is", TokenType::Is},
        {"unless", TokenType::Unless},
        {"until", TokenType::Until},
        {"guard", TokenType::Guard},
        {"repeat", TokenType::Repeat},
        {"extend", TokenType::Extend},
        {"data", TokenType::Data},
        {"sealed", TokenType::Sealed},
        {"lazy", TokenType::Lazy}
    };

    auto it = keywords.find(text);
    TokenType type = it != keywords.end() ? it->second : TokenType::Identifier;
    addToken(type);
}

void Lexer::number() {
    bool isFloat = false;

    // Handle hex: 0x...
    if (source[start] == '0' && peek() == 'x') {
        advance(); // consume 'x'
        while (std::isxdigit(peek())) advance();
        std::string text = source.substr(start, current - start);
        addToken(TokenType::Int, text);
        return;
    }

    // Handle binary: 0b...
    if (source[start] == '0' && peek() == 'b') {
        advance(); // consume 'b'
        while (peek() == '0' || peek() == '1') advance();
        std::string text = source.substr(start, current - start);
        addToken(TokenType::Int, text);
        return;
    }

    while (std::isdigit(peek())) advance();

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance(); // consume '.'
        while (std::isdigit(peek())) advance();
    }

    std::string text = source.substr(start, current - start);
    addToken(isFloat ? TokenType::Float : TokenType::Int, text);
}

void Lexer::string(char quote) {
    std::string value;

    while (peek() != quote && !isAtEnd()) {
        if (peek() == '\\') {
            advance(); // consume '\'
            if (isAtEnd()) break;

            char escaped = advance();
            switch (escaped) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"'; break;
                case '\'': value += '\''; break;
                case '0':  value += '\0'; break;
                case 'x': {
                    // Hex escape: \x41 = 'A'
                    char h1 = isAtEnd() ? '0' : advance();
                    char h2 = isAtEnd() ? '0' : advance();
                    std::string hex = {h1, h2};
                    value += static_cast<char>(std::stoi(hex, nullptr, 16));
                    break;
                }
                default:
                    std::cerr << "Unknown escape sequence '\\" << escaped << "' at line " << line << "\n";
                    value += escaped;
                    break;
            }
        } else {
            if (peek() == '\n') {
                line++;
                value += advance();
                lineStart = current;
            } else {
                value += advance();
            }
        }
    }

    if (isAtEnd()) {
        std::cerr << "Unterminated string at line " << line << "\n";
        return;
    }

    advance(); // consume closing quote
    addToken(TokenType::String, value);
}

void Lexer::rawString() {
    // Already consumed r and opening "
    std::string value;
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line++;
            lineStart = current + 1;
        }
        value += advance();
    }
    if (isAtEnd()) {
        std::cerr << "Unterminated raw string at line " << line << "\n";
        return;
    }
    advance(); // consume closing "
    addToken(TokenType::String, value);
}

void Lexer::tripleString() {
    // Already consumed opening """
    std::string value;
    while (!isAtEnd()) {
        if (peek() == '"' && peekNext() == '"' &&
            (current + 2 < static_cast<int>(source.size())) && source[current + 2] == '"') {
            advance(); advance(); advance(); // consume closing """
            // Trim leading newline if present
            if (!value.empty() && value[0] == '\n') {
                value = value.substr(1);
            }
            addToken(TokenType::String, value);
            return;
        }
        if (peek() == '\n') {
            line++;
            value += advance();
            lineStart = current;
        } else {
            value += advance();
        }
    }
    std::cerr << "Unterminated triple-quoted string at line " << line << "\n";
}
