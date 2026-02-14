#ifndef COMPILER_H
#define COMPILER_H

#pragma once

#include "yen/ast.h"
#include "yen/value.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <functional>

// Signal types for loop control flow (not string-based)
struct BreakSignal {};
struct ContinueSignal {};

class Environment {
public:
    std::unordered_map<std::string, Value> values;

    void define(const std::string& name, const Value& value) {
        values[name] = value;
    }

    Value get(const std::string& name) {
        if (values.count(name)) {
            return values[name];
        }
        throw std::runtime_error("Variable '" + name + "' not defined.");
    }

    void assign(const std::string& name, const Value& value) {
        if (values.count(name)) {
            values[name] = value;
            return;
        }
        throw std::runtime_error("Attempt to assign to undeclared variable '" + name + "'.");
    }
};

class Interpreter {
public:
    Interpreter();
    void execute(const std::vector<std::unique_ptr<Statement>>& statements);
    void execute(const Statement* stmt);
    void register_module(const std::string& name, std::shared_ptr<Environment> env);

private:
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, const FunctionStmt*> functions;
    std::unordered_map<std::string, StructStmt*> structs;
    std::unordered_map<std::string, ClassStmt*> classes;
    std::unordered_set<std::string> immutableVars;
    std::unordered_set<std::string> importedFiles;  // Track imported files to prevent cycles
    std::shared_ptr<Environment> environment = std::make_shared<Environment>();
    std::unordered_map<std::string, std::shared_ptr<Environment>> modules;
    std::string currentModule;
    std::string currentFile;  // Track current file for relative imports
    std::vector<std::vector<const Statement*>> deferStack;
    // OOP Phase 1: tracking current class for access modifiers
    std::string currentClassName;
    // Traits: trait name → required method names
    std::unordered_map<std::string, std::vector<std::string>> traits;
    // Trait default methods: "TraitName.method" → FunctionStmt*
    std::unordered_map<std::string, const FunctionStmt*> traitDefaultMethods;
    // Class→traits associations: className → set of trait names
    std::unordered_map<std::string, std::vector<std::string>> classTraits;
    // Access modifiers: "ClassName.fieldOrMethod" → AccessModifier
    std::unordered_map<std::string, AccessModifier> accessModifiers;
    // Extension methods: "TypeName.method" → FunctionStmt*
    std::unordered_map<std::string, const FunctionStmt*> extensionMethods;
    // Static fields: "ClassName.field" → Value
    // (stored in variables with "ClassName.field" key)
    // Getters/setters: "ClassName.get_prop" / "ClassName.set_prop"
    // (stored in functions map)
    // Sealed classes: className → source file
    std::unordered_map<std::string, std::string> sealedClasses;
    // Native module registry: maps dotted paths (e.g. "net.http") to initializer functions
    std::unordered_map<std::string, std::function<void(std::unordered_map<std::string, Value>&)>> nativeModules;
    std::unordered_set<std::string> loadedNativeModules;  // Track which native modules are loaded
    void initNativeModuleRegistry();
    bool loadNativeModule(const std::string& modulePath);
    Value evalExpr(const Expression* expr);
    Value call(const Value& callee, std::vector<Value>& args);
    void executeDeferredStatements();
    bool matchPattern(const Pattern* pattern, const Value& value, std::unordered_map<std::string, Value>& bindings);
    std::string valueToString(const Value& val);
    bool isTruthy(const Value& val);
};

#endif // COMPILER_H
