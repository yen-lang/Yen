#ifndef AST_H
#define AST_H

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <variant>
#include "value.h"  
// Forward declare Expressions and Statements
struct NumberExpr; struct LiteralExpr; struct InputExpr; struct VariableExpr;
struct BinaryExpr; struct BoolExpr; struct UnaryExpr; struct CallExpr;
struct ListExpr; struct IndexExpr;

struct PrintStmt; struct LetStmt; struct AssignStmt; struct StructStmt;
struct IfStmt; struct BlockStmt; struct FunctionStmt; struct ReturnStmt;
struct ExpressionStmt; struct IndexAssignStmt; struct ForStmt; struct WhileStmt;
struct BreakStmt; struct ContinueStmt; struct EnumStmt; struct MatchStmt;
struct SwitchStmt; struct ClassStmt; struct SetStmt; struct GetExpr;
struct ThisExpr;


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
};

class StatementVisitor {
public:
    virtual void visit(const PrintStmt&) = 0;
    virtual void visit(const LetStmt&) = 0;
    virtual void visit(const AssignStmt&) = 0;
    virtual void visit(const SetStmt&) = 0;
    virtual void visit(const StructStmt&) = 0;
    virtual void visit(const IfStmt&) = 0;
    virtual void visit(const BlockStmt&) = 0;
    virtual void visit(const FunctionStmt&) = 0;
    virtual void visit(const ReturnStmt&) = 0;
    virtual void visit(const ExpressionStmt&) = 0;
    virtual void visit(const IndexAssignStmt&) = 0;
    virtual void visit(const ForStmt&) = 0;
    virtual void visit(const WhileStmt&) = 0;
    virtual void visit(const BreakStmt&) = 0;
    virtual void visit(const ContinueStmt&) = 0;
    virtual void visit(const EnumStmt&) = 0;
    virtual void visit(const MatchStmt&) = 0;
    virtual void visit(const SwitchStmt&) = 0;
    virtual void visit(const ClassStmt&) = 0;
    virtual ~StatementVisitor() = default;
};

// ---------------- Expressions ----------------
struct Expression {
    virtual ~Expression() = default;
    virtual void accept(Visitor&) = 0;
};

enum class BinaryOp {
    Add, Sub, Mul, Div,
    Equal, NotEqual,
    Less, LessEqual,
    Greater, GreaterEqual,
    And, Or
};

enum class UnaryOp {
    Not
};

struct NumberExpr : Expression {
    int value;
    NumberExpr(int val) : value(val) {}
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

struct UnaryExpr : Expression {
    UnaryOp op;
    std::unique_ptr<Expression> right;
    UnaryExpr(UnaryOp o, std::unique_ptr<Expression> r) : op(o), right(std::move(r)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct CallExpr : Expression {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;

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

struct IndexExpr : Expression {
    std::unique_ptr<Expression> listExpr, indexExpr;
    IndexExpr(std::unique_ptr<Expression> list, std::unique_ptr<Expression> index)
        : listExpr(std::move(list)), indexExpr(std::move(index)) {}
    void accept(Visitor& v) override { v.visit(*this); }
};

struct InterpolatedStringExpr : Expression {
    std::string raw;
    explicit InterpolatedStringExpr(const std::string& r) : raw(r) {}
    void accept(Visitor&) override {}
};

// ---------------- Statements ----------------
struct Statement {
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

struct LetStmt : Statement {
    std::string name;
    std::unique_ptr<Expression> expression;
    LetStmt(std::string n, std::unique_ptr<Expression> expr) : name(std::move(n)), expression(std::move(expr)) {}
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
    std::unique_ptr<Statement> body;
    FunctionStmt(std::string n, std::vector<std::string> params, std::unique_ptr<Statement> b)
        : name(std::move(n)), parameters(std::move(params)), body(std::move(b)) {}
    DEFINE_ACCEPT();
};

struct ReturnStmt : Statement {
    std::unique_ptr<Expression> value;
    explicit ReturnStmt(std::unique_ptr<Expression> val) : value(std::move(val)) {}
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

struct BreakStmt : Statement {
    DEFINE_ACCEPT();
};

struct ContinueStmt : Statement {
    DEFINE_ACCEPT();
};

struct EnumStmt : Statement {
    std::string name;
    std::vector<std::string> values;
    EnumStmt(const std::string& n, const std::vector<std::string>& v) : name(n), values(v) {}
    DEFINE_ACCEPT();
};

struct MatchStmt : Statement {
    std::unique_ptr<Expression> expr;
    std::vector<std::pair<std::string, std::unique_ptr<Statement>>> arms;
    MatchStmt(std::unique_ptr<Expression> e, std::vector<std::pair<std::string, std::unique_ptr<Statement>>> a)
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

struct ClassStmt : Statement {
    std::string name;
    std::vector<std::string> fields;
    std::vector<std::unique_ptr<FunctionStmt>> methods;

    ClassStmt(const std::string& name,
              std::vector<std::string> fields,
              std::vector<std::unique_ptr<FunctionStmt>> methods)
        : name(name), fields(std::move(fields)), methods(std::move(methods)) {}

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


#undef DEFINE_ACCEPT

#endif // AST_H
