#ifndef AST_H
#define AST_H

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <variant>
#include "yen/value.h"

// Source location for error reporting and debug info
struct SourceLocation {
    int line;
    int column;

    SourceLocation() : line(0), column(0) {}
    SourceLocation(int l, int c) : line(l), column(c) {}
};

// Forward declare Expressions and Statements
struct NumberExpr; struct LiteralExpr; struct InputExpr; struct VariableExpr;
struct BinaryExpr; struct BoolExpr; struct UnaryExpr; struct CallExpr;
struct ListExpr; struct IndexExpr; struct CastExpr; struct LambdaExpr; struct RangeExpr;
struct MapExpr; struct PipeExpr; struct TernaryExpr; struct NullCoalesceExpr; struct SpreadExpr;
struct SliceExpr;
struct SuperExpr;
struct IsExpr;
struct OptionalGetExpr;
struct ChainedComparisonExpr;

struct PrintStmt; struct LetStmt; struct ConstStmt; struct AssignStmt; struct StructStmt;
struct IfStmt; struct BlockStmt; struct FunctionStmt; struct ReturnStmt;
struct ExpressionStmt; struct IndexAssignStmt; struct ForStmt; struct WhileStmt; struct LoopStmt;
struct BreakStmt; struct ContinueStmt; struct EnumStmt; struct MatchStmt;
struct SwitchStmt; struct ClassStmt; struct SetStmt; struct GetExpr;
struct ThisExpr;
struct ImportStmt;
struct ExportStmt;
struct ExternFunctionDecl;
struct ExternBlock;
struct DeferStmt; struct AssertStmt;
struct TryCatchStmt; struct ThrowStmt;
struct CompoundAssignStmt;
struct DoWhileStmt;
struct DestructureLetStmt;
struct GoStmt;
struct IncrementStmt;
struct ForDestructureStmt;
struct TraitStmt;
struct ImplStmt;
struct RepeatStmt;
struct ExtendStmt;
struct ObjectDestructureLetStmt;

// Visitors
class Visitor {
public:
    virtual void visit(NumberExpr&) = 0;
    virtual void visit(LiteralExpr&) = 0;
    virtual void visit(InputExpr&) = 0;
    virtual void visit(VariableExpr&) = 0;
    virtual void visit(BinaryExpr&) = 0;
    virtual void visit(BoolExpr&) = 0;
    virtual void visit(UnaryExpr&) = 0;
    virtual void visit(CallExpr&) = 0;
    virtual void visit(ListExpr&) = 0;
    virtual void visit(IndexExpr&) = 0;
    virtual void visit(GetExpr&) = 0;
    virtual void visit(ThisExpr&) = 0;
    virtual void visit(ChainedComparisonExpr&) {}  // default no-op
};

class StatementVisitor {
public:
    virtual void visit(const PrintStmt&) = 0;
    virtual void visit(const LetStmt&) = 0;
    virtual void visit(const ConstStmt&) = 0;
    virtual void visit(const AssignStmt&) = 0;
    virtual void visit(const CompoundAssignStmt&) = 0;
    virtual void visit(const SetStmt&) = 0;
    virtual void visit(const StructStmt&) = 0;
    virtual void visit(const IfStmt&) = 0;
    virtual void visit(const BlockStmt&) = 0;
    virtual void visit(const FunctionStmt&) = 0;
    virtual void visit(const ReturnStmt&) = 0;
    virtual void visit(const ExternBlock&) = 0;
    virtual void visit(const ExpressionStmt&) = 0;
    virtual void visit(const IndexAssignStmt&) = 0;
    virtual void visit(const ForStmt&) = 0;
    virtual void visit(const WhileStmt&) = 0;
    virtual void visit(const LoopStmt&) = 0;
    virtual void visit(const BreakStmt&) = 0;
    virtual void visit(const ContinueStmt&) = 0;
    virtual void visit(const EnumStmt&) = 0;
    virtual void visit(const MatchStmt&) = 0;
    virtual void visit(const SwitchStmt&) = 0;
    virtual void visit(const ClassStmt&) = 0;
    virtual void visit(const ImportStmt&) = 0;
    virtual void visit(const ExportStmt&) = 0;
    virtual void visit(const DeferStmt&) = 0;
    virtual void visit(const AssertStmt&) = 0;
    virtual void visit(const TryCatchStmt&) = 0;
    virtual void visit(const ThrowStmt&) = 0;
    virtual void visit(const DoWhileStmt&) = 0;
    virtual void visit(const DestructureLetStmt&) = 0;
    virtual void visit(const GoStmt&) = 0;
    virtual void visit(const IncrementStmt&) = 0;
    virtual void visit(const ForDestructureStmt&) = 0;
    virtual void visit(const TraitStmt&) = 0;
    virtual void visit(const ImplStmt&) = 0;
    virtual void visit(const RepeatStmt&) = 0;
    virtual void visit(const ExtendStmt&) = 0;
    virtual void visit(const ObjectDestructureLetStmt&) = 0;
    virtual ~StatementVisitor() = default;
};

// ---------------- Patterns ----------------
struct Pattern {
    virtual ~Pattern() = default;
};

struct WildcardPattern : Pattern {
    WildcardPattern() = default;
};

struct LiteralPattern : Pattern {
    Value value;
    explicit LiteralPattern(Value v) : value(std::move(v)) {}
};

struct VariablePattern : Pattern {
    std::string name;
    explicit VariablePattern(std::string n) : name(std::move(n)) {}
};

struct RangePattern : Pattern {
    Value start;
    Value end;
    bool inclusive;
    RangePattern(Value s, Value e, bool incl) : start(std::move(s)), end(std::move(e)), inclusive(incl) {}
};

struct TuplePattern : Pattern {
    std::vector<std::unique_ptr<Pattern>> patterns;
    explicit TuplePattern(std::vector<std::unique_ptr<Pattern>> p) : patterns(std::move(p)) {}
};

struct StructPattern : Pattern {
    std::string structName;
    std::vector<std::pair<std::string, std::unique_ptr<Pattern>>> fields;
    StructPattern(std::string name, std::vector<std::pair<std::string, std::unique_ptr<Pattern>>> f)
        : structName(std::move(name)), fields(std::move(f)) {}
};

struct OrPattern : Pattern {
    std::vector<std::unique_ptr<Pattern>> patterns;
    explicit OrPattern(std::vector<std::unique_ptr<Pattern>> p) : patterns(std::move(p)) {}
};

struct GuardedPattern : Pattern {
    std::unique_ptr<Pattern> pattern;
    std::unique_ptr<struct Expression> guard;
    GuardedPattern(std::unique_ptr<Pattern> p, std::unique_ptr<struct Expression> g)
        : pattern(std::move(p)), guard(std::move(g)) {}
};

// ---------------- Expressions ----------------
struct Expression {
    SourceLocation location;

    virtual ~Expression() = default;
    virtual void accept(Visitor&) = 0;
};

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod, Pow,
    Equal, NotEqual,
    Less, LessEqual,
    Greater, GreaterEqual,
    And, Or,
    BitAnd, BitOr, BitXor, Shl, Shr,
    In,     // membership: x in list, key in map, substr in str
    NotIn   // not in operator
};

enum class UnaryOp {
    Not, Neg, BitNot
};

struct NumberExpr : Expression {
    double value;
    bool isInteger;
    NumberExpr(double val, bool isInt = false) : value(val), isInteger(isInt) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct LiteralExpr : Expression {
    Value value;
    explicit LiteralExpr(const Value& val) : value(val) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct InputExpr : Expression {
    std::string prompt;
    std::string type;
    InputExpr(const std::string& p, const std::string& t = "string") : prompt(p), type(t) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct VariableExpr : Expression {
    std::string name;
    VariableExpr(const std::string& n) : name(n) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct BinaryExpr : Expression {
    BinaryOp op;
    std::unique_ptr<Expression> left, right;
    BinaryExpr(std::unique_ptr<Expression> l, BinaryOp o, std::unique_ptr<Expression> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct BoolExpr : Expression {
    bool value;
    BoolExpr(bool val) : value(val) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// Chained comparison: a < b < c → evaluates each pair and ANDs results
struct ChainedComparisonExpr : Expression {
    std::vector<std::unique_ptr<Expression>> operands;  // a, b, c
    std::vector<BinaryOp> operators;                     // <, <
    ChainedComparisonExpr() = default;
    void accept(Visitor& v) override { v.visit(*this); }
};

struct UnaryExpr : Expression {
    UnaryOp op;
    std::unique_ptr<Expression> right;
    UnaryExpr(UnaryOp o, std::unique_ptr<Expression> r) : op(o), right(std::move(r)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct CallExpr : Expression {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    std::vector<std::string> argumentNames;  // Named arguments: empty string = positional

    CallExpr(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args)
        : callee(std::move(callee)), arguments(std::move(args)) {}

    void accept(Visitor& v) override { v.visit(*this); }
};

struct ListExpr : Expression {
    std::vector<std::unique_ptr<Expression>> elements;
    ListExpr(std::vector<std::unique_ptr<Expression>> elems)
        : elements(std::move(elems)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

// Map literal expression: {key: value, key2: value2}
struct MapExpr : Expression {
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> pairs;
    MapExpr(std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> p)
        : pairs(std::move(p)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

struct IndexExpr : Expression {
    std::unique_ptr<Expression> listExpr, indexExpr;
    IndexExpr(std::unique_ptr<Expression> list, std::unique_ptr<Expression> index)
        : listExpr(std::move(list)), indexExpr(std::move(index)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct CastExpr : Expression {
    std::unique_ptr<Expression> expression;
    std::string targetType;
    CastExpr(std::unique_ptr<Expression> expr, std::string type)
        : expression(std::move(expr)), targetType(std::move(type)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

struct InterpolatedStringExpr : Expression {
    std::string raw;
    explicit InterpolatedStringExpr(const std::string& r) : raw(r) {}
    void accept(Visitor&) override {}
};

// Lambda expression: |a, b| a + b  OR  |a, b| { stmts; return expr; }
struct LambdaExpr : Expression {
    std::vector<std::string> parameters;
    std::vector<std::unique_ptr<Expression>> parameterDefaults;  // default values (nullptr = required)
    std::unique_ptr<Expression> body;           // Simple expression body (may be null for block)
    std::unique_ptr<Statement> blockBody;       // Block body with statements (may be null for expr)

    LambdaExpr(std::vector<std::string> params, std::unique_ptr<Expression> body)
        : parameters(std::move(params)), body(std::move(body)), blockBody(nullptr) {}

    LambdaExpr(std::vector<std::string> params, std::unique_ptr<Statement> block)
        : parameters(std::move(params)), body(nullptr), blockBody(std::move(block)) {}

    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Range expression: 0..10 or 0..=10
struct RangeExpr : Expression {
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    bool inclusive;

    RangeExpr(std::unique_ptr<Expression> start, std::unique_ptr<Expression> end, bool inclusive)
        : start(std::move(start)), end(std::move(end)), inclusive(inclusive) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Pipe expression: expr |> func
struct PipeExpr : Expression {
    std::unique_ptr<Expression> value;
    std::unique_ptr<Expression> function;

    PipeExpr(std::unique_ptr<Expression> val, std::unique_ptr<Expression> func)
        : value(std::move(val)), function(std::move(func)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Ternary expression: condition ? then : else
struct TernaryExpr : Expression {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> thenExpr;
    std::unique_ptr<Expression> elseExpr;

    TernaryExpr(std::unique_ptr<Expression> cond, std::unique_ptr<Expression> then_e, std::unique_ptr<Expression> else_e)
        : condition(std::move(cond)), thenExpr(std::move(then_e)), elseExpr(std::move(else_e)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Null coalescing expression: expr ?? default
struct NullCoalesceExpr : Expression {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    NullCoalesceExpr(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left(std::move(l)), right(std::move(r)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Spread expression: ...expr (used in list literals and function calls)
struct SpreadExpr : Expression {
    std::unique_ptr<Expression> expression;

    SpreadExpr(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Slice expression: list[start:end], str[0:5], list[:3], list[2:]
struct SliceExpr : Expression {
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> start;  // may be null (e.g., [:3])
    std::unique_ptr<Expression> end;    // may be null (e.g., [2:])

    SliceExpr(std::unique_ptr<Expression> obj, std::unique_ptr<Expression> s, std::unique_ptr<Expression> e)
        : object(std::move(obj)), start(std::move(s)), end(std::move(e)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// ---------------- Statements ----------------
struct Statement {
    SourceLocation location;

    virtual ~Statement() = default;
    virtual void accept(StatementVisitor&) const = 0;
};

#define DEFINE_ACCEPT() \
    void accept(StatementVisitor& visitor) const override { visitor.visit(*this); }

struct PrintStmt : Statement {
    std::unique_ptr<Expression> expression;
    explicit PrintStmt(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    DEFINE_ACCEPT();
};

struct AssignStmt : Statement {
    std::string name;
    std::unique_ptr<Expression> expression;
    AssignStmt(std::string n, std::unique_ptr<Expression> expr) : name(std::move(n)), expression(std::move(expr)) {}
    DEFINE_ACCEPT();
};

// Compound assignment: x += 5, x -= 3, x *= 2, x /= 4, x %= 3
struct CompoundAssignStmt : Statement {
    std::string name;
    BinaryOp op;  // The underlying operation (Add, Sub, Mul, Div, Mod)
    std::unique_ptr<Expression> expression;
    CompoundAssignStmt(std::string n, BinaryOp o, std::unique_ptr<Expression> expr)
        : name(std::move(n)), op(o), expression(std::move(expr)) {}
    DEFINE_ACCEPT();
};

struct LetStmt : Statement {
    std::string name;
    std::unique_ptr<Expression> expression;
    std::optional<std::string> typeAnnotation;
    bool isMutable;  // false for 'let', true for 'var'

    LetStmt(std::string n, std::unique_ptr<Expression> expr, std::optional<std::string> type = std::nullopt, bool mut = false)
        : name(std::move(n)), expression(std::move(expr)), typeAnnotation(std::move(type)), isMutable(mut) {}
    DEFINE_ACCEPT();
};

struct ConstStmt : Statement {
    std::string name;
    std::unique_ptr<Expression> expression;
    std::string typeAnnotation;

    ConstStmt(std::string n, std::unique_ptr<Expression> expr, std::string type)
        : name(std::move(n)), expression(std::move(expr)), typeAnnotation(std::move(type)) {}
    DEFINE_ACCEPT();
};

struct IfStmt : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;
    IfStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> thenB, std::unique_ptr<Statement> elseB = nullptr)
        : condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
    DEFINE_ACCEPT();
};

struct BlockStmt : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
    BlockStmt(std::vector<std::unique_ptr<Statement>> stmts) : statements(std::move(stmts)) {}
    DEFINE_ACCEPT();
};

struct FunctionStmt : Statement {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::string> parameterTypes;
    std::vector<std::unique_ptr<Expression>> parameterDefaults;  // default values (nullptr = required)
    std::string returnType;
    std::unique_ptr<Statement> body;

    FunctionStmt(std::string n, std::vector<std::string> params, std::unique_ptr<Statement> b,
                 std::vector<std::string> paramTypes = {}, std::string retType = "",
                 std::vector<std::unique_ptr<Expression>> defaults = {})
        : name(std::move(n)), parameters(std::move(params)),
          parameterTypes(std::move(paramTypes)), parameterDefaults(std::move(defaults)),
          returnType(std::move(retType)),
          body(std::move(b)) {}
    DEFINE_ACCEPT();
};

struct ReturnStmt : Statement {
    std::unique_ptr<Expression> value;  // Can be nullptr for bare "return;"
    explicit ReturnStmt(std::unique_ptr<Expression> val = nullptr) : value(std::move(val)) {}
    DEFINE_ACCEPT();
};

struct ExternFunctionDecl {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::string> parameterTypes;
    std::string returnType;
    bool isVarArg;

    ExternFunctionDecl(std::string n, std::vector<std::string> params,
                       std::vector<std::string> paramTypes, std::string retType,
                       bool vararg = false)
        : name(std::move(n)), parameters(std::move(params)),
          parameterTypes(std::move(paramTypes)), returnType(std::move(retType)),
          isVarArg(vararg) {}
};

struct ExternBlock : Statement {
    std::string abi;
    std::vector<std::unique_ptr<ExternFunctionDecl>> functions;

    ExternBlock(std::string a, std::vector<std::unique_ptr<ExternFunctionDecl>> funcs)
        : abi(std::move(a)), functions(std::move(funcs)) {}
    DEFINE_ACCEPT();
};

struct ExpressionStmt : Statement {
    std::unique_ptr<Expression> expression;
    ExpressionStmt(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    DEFINE_ACCEPT();
};

struct IndexAssignStmt : Statement {
    std::unique_ptr<Expression> listExpr;
    std::unique_ptr<Expression> indexExpr;
    std::unique_ptr<Expression> valueExpr;
    IndexAssignStmt(std::unique_ptr<Expression> list, std::unique_ptr<Expression> index, std::unique_ptr<Expression> value)
        : listExpr(std::move(list)), indexExpr(std::move(index)), valueExpr(std::move(value)) {}
    DEFINE_ACCEPT();
};

struct ForStmt : Statement {
    std::string var;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;
    ForStmt(std::string v, std::unique_ptr<Expression> i, std::unique_ptr<Statement> b)
        : var(std::move(v)), iterable(std::move(i)), body(std::move(b)) {}
    DEFINE_ACCEPT();
};

struct WhileStmt : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    WhileStmt(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    DEFINE_ACCEPT();
};

struct LoopStmt : Statement {
    std::unique_ptr<Statement> body;
    LoopStmt(std::unique_ptr<Statement> b) : body(std::move(b)) {}
    DEFINE_ACCEPT();
};

struct BreakStmt : Statement {
    DEFINE_ACCEPT();
};

struct ContinueStmt : Statement {
    DEFINE_ACCEPT();
};

struct EnumStmt : Statement {
    std::string name;
    std::vector<std::string> values;
    // Associated data: maps variant name to its parameter names (empty vector = no params)
    std::vector<std::vector<std::string>> variantParams;
    EnumStmt(const std::string& n, const std::vector<std::string>& v) : name(n), values(v) {}
    DEFINE_ACCEPT();
};

struct MatchArm {
    std::unique_ptr<Pattern> pattern;
    std::unique_ptr<Statement> body;

    MatchArm(std::unique_ptr<Pattern> p, std::unique_ptr<Statement> b)
        : pattern(std::move(p)), body(std::move(b)) {}
};

struct MatchStmt : Statement {
    std::unique_ptr<Expression> expr;
    std::vector<MatchArm> arms;

    MatchStmt(std::unique_ptr<Expression> e, std::vector<MatchArm> a)
        : expr(std::move(e)), arms(std::move(a)) {}
    DEFINE_ACCEPT();
};

struct SwitchStmt : Statement {
    std::unique_ptr<Expression> expr;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>> cases;
    std::unique_ptr<Statement> defaultCase;
    SwitchStmt(std::unique_ptr<Expression> e, std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>> c, std::unique_ptr<Statement> d)
        : expr(std::move(e)), cases(std::move(c)), defaultCase(std::move(d)) {}
    DEFINE_ACCEPT();
};

class StructStmt : public Statement {
public:
    std::string name;
    std::vector<std::string> fields;

    StructStmt(std::string name, std::vector<std::string> fields)
        : name(std::move(name)), fields(std::move(fields)) {}

    void accept(StatementVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

enum class AccessModifier { Public, Private };

struct ClassStmt : Statement {
    std::string name;
    std::string parentName;  // inheritance: class Dog extends Animal
    std::vector<std::string> fields;
    std::vector<AccessModifier> fieldAccess;  // access modifier per field
    std::vector<std::unique_ptr<FunctionStmt>> methods;
    std::vector<AccessModifier> methodAccess;  // access modifier per method
    // Static members
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> staticFields;
    std::vector<std::unique_ptr<FunctionStmt>> staticMethods;
    // Getters and setters
    std::vector<std::unique_ptr<FunctionStmt>> getters;
    std::vector<std::unique_ptr<FunctionStmt>> setters;
    // Lazy properties: lazy let parsed = expr;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> lazyFields;
    // Multiple trait implementation: class X impl Trait1, Trait2
    std::vector<std::string> implTraits;
    // Flags
    bool isDataClass = false;
    bool isSealed = false;

    ClassStmt(const std::string& name,
              std::vector<std::string> fields,
              std::vector<std::unique_ptr<FunctionStmt>> methods,
              const std::string& parent = "")
        : name(name), parentName(parent), fields(std::move(fields)), methods(std::move(methods)) {}

    void accept(StatementVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

struct SetStmt : Statement {
    std::unique_ptr<Expression> object;
    std::unique_ptr<Expression> index;
    std::unique_ptr<Expression> value;

    SetStmt(std::unique_ptr<Expression> object,
            std::unique_ptr<Expression> index,
            std::unique_ptr<Expression> value)
        : object(std::move(object)), index(std::move(index)), value(std::move(value)) {}

    void accept(StatementVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

struct GetExpr : Expression {
    std::unique_ptr<Expression> object;
    std::string name;

    GetExpr(std::unique_ptr<Expression> object, const std::string& name)
        : object(std::move(object)), name(name) {}

    void accept(Visitor& visitor) override {
        visitor.visit(*this);
    }
};

struct ThisExpr : Expression {
    ThisExpr() {}

    void accept(Visitor& visitor) override {
        visitor.visit(*this);
    }
};

struct ImportStmt : Statement {
    std::string path;
    ImportStmt(const std::string& path) : path(path) {}
    DEFINE_ACCEPT();
};

struct ExportStmt : Statement {
    std::unique_ptr<Statement> statement;
    ExportStmt(std::unique_ptr<Statement> stmt) : statement(std::move(stmt)) {}
    DEFINE_ACCEPT();
};

struct DeferStmt : Statement {
    std::unique_ptr<Statement> statement;
    explicit DeferStmt(std::unique_ptr<Statement> stmt) : statement(std::move(stmt)) {}
    DEFINE_ACCEPT();
};

struct AssertStmt : Statement {
    std::unique_ptr<Expression> condition;
    std::string message;
    bool isDebugOnly;

    AssertStmt(std::unique_ptr<Expression> cond, std::string msg = "", bool debugOnly = false)
        : condition(std::move(cond)), message(std::move(msg)), isDebugOnly(debugOnly) {}
    DEFINE_ACCEPT();
};

// try { ... } catch (TypeError | ValueError as e) { ... } finally { ... }
struct TryCatchStmt : Statement {
    std::unique_ptr<Statement> tryBlock;
    std::string errorVar;  // Variable name for caught error
    std::vector<std::string> errorTypes;  // optional: list of error types to catch (multi-catch)
    std::unique_ptr<Statement> catchBlock;
    std::unique_ptr<Statement> finallyBlock;  // optional finally block

    TryCatchStmt(std::unique_ptr<Statement> tryB, std::string errVar, std::unique_ptr<Statement> catchB,
                 std::unique_ptr<Statement> finallyB = nullptr)
        : tryBlock(std::move(tryB)), errorVar(std::move(errVar)), catchBlock(std::move(catchB)),
          finallyBlock(std::move(finallyB)) {}
    DEFINE_ACCEPT();
};

// throw expression;
struct ThrowStmt : Statement {
    std::unique_ptr<Expression> expression;
    explicit ThrowStmt(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    DEFINE_ACCEPT();
};

// do { ... } while (condition);
struct DoWhileStmt : Statement {
    std::unique_ptr<Statement> body;
    std::unique_ptr<Expression> condition;

    DoWhileStmt(std::unique_ptr<Statement> b, std::unique_ptr<Expression> cond)
        : body(std::move(b)), condition(std::move(cond)) {}
    DEFINE_ACCEPT();
};

// let [a, b, c] = list;
struct DestructureLetStmt : Statement {
    std::vector<std::string> names;
    std::unique_ptr<Expression> expression;
    bool isMutable;

    DestructureLetStmt(std::vector<std::string> n, std::unique_ptr<Expression> expr, bool mut = false)
        : names(std::move(n)), expression(std::move(expr)), isMutable(mut) {}
    DEFINE_ACCEPT();
};

// go expression; (goroutine-style concurrency)
struct GoStmt : Statement {
    std::unique_ptr<Expression> expression;
    explicit GoStmt(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    DEFINE_ACCEPT();
};

// i++ or i-- (increment/decrement statement)
struct IncrementStmt : Statement {
    std::string name;
    bool isIncrement;  // true = ++, false = --

    IncrementStmt(std::string n, bool inc) : name(std::move(n)), isIncrement(inc) {}
    DEFINE_ACCEPT();
};

// for [a, b] in expr { ... } (destructuring for loop)
struct ForDestructureStmt : Statement {
    std::vector<std::string> vars;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;

    ForDestructureStmt(std::vector<std::string> v, std::unique_ptr<Expression> i, std::unique_ptr<Statement> b)
        : vars(std::move(v)), iterable(std::move(i)), body(std::move(b)) {}
    DEFINE_ACCEPT();
};

// super.method(args) expression
struct SuperExpr : Expression {
    std::string methodName;
    SuperExpr(const std::string& method) : methodName(method) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// x is int, x is ClassName
struct IsExpr : Expression {
    std::unique_ptr<Expression> object;
    std::string typeName;
    IsExpr(std::unique_ptr<Expression> obj, const std::string& type)
        : object(std::move(obj)), typeName(type) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// obj?.field (null-propagating access)
struct OptionalGetExpr : Expression {
    std::unique_ptr<Expression> object;
    std::string name;
    OptionalGetExpr(std::unique_ptr<Expression> obj, const std::string& n)
        : object(std::move(obj)), name(n) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// trait Printable { func toString(); }
struct TraitStmt : Statement {
    std::string name;
    std::vector<std::string> requiredMethods;
    std::vector<std::unique_ptr<FunctionStmt>> defaultMethods;  // methods with bodies

    TraitStmt(const std::string& n, std::vector<std::string> required,
              std::vector<std::unique_ptr<FunctionStmt>> defaults = {})
        : name(n), requiredMethods(std::move(required)), defaultMethods(std::move(defaults)) {}
    DEFINE_ACCEPT();
};

// impl Printable for Point { }
struct ImplStmt : Statement {
    std::string traitName;
    std::string className;
    std::vector<std::unique_ptr<FunctionStmt>> methods;  // optional method implementations

    ImplStmt(const std::string& trait, const std::string& cls,
             std::vector<std::unique_ptr<FunctionStmt>> meths = {})
        : traitName(trait), className(cls), methods(std::move(meths)) {}
    DEFINE_ACCEPT();
};

// repeat 5 { ... } or repeat 3 as i { ... }
struct RepeatStmt : Statement {
    std::unique_ptr<Expression> count;
    std::string varName;  // optional loop variable (empty = no variable)
    std::unique_ptr<Statement> body;

    RepeatStmt(std::unique_ptr<Expression> c, const std::string& var, std::unique_ptr<Statement> b)
        : count(std::move(c)), varName(var), body(std::move(b)) {}
    DEFINE_ACCEPT();
};

// extend String { func isPalindrome(s) { ... } }
struct ExtendStmt : Statement {
    std::string typeName;
    std::vector<std::unique_ptr<FunctionStmt>> methods;

    ExtendStmt(const std::string& type, std::vector<std::unique_ptr<FunctionStmt>> meths)
        : typeName(type), methods(std::move(meths)) {}
    DEFINE_ACCEPT();
};

// let {name, age} = person;
struct ObjectDestructureLetStmt : Statement {
    std::vector<std::string> fieldNames;
    std::unique_ptr<Expression> expression;
    bool isMutable;

    ObjectDestructureLetStmt(std::vector<std::string> names, std::unique_ptr<Expression> expr, bool mut = false)
        : fieldNames(std::move(names)), expression(std::move(expr)), isMutable(mut) {}
    DEFINE_ACCEPT();
};

// BinaryOp extension
// NotIn is handled as separate BinaryOp

// List comprehension: [x*2 for x in 0..10 if x % 2 == 0]
struct ListComprehensionExpr : Expression {
    std::unique_ptr<Expression> body;
    std::string varName;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Expression> condition;  // optional filter

    ListComprehensionExpr(std::unique_ptr<Expression> b, const std::string& var,
                          std::unique_ptr<Expression> iter, std::unique_ptr<Expression> cond = nullptr)
        : body(std::move(b)), varName(var), iterable(std::move(iter)), condition(std::move(cond)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Map comprehension: {k: v for x in iterable}
struct MapComprehensionExpr : Expression {
    std::unique_ptr<Expression> keyExpr;
    std::unique_ptr<Expression> valueExpr;
    std::string varName;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Expression> condition;

    MapComprehensionExpr(std::unique_ptr<Expression> k, std::unique_ptr<Expression> v,
                         const std::string& var, std::unique_ptr<Expression> iter,
                         std::unique_ptr<Expression> cond = nullptr)
        : keyExpr(std::move(k)), valueExpr(std::move(v)), varName(var),
          iterable(std::move(iter)), condition(std::move(cond)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Walrus operator: let x := expr (assigns and returns value)
struct WalrusExpr : Expression {
    std::string name;
    std::unique_ptr<Expression> expression;
    WalrusExpr(const std::string& n, std::unique_ptr<Expression> expr)
        : name(n), expression(std::move(expr)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

// Function composition: f >>> g
struct ComposeExpr : Expression {
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    ComposeExpr(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left(std::move(l)), right(std::move(r)) {}
    void accept(Visitor& v) override { /* handled directly in interpreter */ }
};

#undef DEFINE_ACCEPT

#endif // AST_H
