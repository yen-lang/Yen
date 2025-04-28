#ifndef COMPILER_H
#define COMPILER_H

#pragma once

#include "ast.h"
#include "value.h"
#include <unordered_map>
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
        throw std::runtime_error("Variável '" + name + "' não definida.");
    }

    void assign(const std::string& name, const Value& value) {
        if (values.count(name)) {
            values[name] = value;
            return;
        }
        throw std::runtime_error("Tentativa de atribuir a variável não declarada '" + name + "'.");
    }
};

class Interpreter {
public:
    void execute(const std::vector<std::unique_ptr<Statement>>& statements);
    void execute(const Statement* stmt);

private:
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, const FunctionStmt*> functions;
    std::unordered_map<std::string, StructStmt*> structs;
    std::unordered_map<std::string, ClassStmt*> classes;
    std::shared_ptr<Environment> environment = std::make_shared<Environment>();
    Value evalExpr(const Expression* expr);
};

#endif // COMPILER_H
