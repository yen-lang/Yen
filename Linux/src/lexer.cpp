#include "lexer.h"
#include <cctype>
#include <iostream>
#include <unordered_map>

Lexer::Lexer(const std::string& src) : source(src) {}

std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.emplace_back(TokenType::Eof, "", line);
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
    tokens.emplace_back(type, lexeme.empty() ? source.substr(start, current - start) : lexeme, line);
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
            advance(); // consome '*'
            advance(); // consome '/'
            return;
        }
        if (peek() == '\n') line++;
        advance();
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
        case '+': addToken(TokenType::Plus); break;
        case '-': addToken(TokenType::Minus); break;
        case '*': addToken(TokenType::Star); break;
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
            } else {
                addToken(TokenType::Slash);
            }
            break;
        case '<': addToken(match('=') ? TokenType::LessEqual : TokenType::Less); break;
        case '>': addToken(match('=') ? TokenType::GreaterEqual : TokenType::Greater); break;
        case '\n': line++; break;
        case ' ':
        case '\r':
        case '\t': break;
        case '"': string(); break;
        case '#': skipComment(); break;
        case '!': addToken(match('=') ? TokenType::BangEqual : TokenType::Not); break;
        case '&':
            if (match('&')) addToken(TokenType::AndAnd);  // && 
            else addToken(TokenType::Invalid);            //  &
            break;
        case '|':
            if (match('|')) addToken(TokenType::OrOr);    // || 
            else addToken(TokenType::Invalid);            //  |
            break;
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
        case '.': addToken(TokenType::Dot); break;
    }
}

void Lexer::identifier() {
    while (std::isalnum(peek()) || peek() == '_') advance();
    std::string text = source.substr(start, current - start);

    static std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::Let}, {"if", TokenType::If}, {"else", TokenType::Else},
        {"while", TokenType::While}, {"func", TokenType::Func}, {"return", TokenType::Return},
        {"true", TokenType::True}, {"false", TokenType::False}, {"print", TokenType::Print},
        {"input", TokenType::Input},
        //  cast
        {"int", TokenType::Int},
        {"float", TokenType::Float},
        {"bool", TokenType::Bool},
        {"str", TokenType::Str},
        // input<T>
        {"_int", TokenType::IntType},
        {"_float", TokenType::FloatType},
        {"_bool", TokenType::BoolType},
        {"_str", TokenType::StrType},
        {"for", TokenType::For},
        {"in", TokenType::In},
        {"for", TokenType::For},
        {"in", TokenType::In},
        {"while", TokenType::While},
        {"break", TokenType::Break},
        {"enum", TokenType::Enum},
        {"continue", TokenType::Continue},
        {"match", TokenType::Match},
        {"switch", TokenType::Switch},
        {"case", TokenType::Case},
        {"default", TokenType::Default},
        {"struct", TokenType::Struct},
        {"class", TokenType::Class}
    };

    auto it = keywords.find(text);
    TokenType type = it != keywords.end() ? it->second : TokenType::Identifier;
    std::cout << "[LEXER] " << text << " → " << (int)type << std::endl;  // DEBUG
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
    addToken(TokenType::Number, text);
}


void Lexer::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (isAtEnd()) {
        std::cerr << "Unterminated string in line " << line << "\n";
        return;
    }

    advance(); 
    std::string value = source.substr(start + 1, current - start - 2);
    addToken(TokenType::String, value);
}

