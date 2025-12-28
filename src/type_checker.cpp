#include "yen/type_checker.h"
#include <iostream>
#include <sstream>

namespace yen {

// ============================================================================
// TYPE CHECKER IMPLEMENTATION
// ============================================================================

TypeChecker::TypeChecker()
    : environment(std::make_shared<TypeEnvironment>()),
      currentFunctionReturnType(nullptr),
      loopDepth(0) {

    // Register builtin types and functions
    // String functions
    environment->define("split", makeFunction(
        {makePrimitive(PrimitiveType::String), makePrimitive(PrimitiveType::String)},
        makeArray(makePrimitive(PrimitiveType::String))
    ));
    environment->define("join", makeFunction(
        {makeArray(makePrimitive(PrimitiveType::String)), makePrimitive(PrimitiveType::String)},
        makePrimitive(PrimitiveType::String)
    ));
    environment->define("toUpper", makeFunction(
        {makePrimitive(PrimitiveType::String)},
        makePrimitive(PrimitiveType::String)
    ));
    environment->define("toLower", makeFunction(
        {makePrimitive(PrimitiveType::String)},
        makePrimitive(PrimitiveType::String)
    ));
    environment->define("substring", makeFunction(
        {makePrimitive(PrimitiveType::String), makePrimitive(PrimitiveType::Int32), makePrimitive(PrimitiveType::Int32)},
        makePrimitive(PrimitiveType::String)
    ));

    // Type conversion functions
    environment->define("str", makeFunction(
        {makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::String)
    ));
    environment->define("int", makeFunction(
        {makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::Int32)
    ));
    environment->define("float", makeFunction(
        {makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::Float64)
    ));

    // Utility functions
    environment->define("len", makeFunction(
        {makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::Int32)
    ));
    environment->define("type", makeFunction(
        {makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::String)
    ));
    environment->define("range", makeFunction(
        {makePrimitive(PrimitiveType::Int32)},
        makeArray(makePrimitive(PrimitiveType::Int32))
    ));

    // List functions
    environment->define("push", makeFunction(
        {makeArray(makePrimitive(PrimitiveType::Unknown)), makePrimitive(PrimitiveType::Unknown)},
        makeArray(makePrimitive(PrimitiveType::Unknown))
    ));
    environment->define("sort", makeFunction(
        {makeArray(makePrimitive(PrimitiveType::Unknown))},
        makeArray(makePrimitive(PrimitiveType::Unknown))
    ));
    environment->define("contains", makeFunction(
        {makeArray(makePrimitive(PrimitiveType::Unknown)), makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::Bool)
    ));

    // Print function
    environment->define("print", makeFunction(
        {makePrimitive(PrimitiveType::Unknown)},
        makePrimitive(PrimitiveType::Void)
    ));
}

bool TypeChecker::check(const std::vector<std::unique_ptr<Statement>>& statements) {
    errors.clear();

    for (const auto& stmt : statements) {
        checkStatement(stmt.get());
    }

    return !hasErrors();
}

void TypeChecker::checkStatement(const Statement* stmt) {
    if (!stmt) return;

    if (auto* let = dynamic_cast<const LetStmt*>(stmt)) {
        checkLetStmt(let);
    } else if (auto* assign = dynamic_cast<const AssignStmt*>(stmt)) {
        checkAssignStmt(assign);
    } else if (auto* func = dynamic_cast<const FunctionStmt*>(stmt)) {
        checkFunctionStmt(func);
    } else if (auto* externBlock = dynamic_cast<const ExternBlock*>(stmt)) {
        checkExternBlock(externBlock);
    } else if (auto* structStmt = dynamic_cast<const StructStmt*>(stmt)) {
        checkStructStmt(structStmt);
    } else if (auto* ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        checkReturnStmt(ret);
    } else if (auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        checkIfStmt(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        checkWhileStmt(whileStmt);
    } else if (auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        checkForStmt(forStmt);
    } else if (auto* block = dynamic_cast<const BlockStmt*>(stmt)) {
        checkBlockStmt(block);
    } else if (auto* exprStmt = dynamic_cast<const ExpressionStmt*>(stmt)) {
        inferType(exprStmt->expression.get());
    } else if (auto* printStmt = dynamic_cast<const PrintStmt*>(stmt)) {
        inferType(printStmt->expression.get());
    } else if (auto* breakStmt = dynamic_cast<const BreakStmt*>(stmt)) {
        checkBreakStmt(breakStmt);
    } else if (auto* continueStmt = dynamic_cast<const ContinueStmt*>(stmt)) {
        checkContinueStmt(continueStmt);
    }
    // Add more statement types as needed
}

void TypeChecker::checkLetStmt(const LetStmt* stmt) {
    TypePtr exprType = inferType(stmt->expression.get());

    // Check if user provided explicit type annotation
    if (stmt->typeAnnotation.has_value()) {
        TypePtr annotatedType = parseType(stmt->typeAnnotation.value());

        if (exprType && !isAssignable(exprType, annotatedType)) {
            reportError("Type mismatch in variable '" + stmt->name + "': declared as " +
                       annotatedType->toString() + " but initialized with " +
                       exprType->toString());
        }

        // Use the annotated type (user's explicit declaration)
        environment->define(stmt->name, annotatedType);
    } else {
        // No annotation - use inferred type
        if (exprType) {
            environment->define(stmt->name, exprType);
        }
    }
}

void TypeChecker::checkAssignStmt(const AssignStmt* stmt) {
    TypePtr varType = environment->lookup(stmt->name);
    if (!varType) {
        reportError("Undefined variable '" + stmt->name + "'");
        return;
    }

    TypePtr exprType = inferType(stmt->expression.get());
    if (exprType && !isAssignable(exprType, varType)) {
        reportError("Type mismatch in assignment: cannot assign " +
                   exprType->toString() + " to " + varType->toString());
    }
}

void TypeChecker::checkFunctionStmt(const FunctionStmt* stmt) {
    // Parse parameter types from annotations (or default to int32)
    std::vector<TypePtr> paramTypes;
    for (size_t i = 0; i < stmt->parameters.size(); ++i) {
        if (i < stmt->parameterTypes.size() && !stmt->parameterTypes[i].empty()) {
            // User provided type annotation - parse it
            paramTypes.push_back(parseType(stmt->parameterTypes[i]));
        } else {
            // No annotation - default to int32 for backward compatibility
            paramTypes.push_back(makePrimitive(PrimitiveType::Int32));
        }
    }

    // Parse return type from annotation (or default to int32)
    TypePtr returnType;
    if (!stmt->returnType.empty()) {
        returnType = parseType(stmt->returnType);
    } else {
        // No annotation - default to int32
        returnType = makePrimitive(PrimitiveType::Int32);
    }

    // Register function type
    TypePtr funcType = makeFunction(paramTypes, returnType);
    environment->define(stmt->name, funcType);

    // Check function body in new scope
    enterScope();

    // Define parameters in function scope with their annotated types
    for (size_t i = 0; i < stmt->parameters.size(); ++i) {
        environment->define(stmt->parameters[i], paramTypes[i]);
    }

    // Set current function return type for return statement checking
    auto prevReturnType = currentFunctionReturnType;
    currentFunctionReturnType = returnType;

    checkStatement(stmt->body.get());

    currentFunctionReturnType = prevReturnType;
    exitScope();
}

void TypeChecker::checkExternBlock(const ExternBlock* block) {
    // Register all extern functions in the environment
    for (const auto& funcDecl : block->functions) {
        // Parse parameter types
        std::vector<TypePtr> paramTypes;
        for (const auto& paramType : funcDecl->parameterTypes) {
            paramTypes.push_back(parseType(paramType));
        }

        // Parse return type
        TypePtr returnType = parseType(funcDecl->returnType);

        // Register function type with varargs support
        TypePtr funcType = makeFunction(paramTypes, returnType, funcDecl->isVarArg);
        environment->define(funcDecl->name, funcType);
    }
}

void TypeChecker::checkStructStmt(const StructStmt* stmt) {
    // Register struct type
    std::unordered_map<std::string, TypePtr> fields;
    for (const auto& field : stmt->fields) {
        // Default to int32 for now (needs parser support for type annotations)
        fields[field] = makePrimitive(PrimitiveType::Int32);
    }

    auto structType = std::make_shared<StructType>(stmt->name, fields);
    structTypes[stmt->name] = structType;
}

void TypeChecker::checkReturnStmt(const ReturnStmt* stmt) {
    if (!currentFunctionReturnType) {
        reportError("Return statement outside of function");
        return;
    }

    TypePtr exprType = inferType(stmt->value.get());
    if (exprType && !isAssignable(exprType, currentFunctionReturnType)) {
        reportError("Return type mismatch: expected " +
                   currentFunctionReturnType->toString() + " but got " +
                   exprType->toString());
    }
}

void TypeChecker::checkIfStmt(const IfStmt* stmt) {
    TypePtr condType = inferType(stmt->condition.get());
    if (condType && !condType->equals(*makePrimitive(PrimitiveType::Bool))) {
        reportError("If condition must be boolean, got " + condType->toString());
    }

    checkStatement(stmt->thenBranch.get());
    if (stmt->elseBranch) {
        checkStatement(stmt->elseBranch.get());
    }
}

void TypeChecker::checkWhileStmt(const WhileStmt* stmt) {
    TypePtr condType = inferType(stmt->condition.get());
    if (condType && !condType->equals(*makePrimitive(PrimitiveType::Bool))) {
        reportError("While condition must be boolean, got " + condType->toString());
    }

    loopDepth++;  // Enter loop
    checkStatement(stmt->body.get());
    loopDepth--;  // Exit loop
}

void TypeChecker::checkForStmt(const ForStmt* stmt) {
    TypePtr iterableType = inferType(stmt->iterable.get());

    enterScope();

    // Infer element type from iterable
    if (auto* arrayType = dynamic_cast<ArrayType*>(iterableType.get())) {
        environment->define(stmt->var, arrayType->elementType);
    } else {
        // Assume int32 for now
        environment->define(stmt->var, makePrimitive(PrimitiveType::Int32));
    }

    loopDepth++;  // Enter loop
    checkStatement(stmt->body.get());
    loopDepth--;  // Exit loop

    exitScope();
}

void TypeChecker::checkBreakStmt(const BreakStmt* stmt) {
    (void)stmt;  // Suppress unused parameter warning
    if (loopDepth == 0) {
        reportError("'break' statement used outside of loop");
    }
}

void TypeChecker::checkContinueStmt(const ContinueStmt* stmt) {
    (void)stmt;  // Suppress unused parameter warning
    if (loopDepth == 0) {
        reportError("'continue' statement used outside of loop");
    }
}

void TypeChecker::checkBlockStmt(const BlockStmt* stmt) {
    enterScope();

    for (const auto& s : stmt->statements) {
        checkStatement(s.get());
    }

    exitScope();
}

// ============================================================================
// TYPE INFERENCE
// ============================================================================

TypePtr TypeChecker::inferType(const Expression* expr) {
    if (!expr) return nullptr;

    if (auto* num = dynamic_cast<const NumberExpr*>(expr)) {
        return inferNumberExpr(num);
    } else if (auto* lit = dynamic_cast<const LiteralExpr*>(expr)) {
        return inferLiteralExpr(lit);
    } else if (auto* var = dynamic_cast<const VariableExpr*>(expr)) {
        return inferVariableExpr(var);
    } else if (auto* bin = dynamic_cast<const BinaryExpr*>(expr)) {
        return inferBinaryExpr(bin);
    } else if (auto* unary = dynamic_cast<const UnaryExpr*>(expr)) {
        return inferUnaryExpr(unary);
    } else if (auto* call = dynamic_cast<const CallExpr*>(expr)) {
        return inferCallExpr(call);
    } else if (auto* list = dynamic_cast<const ListExpr*>(expr)) {
        return inferListExpr(list);
    } else if (auto* index = dynamic_cast<const IndexExpr*>(expr)) {
        return inferIndexExpr(index);
    } else if (auto* boolExpr = dynamic_cast<const BoolExpr*>(expr)) {
        return makePrimitive(PrimitiveType::Bool);
    }

    return makePrimitive(PrimitiveType::Unknown);
}

TypePtr TypeChecker::inferNumberExpr(const NumberExpr* expr) {
    // Check if it's an integer or float
    if (expr->value == static_cast<int>(expr->value)) {
        return makePrimitive(PrimitiveType::Int32);
    }
    return makePrimitive(PrimitiveType::Float64);
}

TypePtr TypeChecker::inferLiteralExpr(const LiteralExpr* expr) {
    if (expr->value.holds_alternative<int>()) {
        return makePrimitive(PrimitiveType::Int32);
    } else if (expr->value.holds_alternative<double>()) {
        return makePrimitive(PrimitiveType::Float64);
    } else if (expr->value.holds_alternative<float>()) {
        return makePrimitive(PrimitiveType::Float32);
    } else if (expr->value.holds_alternative<bool>()) {
        return makePrimitive(PrimitiveType::Bool);
    } else if (expr->value.holds_alternative<std::string>()) {
        return makePrimitive(PrimitiveType::String);
    } else if (expr->value.holds_alternative<std::vector<Value>>()) {
        // Array type - infer element type from first element if available
        const auto& vec = expr->value.get<std::vector<Value>>();
        if (!vec.empty()) {
            // Create a temporary LiteralExpr to infer element type
            LiteralExpr tempExpr(vec[0]);
            TypePtr elemType = inferLiteralExpr(&tempExpr);
            return makeArray(elemType);
        }
        return makeArray(makePrimitive(PrimitiveType::Unknown));
    }

    return makePrimitive(PrimitiveType::Unknown);
}

TypePtr TypeChecker::inferVariableExpr(const VariableExpr* expr) {
    TypePtr type = environment->lookup(expr->name);
    if (!type) {
        reportError("Undefined variable '" + expr->name + "'");
        return makePrimitive(PrimitiveType::Unknown);
    }
    return type;
}

TypePtr TypeChecker::inferBinaryExpr(const BinaryExpr* expr) {
    TypePtr leftType = inferType(expr->left.get());
    TypePtr rightType = inferType(expr->right.get());

    if (!leftType || !rightType) {
        return makePrimitive(PrimitiveType::Unknown);
    }

    switch (expr->op) {
        case BinaryOp::Add:
            // Special case: + can be string concatenation or numeric addition
            if (leftType->equals(*makePrimitive(PrimitiveType::String)) ||
                rightType->equals(*makePrimitive(PrimitiveType::String))) {
                // String concatenation - return string
                return makePrimitive(PrimitiveType::String);
            }
            // Otherwise, numeric addition
            if (!isNumeric(leftType) || !isNumeric(rightType)) {
                reportError("Addition requires numeric types or strings");
                return makePrimitive(PrimitiveType::Unknown);
            }
            // Return the "larger" numeric type (float64 > float32 > int64 > int32)
            if (leftType->equals(*makePrimitive(PrimitiveType::Float64)) ||
                rightType->equals(*makePrimitive(PrimitiveType::Float64))) {
                return makePrimitive(PrimitiveType::Float64);
            }
            if (leftType->equals(*makePrimitive(PrimitiveType::Float32)) ||
                rightType->equals(*makePrimitive(PrimitiveType::Float32))) {
                return makePrimitive(PrimitiveType::Float32);
            }
            return makePrimitive(PrimitiveType::Int32);

        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
            // Arithmetic operations require numeric types
            if (!isNumeric(leftType) || !isNumeric(rightType)) {
                reportError("Arithmetic operation requires numeric types");
                return makePrimitive(PrimitiveType::Unknown);
            }
            // Return the "larger" type (float64 > float32 > int64 > int32)
            if (leftType->equals(*makePrimitive(PrimitiveType::Float64)) ||
                rightType->equals(*makePrimitive(PrimitiveType::Float64))) {
                return makePrimitive(PrimitiveType::Float64);
            }
            if (leftType->equals(*makePrimitive(PrimitiveType::Float32)) ||
                rightType->equals(*makePrimitive(PrimitiveType::Float32))) {
                return makePrimitive(PrimitiveType::Float32);
            }
            return makePrimitive(PrimitiveType::Int32);

        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
        case BinaryOp::Less:
        case BinaryOp::LessEqual:
        case BinaryOp::Greater:
        case BinaryOp::GreaterEqual:
            // Comparison operations return bool
            if (!isComparable(leftType) || !isComparable(rightType)) {
                reportError("Comparison requires comparable types");
            }
            return makePrimitive(PrimitiveType::Bool);

        case BinaryOp::And:
        case BinaryOp::Or:
            // Logical operations require bool
            if (!leftType->equals(*makePrimitive(PrimitiveType::Bool)) ||
                !rightType->equals(*makePrimitive(PrimitiveType::Bool))) {
                reportError("Logical operation requires boolean types");
            }
            return makePrimitive(PrimitiveType::Bool);

        default:
            return makePrimitive(PrimitiveType::Unknown);
    }
}

TypePtr TypeChecker::inferUnaryExpr(const UnaryExpr* expr) {
    TypePtr rightType = inferType(expr->right.get());

    switch (expr->op) {
        case UnaryOp::Not:
            if (!rightType->equals(*makePrimitive(PrimitiveType::Bool))) {
                reportError("Not operation requires boolean type");
            }
            return makePrimitive(PrimitiveType::Bool);

        default:
            return makePrimitive(PrimitiveType::Unknown);
    }
}

TypePtr TypeChecker::inferCallExpr(const CallExpr* expr) {
    // Get callee type
    TypePtr calleeType = inferType(expr->callee.get());

    if (auto* funcType = dynamic_cast<FunctionType*>(calleeType.get())) {
        // Check argument count
        if (funcType->isVarArg) {
            // Varargs function: must have at least the required parameters
            if (expr->arguments.size() < funcType->paramTypes.size()) {
                reportError("Function expects at least " + std::to_string(funcType->paramTypes.size()) +
                           " arguments but got " + std::to_string(expr->arguments.size()));
            }
        } else {
            // Non-varargs: must match exactly
            if (expr->arguments.size() != funcType->paramTypes.size()) {
                reportError("Function expects " + std::to_string(funcType->paramTypes.size()) +
                           " arguments but got " + std::to_string(expr->arguments.size()));
            }
        }

        // Check argument types for the non-varargs parameters
        for (size_t i = 0; i < expr->arguments.size() && i < funcType->paramTypes.size(); ++i) {
            TypePtr argType = inferType(expr->arguments[i].get());
            if (argType && !isAssignable(argType, funcType->paramTypes[i])) {
                reportError("Argument " + std::to_string(i + 1) + " type mismatch: expected " +
                           funcType->paramTypes[i]->toString() + " but got " +
                           argType->toString());
            }
        }

        // For varargs, infer types of additional arguments but don't validate them
        // (C varargs can accept any type)
        for (size_t i = funcType->paramTypes.size(); i < expr->arguments.size(); ++i) {
            inferType(expr->arguments[i].get());
        }

        return funcType->returnType;
    }

    // Unknown function type - might be native function, return unknown
    return makePrimitive(PrimitiveType::Unknown);
}

TypePtr TypeChecker::inferListExpr(const ListExpr* expr) {
    if (expr->elements.empty()) {
        return makeArray(makePrimitive(PrimitiveType::Unknown));
    }

    // Infer type from first element
    TypePtr elemType = inferType(expr->elements[0].get());

    // Check all elements have compatible types
    for (size_t i = 1; i < expr->elements.size(); ++i) {
        TypePtr type = inferType(expr->elements[i].get());
        if (type && !type->equals(*elemType)) {
            reportError("List elements must have same type");
            break;
        }
    }

    return makeArray(elemType, expr->elements.size());
}

TypePtr TypeChecker::inferIndexExpr(const IndexExpr* expr) {
    TypePtr listType = inferType(expr->listExpr.get());
    TypePtr indexType = inferType(expr->indexExpr.get());

    // Check index is integer
    if (indexType && !indexType->equals(*makePrimitive(PrimitiveType::Int32))) {
        reportError("Index must be integer type");
    }

    // Extract element type from array
    if (auto* arrayType = dynamic_cast<ArrayType*>(listType.get())) {
        return arrayType->elementType;
    }

    return makePrimitive(PrimitiveType::Unknown);
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

TypePtr TypeChecker::parseType(const std::string& typeStr) {
    // Integer types
    if (typeStr == "int8") return makePrimitive(PrimitiveType::Int8);
    if (typeStr == "int16") return makePrimitive(PrimitiveType::Int16);
    if (typeStr == "int32" || typeStr == "int") return makePrimitive(PrimitiveType::Int32);
    if (typeStr == "int64") return makePrimitive(PrimitiveType::Int64);

    // Unsigned integer types
    if (typeStr == "uint8") return makePrimitive(PrimitiveType::UInt8);
    if (typeStr == "uint16") return makePrimitive(PrimitiveType::UInt16);
    if (typeStr == "uint32") return makePrimitive(PrimitiveType::UInt32);
    if (typeStr == "uint64") return makePrimitive(PrimitiveType::UInt64);

    // Floating point types
    if (typeStr == "float32") return makePrimitive(PrimitiveType::Float32);
    if (typeStr == "float64" || typeStr == "float") return makePrimitive(PrimitiveType::Float64);

    // Other primitive types
    if (typeStr == "bool") return makePrimitive(PrimitiveType::Bool);
    if (typeStr == "char") return makePrimitive(PrimitiveType::Char);
    if (typeStr == "string" || typeStr == "str") return makePrimitive(PrimitiveType::String);
    if (typeStr == "void") return makePrimitive(PrimitiveType::Void);

    // Check for array types [T] or [T; N]
    // TODO: Implement array type parsing

    return makePrimitive(PrimitiveType::Unknown);
}

bool TypeChecker::isAssignable(const TypePtr& from, const TypePtr& to) {
    if (!from || !to) return false;

    // Same type is always assignable
    if (from->equals(*to)) return true;

    // Unknown type is assignable to/from anything (gradual typing)
    auto unknownType = makePrimitive(PrimitiveType::Unknown);
    if (from->equals(*unknownType) || to->equals(*unknownType)) {
        return true;
    }

    // Numeric type coercions
    auto fromPrim = dynamic_cast<const PrimitiveTypeNode*>(from.get());
    auto toPrim = dynamic_cast<const PrimitiveTypeNode*>(to.get());

    if (fromPrim && toPrim) {
        // Allow int to float coercion
        if (isNumeric(from) && isNumeric(to)) {
            return true;
        }
    }

    return false;
}

bool TypeChecker::isNumeric(const TypePtr& type) {
    if (!type) return false;

    auto prim = dynamic_cast<const PrimitiveTypeNode*>(type.get());
    if (!prim) return false;

    switch (prim->primitiveType) {
        case PrimitiveType::Int8:
        case PrimitiveType::Int16:
        case PrimitiveType::Int32:
        case PrimitiveType::Int64:
        case PrimitiveType::UInt8:
        case PrimitiveType::UInt16:
        case PrimitiveType::UInt32:
        case PrimitiveType::UInt64:
        case PrimitiveType::Float32:
        case PrimitiveType::Float64:
            return true;
        default:
            return false;
    }
}

bool TypeChecker::isComparable(const TypePtr& type) {
    if (!type) return false;

    // Numbers, bool, char, string are comparable
    if (isNumeric(type)) return true;

    auto prim = dynamic_cast<const PrimitiveTypeNode*>(type.get());
    if (!prim) return false;

    switch (prim->primitiveType) {
        case PrimitiveType::Bool:
        case PrimitiveType::Char:
        case PrimitiveType::String:
            return true;
        default:
            return false;
    }
}

void TypeChecker::reportError(const std::string& message) {
    errors.push_back("Type error: " + message);
}

TypePtr TypeChecker::makePrimitive(PrimitiveType pt) {
    return std::make_shared<PrimitiveTypeNode>(pt);
}

TypePtr TypeChecker::makeFunction(std::vector<TypePtr> params, TypePtr ret, bool vararg) {
    return std::make_shared<FunctionType>(std::move(params), std::move(ret), vararg);
}

TypePtr TypeChecker::makeArray(TypePtr elem, std::optional<size_t> size) {
    return std::make_shared<ArrayType>(std::move(elem), size);
}

TypePtr TypeChecker::makePointer(TypePtr pointee, bool mut) {
    return std::make_shared<PointerType>(std::move(pointee), mut);
}

TypePtr TypeChecker::makeReference(TypePtr referent, bool mut) {
    return std::make_shared<ReferenceType>(std::move(referent), mut);
}

void TypeChecker::enterScope() {
    environment = std::make_shared<TypeEnvironment>(environment);
}

void TypeChecker::exitScope() {
    if (environment->parent) {
        environment = environment->parent;
    }
}

} // namespace yen
