#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <variant>
#include "yen/ast.h"

namespace yen {

// ============================================================================
// TYPE SYSTEM
// ============================================================================

enum class PrimitiveType {
    // Integers
    Int8, Int16, Int32, Int64,
    UInt8, UInt16, UInt32, UInt64,

    // Floating point
    Float32, Float64,

    // Other primitives
    Bool,
    Char,
    String,

    // Special
    Void,
    Unknown
};

// Forward declarations
struct Type;
struct FunctionType;
struct StructType;
struct ArrayType;
struct PointerType;
struct ReferenceType;

using TypePtr = std::shared_ptr<Type>;

// Type variant - can be primitive, function, struct, array, pointer, or reference
struct Type {
    virtual ~Type() = default;
    virtual std::string toString() const = 0;
    virtual bool equals(const Type& other) const = 0;
};

// Primitive type (int32, float64, bool, etc.)
struct PrimitiveTypeNode : Type {
    PrimitiveType primitiveType;

    explicit PrimitiveTypeNode(PrimitiveType pt) : primitiveType(pt) {}

    std::string toString() const override {
        switch (primitiveType) {
            case PrimitiveType::Int8: return "int8";
            case PrimitiveType::Int16: return "int16";
            case PrimitiveType::Int32: return "int32";
            case PrimitiveType::Int64: return "int64";
            case PrimitiveType::UInt8: return "uint8";
            case PrimitiveType::UInt16: return "uint16";
            case PrimitiveType::UInt32: return "uint32";
            case PrimitiveType::UInt64: return "uint64";
            case PrimitiveType::Float32: return "float32";
            case PrimitiveType::Float64: return "float64";
            case PrimitiveType::Bool: return "bool";
            case PrimitiveType::Char: return "char";
            case PrimitiveType::String: return "string";
            case PrimitiveType::Void: return "void";
            case PrimitiveType::Unknown: return "unknown";
            default: return "unknown";
        }
    }

    bool equals(const Type& other) const override {
        if (auto* prim = dynamic_cast<const PrimitiveTypeNode*>(&other)) {
            return primitiveType == prim->primitiveType;
        }
        return false;
    }
};

// Function type: (T1, T2, ..., Tn) -> R
struct FunctionType : Type {
    std::vector<TypePtr> paramTypes;
    TypePtr returnType;
    bool isVarArg;  // Supports variable arguments (...)

    FunctionType(std::vector<TypePtr> params, TypePtr ret, bool vararg = false)
        : paramTypes(std::move(params)), returnType(std::move(ret)), isVarArg(vararg) {}

    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) result += ", ";
            result += paramTypes[i]->toString();
        }
        if (isVarArg) {
            if (!paramTypes.empty()) result += ", ";
            result += "...";
        }
        result += ") -> " + returnType->toString();
        return result;
    }

    bool equals(const Type& other) const override {
        if (auto* func = dynamic_cast<const FunctionType*>(&other)) {
            if (paramTypes.size() != func->paramTypes.size()) return false;
            for (size_t i = 0; i < paramTypes.size(); ++i) {
                if (!paramTypes[i]->equals(*func->paramTypes[i])) return false;
            }
            return returnType->equals(*func->returnType);
        }
        return false;
    }
};

// Struct type
struct StructType : Type {
    std::string name;
    std::unordered_map<std::string, TypePtr> fields;

    StructType(std::string n, std::unordered_map<std::string, TypePtr> f)
        : name(std::move(n)), fields(std::move(f)) {}

    std::string toString() const override {
        return "struct " + name;
    }

    bool equals(const Type& other) const override {
        if (auto* st = dynamic_cast<const StructType*>(&other)) {
            return name == st->name; // Nominal typing
        }
        return false;
    }
};

// Array type: [T; N] for fixed size or [T] for dynamic
struct ArrayType : Type {
    TypePtr elementType;
    std::optional<size_t> size; // None = dynamic array

    ArrayType(TypePtr elem, std::optional<size_t> sz = std::nullopt)
        : elementType(std::move(elem)), size(sz) {}

    std::string toString() const override {
        if (size.has_value()) {
            return "[" + elementType->toString() + "; " + std::to_string(*size) + "]";
        }
        return "[" + elementType->toString() + "]";
    }

    bool equals(const Type& other) const override {
        if (auto* arr = dynamic_cast<const ArrayType*>(&other)) {
            return elementType->equals(*arr->elementType) && size == arr->size;
        }
        return false;
    }
};

// Pointer type: *T or *mut T
struct PointerType : Type {
    TypePtr pointeeType;
    bool isMutable;

    PointerType(TypePtr pointee, bool mut = false)
        : pointeeType(std::move(pointee)), isMutable(mut) {}

    std::string toString() const override {
        return std::string("*") + (isMutable ? "mut " : "") + pointeeType->toString();
    }

    bool equals(const Type& other) const override {
        if (auto* ptr = dynamic_cast<const PointerType*>(&other)) {
            return pointeeType->equals(*ptr->pointeeType) && isMutable == ptr->isMutable;
        }
        return false;
    }
};

// Reference type: &T or &mut T
struct ReferenceType : Type {
    TypePtr referentType;
    bool isMutable;

    ReferenceType(TypePtr referent, bool mut = false)
        : referentType(std::move(referent)), isMutable(mut) {}

    std::string toString() const override {
        return std::string("&") + (isMutable ? "mut " : "") + referentType->toString();
    }

    bool equals(const Type& other) const override {
        if (auto* ref = dynamic_cast<const ReferenceType*>(&other)) {
            return referentType->equals(*ref->referentType) && isMutable == ref->isMutable;
        }
        return false;
    }
};

// ============================================================================
// TYPE ANNOTATIONS (for AST nodes)
// ============================================================================

struct TypeAnnotation {
    TypePtr type;

    TypeAnnotation() : type(nullptr) {}
    explicit TypeAnnotation(TypePtr t) : type(std::move(t)) {}

    bool hasType() const { return type != nullptr; }
    std::string toString() const {
        return type ? type->toString() : "untyped";
    }
};

// ============================================================================
// TYPE ENVIRONMENT
// ============================================================================

class TypeEnvironment {
public:
    std::unordered_map<std::string, TypePtr> variables;
    std::shared_ptr<TypeEnvironment> parent;

    TypeEnvironment() : parent(nullptr) {}
    explicit TypeEnvironment(std::shared_ptr<TypeEnvironment> p) : parent(std::move(p)) {}

    void define(const std::string& name, TypePtr type) {
        variables[name] = std::move(type);
    }

    TypePtr lookup(const std::string& name) const {
        auto it = variables.find(name);
        if (it != variables.end()) {
            return it->second;
        }
        if (parent) {
            return parent->lookup(name);
        }
        return nullptr;
    }

    bool exists(const std::string& name) const {
        return variables.find(name) != variables.end() || (parent && parent->exists(name));
    }
};

// ============================================================================
// TYPE CHECKER
// ============================================================================

class TypeChecker {
private:
    std::shared_ptr<TypeEnvironment> environment;
    std::unordered_map<std::string, TypePtr> structTypes;
    std::vector<std::string> errors;

public:
    TypeChecker();

    // Main entry point
    bool check(const std::vector<std::unique_ptr<Statement>>& statements);

    // Type checking for statements
    void checkStatement(const Statement* stmt);
    void checkLetStmt(const LetStmt* stmt);
    void checkAssignStmt(const AssignStmt* stmt);
    void checkFunctionStmt(const FunctionStmt* stmt);
    void checkExternBlock(const ExternBlock* block);
    void checkStructStmt(const StructStmt* stmt);
    void checkReturnStmt(const ReturnStmt* stmt);
    void checkIfStmt(const IfStmt* stmt);
    void checkWhileStmt(const WhileStmt* stmt);
    void checkForStmt(const ForStmt* stmt);
    void checkBlockStmt(const BlockStmt* stmt);
    void checkBreakStmt(const BreakStmt* stmt);
    void checkContinueStmt(const ContinueStmt* stmt);

    // Type inference for expressions
    TypePtr inferType(const Expression* expr);
    TypePtr inferNumberExpr(const NumberExpr* expr);
    TypePtr inferLiteralExpr(const LiteralExpr* expr);
    TypePtr inferVariableExpr(const VariableExpr* expr);
    TypePtr inferBinaryExpr(const BinaryExpr* expr);
    TypePtr inferUnaryExpr(const UnaryExpr* expr);
    TypePtr inferCallExpr(const CallExpr* expr);
    TypePtr inferListExpr(const ListExpr* expr);
    TypePtr inferIndexExpr(const IndexExpr* expr);

    // Type parsing from string (for annotations)
    TypePtr parseType(const std::string& typeStr);

    // Type compatibility
    bool isAssignable(const TypePtr& from, const TypePtr& to);
    bool isNumeric(const TypePtr& type);
    bool isComparable(const TypePtr& type);

    // Error handling
    void reportError(const std::string& message);
    const std::vector<std::string>& getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

    // Utility
    static TypePtr makePrimitive(PrimitiveType pt);
    static TypePtr makeFunction(std::vector<TypePtr> params, TypePtr ret, bool vararg = false);
    static TypePtr makeArray(TypePtr elem, std::optional<size_t> size = std::nullopt);
    static TypePtr makePointer(TypePtr pointee, bool mut = false);
    static TypePtr makeReference(TypePtr referent, bool mut = false);

private:
    void enterScope();
    void exitScope();
    TypePtr currentFunctionReturnType; // Track return type for current function
    int loopDepth; // Track nesting depth of loops for break/continue validation
};

} // namespace yen

#endif // TYPE_CHECKER_H
