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
    int column = start - lineStart + 1;  // Calculate column (1-indexed)
    tokens.emplace_back(type, lexeme.empty() ? source.substr(start, current - start) : lexeme, line, column);
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
            lineStart = current;  // Reset line start for column calculation
        } else {
            advance();
        }
    }
    std::cerr << "Unfinished block comment before end of file\n";
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
        case ':': addToken(TokenType::Colon); break;
        case '+':
            addToken(match('=') ? TokenType::PlusEqual : TokenType::Plus);
            break;
        case '-':
            addToken(match('=') ? TokenType::MinusEqual : TokenType::Minus);
            break;
        case '*':
            if (match('*')) {
                addToken(TokenType::DoubleStar);  // **
            } else if (match('=')) {
                addToken(TokenType::StarEqual);   // *=
            } else {
                addToken(TokenType::Star);        // *
            }
            break;
        case '%': addToken(TokenType::Percent); break;
        case '[': addToken(TokenType::LBracket); break;
        case ']': addToken(TokenType::RBracket); break;        
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
                addToken(TokenType::SlashEqual);  // /=
            } else {
                addToken(TokenType::Slash);       // /
            }
            break;
        case '<':
            if (match('<')) {
                addToken(TokenType::LeftShift);   // <<
            } else if (match('=')) {
                addToken(TokenType::LessEqual);   // <=
            } else {
                addToken(TokenType::Less);        // <
            }
            break;
        case '>':
            if (match('>')) {
                addToken(TokenType::RightShift);  // >>
            } else if (match('=')) {
                addToken(TokenType::GreaterEqual); // >=
            } else {
                addToken(TokenType::Greater);     // >
            }
            break;
        case '\n':
            line++;
            lineStart = current;  // Reset line start for column calculation
            break;
        case ' ':
        case '\r':
        case '\t': break;
        case '"': string(); break;
        case '#': skipComment(); break;
        case '!': addToken(match('=') ? TokenType::BangEqual : TokenType::Not); break;
        case '?': addToken(TokenType::Question); break;  // ? for try operator
        case '&':
            if (match('&')) {
                addToken(TokenType::AndAnd);      // &&
            } else {
                addToken(TokenType::Ampersand);   // & (bitwise)
            }
            break;
        case '|':
            if (match('|')) {
                addToken(TokenType::OrOr);        // ||
            } else {
                addToken(TokenType::Pipe);        // | (bitwise or lambda)
            }
            break;
        case '^': addToken(TokenType::Caret); break;   // ^ (bitwise XOR)
        case '~': addToken(TokenType::Tilde); break;   // ~ (bitwise NOT)
        case '=':
            if (peek() == '>') {
                advance();  //  '>'
                addToken(TokenType::Arrow, "=>");
            } else if (peek() == '=') {
                advance();  // '='
                addToken(TokenType::EqualEqual, "==");
            } else {
                addToken(TokenType::Assign, "=");
            }
            break;
        default:
            if (std::isdigit(c)) {
                number();
            } else if (std::isalpha(c) || c == '_') {
                identifier();
            } else {
                std::cerr << "Caractere inválido na linha " << line << ": '" << c << "'\n";
                addToken(TokenType::Invalid);
            }
            break;
        case '.':
            if (peek() == '.') {
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
    }
}

void Lexer::identifier() {
    while (std::isalnum(peek()) || peek() == '_') advance();
    std::string text = source.substr(start, current - start);

    static std::unordered_map<std::string, TokenType> keywords = {
        // Control flow
        {"let", TokenType::Let}, {"var", TokenType::Var},
        {"if", TokenType::If}, {"else", TokenType::Else},
        {"while", TokenType::While}, {"loop", TokenType::Loop}, {"for", TokenType::For}, {"in", TokenType::In},
        {"break", TokenType::Break}, {"continue", TokenType::Continue},
        {"match", TokenType::Match}, {"switch", TokenType::Switch},
        {"case", TokenType::Case}, {"default", TokenType::Default},
        {"defer", TokenType::Defer}, {"assert", TokenType::Assert},

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
        {"export", TokenType::Export}
    };

    auto it = keywords.find(text);
    TokenType type = it != keywords.end() ? it->second : TokenType::Identifier;
    // std::cout << "[LEXER] " << text << " → " << (int)type << std::endl;  // DEBUG
    addToken(type);
}


void Lexer::number() {
    bool isFloat = false;
    while (std::isdigit(peek())) advance();

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance(); //  '.'
        while (std::isdigit(peek())) advance();
    }

    std::string text = source.substr(start, current - start);
    addToken(isFloat ? TokenType::Float : TokenType::Int, text);
}


void Lexer::string() {
    std::string value;

    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\\') {
            // Handle escape sequences
            advance(); // consume '\'
            if (isAtEnd()) break;

            char escaped = advance();
            switch (escaped) {
                case 'n':  value += '\n'; break;
                case 't':  value += '\t'; break;
                case 'r':  value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"':  value += '"'; break;
                case '0':  value += '\0'; break;
                default:
                    std::cerr << "Unknown escape sequence '\\" << escaped << "' at line " << line << "\n";
                    value += escaped;
                    break;
            }
        } else {
            if (peek() == '\n') {
                line++;
                value += advance();
                lineStart = current;  // Reset line start for column calculation
            } else {
                value += advance();
            }
        }
    }

    if (isAtEnd()) {
        std::cerr << "Unterminated string in line " << line << "\n";
        return;
    }

    advance(); // consume closing "
    addToken(TokenType::String, value);
}

