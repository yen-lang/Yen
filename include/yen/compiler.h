#ifndef COMPILER_H
#define COMPILER_H

#pragma once

#include "yen/ast.h"
#include "yen/value.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

inline const std::string BREAK_SIGNAL = "__BREAK__";
inline const std::string CONTINUE_SIGNAL = "__CONTINUE__";

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
    Interpreter();  // Constructor to register native libraries
    void execute(const std::vector<std::unique_ptr<Statement>>& statements);
    void execute(const Statement* stmt);
    void register_module(const std::string& name, std::shared_ptr<Environment> env);

private:
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, const FunctionStmt*> functions;
    std::unordered_map<std::string, StructStmt*> structs;
    std::unordered_map<std::string, ClassStmt*> classes;
    std::unordered_set<std::string> immutableVars;  // Track immutable variables (let/const)
    std::shared_ptr<Environment> environment = std::make_shared<Environment>();
    std::unordered_map<std::string, std::shared_ptr<Environment>> modules;
    std::string currentModule;
    std::vector<std::vector<const Statement*>> deferStack;  // Stack of defer statements per scope
    Value evalExpr(const Expression* expr);
    Value call(const Value& callee, const std::vector<Value>& args);
    void executeDeferredStatements();  // Execute deferred statements in LIFO order

    // Pattern matching
    bool matchPattern(const Pattern* pattern, const Value& value, std::unordered_map<std::string, Value>& bindings);
};

#endif // COMPILER_H
