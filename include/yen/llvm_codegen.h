#ifndef LLVM_CODEGEN_H
#define LLVM_CODEGEN_H

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#ifdef HAVE_LLVM
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#endif

#include "yen/ast.h"
#include "yen/type_checker.h"

namespace yen {

#ifdef HAVE_LLVM

// ============================================================================
// LLVM CODE GENERATOR
// ============================================================================

class LLVMCodeGen {
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::Module> module;

    // Symbol table: variable name -> LLVM value
    std::unordered_map<std::string, llvm::Value*> namedValues;

    // Type checker for type information
    TypeChecker* typeChecker;

    // Current function being compiled
    llvm::Function* currentFunction;

    // Debug information
    std::unique_ptr<llvm::DIBuilder> debugBuilder;
    llvm::DICompileUnit* debugCompileUnit;
    llvm::DIFile* debugFile;
    llvm::DIScope* debugScope;
    std::string sourceFilename;
    bool enableDebugInfo;

public:
    LLVMCodeGen(const std::string& moduleName, TypeChecker* tc);
    ~LLVMCodeGen();

    // Main code generation entry point
    bool generate(const std::vector<std::unique_ptr<Statement>>& statements);

    // Statement code generation
    void codegen(const Statement* stmt);
    void codegenLetStmt(const LetStmt* stmt);
    void codegenAssignStmt(const AssignStmt* stmt);
    void codegenFunctionStmt(const FunctionStmt* stmt);
    void codegenReturnStmt(const ReturnStmt* stmt);
    void codegenIfStmt(const IfStmt* stmt);
    void codegenWhileStmt(const WhileStmt* stmt);
    void codegenForStmt(const ForStmt* stmt);
    void codegenBlockStmt(const BlockStmt* stmt);
    void codegenPrintStmt(const PrintStmt* stmt);
    void codegenExprStmt(const ExpressionStmt* stmt);
    void codegenExternBlock(const ExternBlock* stmt);

    // Expression code generation
    llvm::Value* codegen(const Expression* expr);
    llvm::Value* codegenNumberExpr(const NumberExpr* expr);
    llvm::Value* codegenLiteralExpr(const LiteralExpr* expr);
    llvm::Value* codegenVariableExpr(const VariableExpr* expr);
    llvm::Value* codegenBinaryExpr(const BinaryExpr* expr);
    llvm::Value* codegenUnaryExpr(const UnaryExpr* expr);
    llvm::Value* codegenCallExpr(const CallExpr* expr);
    llvm::Value* codegenListExpr(const ListExpr* expr);
    llvm::Value* codegenIndexExpr(const IndexExpr* expr);
    llvm::Value* codegenBoolExpr(const BoolExpr* expr);

    // Type conversion: YEN type -> LLVM type
    llvm::Type* getLLVMType(const TypePtr& yenType);

    // Output generation
    void emitLLVMIR(const std::string& filename);
    void emitObjectFile(const std::string& filename);
    void emitExecutable(const std::string& filename);

    // Optimization
    void optimize(int level = 2); // 0 = none, 1 = basic, 2 = default, 3 = aggressive

    // Utility
    void printModule();
    bool verifyModule();

    // Debug information
    void setSourceFile(const std::string& filename);
    void setDebugInfo(bool enable);
    void finalizeDebugInfo();

private:
    // Helper functions
    llvm::Function* createPrintfFunction();
    llvm::Function* createMallocFunction();
    llvm::Function* createFreeFunction();
    llvm::Type* getLLVMTypeFromString(const std::string& typeStr);

    // Error handling
    void reportError(const std::string& message);
    std::vector<std::string> errors;
};

#else // !HAVE_LLVM

// Stub implementation when LLVM is not available
class LLVMCodeGen {
public:
    LLVMCodeGen(const std::string& moduleName, TypeChecker* tc) {
        (void)moduleName;
        (void)tc;
    }

    bool generate(const std::vector<std::unique_ptr<Statement>>& statements) {
        (void)statements;
        std::cerr << "Error: YEN was not compiled with LLVM support.\n";
        std::cerr << "Please install LLVM and rebuild to use the compiler.\n";
        std::cerr << "See LLVM_SETUP.md for installation instructions.\n";
        return false;
    }

    void emitLLVMIR(const std::string& filename) {
        (void)filename;
        std::cerr << "Error: LLVM support not available.\n";
    }

    void emitObjectFile(const std::string& filename) {
        (void)filename;
        std::cerr << "Error: LLVM support not available.\n";
    }

    void emitExecutable(const std::string& filename) {
        (void)filename;
        std::cerr << "Error: LLVM support not available.\n";
    }

    void optimize(int level = 2) {
        (void)level;
    }

    void printModule() {
        std::cerr << "Error: LLVM support not available.\n";
    }

    bool verifyModule() {
        return false;
    }
};

#endif // HAVE_LLVM

} // namespace yen

#endif // LLVM_CODEGEN_H
