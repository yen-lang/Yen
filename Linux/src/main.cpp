#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: yenlang <arquivo.yl>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Erro ao abrir o arquivo.\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto statements = parser.parse();

    Interpreter interpreter;
    interpreter.execute(statements);

    return 0;
}
