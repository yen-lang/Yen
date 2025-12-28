#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "yen/lexer.h"
#include "yen/parser.h"
#include "yen/type_checker.h"

struct TestCase {
    std::string name;
    std::string code;
    bool shouldPass;
};

bool runTypeCheck(const std::string& code) {
    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens);
    std::vector<std::unique_ptr<Statement>> ast = parser.parse();

    yen::TypeChecker typeChecker;
    return typeChecker.check(ast);
}

void printErrors(const std::string& code) {
    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser(tokens);
    std::vector<std::unique_ptr<Statement>> ast = parser.parse();

    yen::TypeChecker typeChecker;
    typeChecker.check(ast);

    for (const auto& error : typeChecker.getErrors()) {
        std::cout << "    " << error << "\n";
    }
}

int main() {
    std::vector<TestCase> tests = {
        // Valid cases
        {
            "Valid: Basic int32 variable",
            "let x: int32 = 42;",
            true
        },
        {
            "Valid: Basic string variable",
            "let name: string = \"Alice\";",
            true
        },
        {
            "Valid: Function with matching types",
            "func add(a: int32, b: int32) -> int32 { return a + b; }",
            true
        },
        {
            "Valid: String concatenation",
            "func greet(name: string) -> string { return \"Hello, \" + name; }",
            true
        },
        {
            "Valid: Mixed typed and untyped",
            "let typed: int32 = 10;\nlet untyped = 20;",
            true
        },

        // Invalid cases
        {
            "Invalid: String assigned to int32",
            "let x: int32 = \"hello\";",
            false
        },
        {
            "Invalid: Int assigned to string",
            "let name: string = 42;",
            false
        },
        {
            "Invalid: Wrong function argument types",
            "func add(a: int32, b: int32) -> int32 { return a + b; }\nlet r = add(\"x\", \"y\");",
            false
        },
        {
            "Invalid: Wrong return type",
            "func getNumber() -> int32 { return \"not a number\"; }",
            false
        },
        {
            "Invalid: Float assigned to bool",
            "let flag: bool = 3.14;",
            false
        },
    };

    int passed = 0;
    int failed = 0;

    for (const auto& test : tests) {
        bool result = runTypeCheck(test.code);
        bool success = (result == test.shouldPass);

        if (success) {
            std::cout << "✅ PASS: " << test.name << "\n";
            passed++;
        } else {
            std::cout << "❌ FAIL: " << test.name << "\n";
            std::cout << "  Expected: " << (test.shouldPass ? "pass" : "fail") << "\n";
            std::cout << "  Got: " << (result ? "pass" : "fail") << "\n";

            if (!test.shouldPass && result) {
                std::cout << "  (Should have detected type errors but didn't)\n";
            } else if (test.shouldPass && !result) {
                std::cout << "  Type errors detected:\n";
                printErrors(test.code);
            }

            failed++;
        }
    }

    std::cout << "\n";
    std::cout << "=========================================\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "=========================================\n";

    return (failed == 0) ? 0 : 1;
}
