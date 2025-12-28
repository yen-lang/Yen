#include "yen/lexer.h"
#include "yen/parser.h"
#include "yen/compiler.h"
#include "yen/stdlib.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Forward declarations
static void runFile(const char* path);
static void runRepl();
static void run(const std::string& source);

Interpreter interpreter;

int main(int argc, char* argv[]) {
    initialize_globals(interpreter);
    if (argc > 2) {
        std::cout << "Usage: yen [script]" << std::endl;
        return 64;
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        runRepl();
    }
    return 0;
}

static void runFile(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error opening file: " << path << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    run(buffer.str());
}

static void runRepl() {
    std::string line;
    for (;;) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
            break;
        }
        run(line);
    }
}

static void run(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    std::vector<std::unique_ptr<Statement>> statements = parser.parse();

    // In case of a syntax error, stop.
    if (parser.hadError()) return;

    try {
        interpreter.execute(statements);
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    }
}

