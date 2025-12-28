#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include "yen/lexer.h"
#include "yen/parser.h"
#include "yen/type_checker.h"
#include "yen/llvm_codegen.h"

// Command line options
struct CompilerOptions {
    std::string inputFile;
    std::string outputFile;
    bool emitLLVM = false;
    bool emitObject = false;
    bool compileOnly = false;
    int optimizationLevel = 2;
    bool verbose = false;
    bool typeCheckOnly = false;
    bool debugInfo = false;

    void printHelp() {
        std::cout << "YEN Compiler (yenc) - Compile YEN code to native executables\n\n";
        std::cout << "Usage: yenc <input.yen> [options]\n\n";
        std::cout << "Options:\n";
        std::cout << "  -o <file>        Output file name (default: a.out)\n";
        std::cout << "  -c               Compile to object file only (.o)\n";
        std::cout << "  -g, --debug      Generate debug symbols (DWARF)\n";
        std::cout << "  --emit-llvm      Emit LLVM IR (.ll file)\n";
        std::cout << "  --opt=<level>    Optimization level: 0-3 (default: 2)\n";
        std::cout << "  --type-check     Only run type checker, don't compile\n";
        std::cout << "  -v, --verbose    Verbose output\n";
        std::cout << "  -h, --help       Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  yenc script.yen -o myprogram\n";
        std::cout << "  yenc script.yen -g -o myprogram    # With debug symbols\n";
        std::cout << "  yenc script.yen --emit-llvm\n";
        std::cout << "  yenc script.yen -c -o script.o\n";
        std::cout << "  yenc script.yen --type-check\n";
    }
};

bool parseArgs(int argc, char* argv[], CompilerOptions& options) {
    if (argc < 2) {
        std::cerr << "Error: No input file specified\n";
        options.printHelp();
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            options.printHelp();
            return false;
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "-g" || arg == "--debug") {
            options.debugInfo = true;
        } else if (arg == "-o" && i + 1 < argc) {
            options.outputFile = argv[++i];
        } else if (arg == "-c") {
            options.compileOnly = true;
        } else if (arg == "--emit-llvm") {
            options.emitLLVM = true;
        } else if (arg == "--type-check") {
            options.typeCheckOnly = true;
        } else if (arg.find("--opt=") == 0) {
            try {
                options.optimizationLevel = std::stoi(arg.substr(6));
                if (options.optimizationLevel < 0 || options.optimizationLevel > 3) {
                    std::cerr << "Error: Optimization level must be 0-3\n";
                    return false;
                }
            } catch (...) {
                std::cerr << "Error: Invalid optimization level\n";
                return false;
            }
        } else if (arg[0] != '-') {
            options.inputFile = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            options.printHelp();
            return false;
        }
    }

    if (options.inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        options.printHelp();
        return false;
    }

    // Set default output file if not specified
    if (options.outputFile.empty()) {
        if (options.emitLLVM) {
            options.outputFile = "output.ll";
        } else if (options.compileOnly) {
            options.outputFile = "output.o";
        } else {
            options.outputFile = "a.out";
        }
    }

    return true;
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << "\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    CompilerOptions options;

    // Parse command line arguments
    if (!parseArgs(argc, argv, options)) {
        return 1;
    }

    if (options.verbose) {
        std::cout << "YEN Compiler v0.1.0\n";
        std::cout << "Input file: " << options.inputFile << "\n";
        std::cout << "Output file: " << options.outputFile << "\n";
        std::cout << "Optimization level: " << options.optimizationLevel << "\n";
    }

    // Read source file
    std::string source = readFile(options.inputFile);
    if (source.empty()) {
        return 1;
    }

    // ========================================================================
    // LEXING
    // ========================================================================
    if (options.verbose) {
        std::cout << "\n[1/4] Lexing...\n";
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (options.verbose) {
        std::cout << "Generated " << tokens.size() << " tokens\n";
    }

    // ========================================================================
    // PARSING
    // ========================================================================
    if (options.verbose) {
        std::cout << "\n[2/4] Parsing...\n";
    }

    Parser parser(tokens);
    std::vector<std::unique_ptr<Statement>> ast;

    try {
        ast = parser.parse();
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    if (options.verbose) {
        std::cout << "Generated AST with " << ast.size() << " top-level statements\n";
    }

    // ========================================================================
    // TYPE CHECKING
    // ========================================================================
    if (options.verbose) {
        std::cout << "\n[3/4] Type checking...\n";
    }

    yen::TypeChecker typeChecker;
    bool typeCheckPassed = typeChecker.check(ast);

    if (!typeCheckPassed) {
        std::cerr << "Type checking failed:\n";
        for (const auto& error : typeChecker.getErrors()) {
            std::cerr << "  " << error << "\n";
        }
        return 1;
    }

    if (options.verbose) {
        std::cout << "Type checking passed\n";
    }

    // If only type checking was requested, we're done
    if (options.typeCheckOnly) {
        std::cout << "Type checking successful!\n";
        return 0;
    }

    // ========================================================================
    // CODE GENERATION
    // ========================================================================
    if (options.verbose) {
        std::cout << "\n[4/4] Generating code...\n";
    }

    yen::LLVMCodeGen codegen(options.inputFile, &typeChecker);

    // Set up debug information if requested
    if (options.debugInfo) {
        codegen.setSourceFile(options.inputFile);
        codegen.setDebugInfo(true);
        if (options.verbose) {
            std::cout << "Debug symbols enabled (DWARF)\n";
        }
    }

    if (!codegen.generate(ast)) {
        std::cerr << "Code generation failed\n";
        return 1;
    }

    if (options.verbose) {
        std::cout << "Code generation successful\n";
    }

    // Optimize if requested
    if (options.optimizationLevel > 0) {
        if (options.verbose) {
            std::cout << "Optimizing (level " << options.optimizationLevel << ")...\n";
        }
        codegen.optimize(options.optimizationLevel);
    }

    // Verify module
    if (!codegen.verifyModule()) {
        std::cerr << "Module verification failed\n";
        return 1;
    }

    // ========================================================================
    // OUTPUT
    // ========================================================================
    if (options.emitLLVM) {
        if (options.verbose) {
            std::cout << "Emitting LLVM IR to " << options.outputFile << "...\n";
        }
        codegen.emitLLVMIR(options.outputFile);
    } else if (options.compileOnly) {
        if (options.verbose) {
            std::cout << "Emitting object file to " << options.outputFile << "...\n";
        }
        codegen.emitObjectFile(options.outputFile);
    } else {
        if (options.verbose) {
            std::cout << "Emitting executable to " << options.outputFile << "...\n";
        }
        codegen.emitExecutable(options.outputFile);
    }

    if (options.verbose) {
        std::cout << "\nCompilation successful!\n";
        std::cout << "Output written to: " << options.outputFile << "\n";
    }

    return 0;
}
