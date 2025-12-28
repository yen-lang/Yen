#pragma once
#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream> // For std::ostream

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


struct ObjectInstance {
    std::unordered_map<std::string, struct Value> fields;
};

struct ClassInstance {
    std::unordered_map<std::string, struct Value> fields;
    std::string className; // ADDED THIS
};

struct FunctionStmt;
struct Expression;

struct NativeFunction {
    using FunctionType = Value(*)(std::vector<struct Value>);
    FunctionType function;
    int arity;

    bool operator==(const NativeFunction& other) const {
        return function == other.function;
    }
    // No operator< for NativeFunction as it doesn't have a meaningful order
};

// Lambda/closure representation
struct LambdaValue {
    std::vector<std::string> parameters;
    const Expression* body;  // Pointer to lambda body expression
    std::shared_ptr<std::unordered_map<std::string, Value>> captured_env;  // Captured variables

    LambdaValue(std::vector<std::string> params, const Expression* body_expr,
                std::shared_ptr<std::unordered_map<std::string, Value>> env = nullptr)
        : parameters(std::move(params)), body(body_expr), captured_env(env) {}

    bool operator==(const LambdaValue& other) const {
        return body == other.body;  // Compare by body pointer
    }
};

// Global operator<< for NativeFunction
inline std::ostream& operator<<(std::ostream& os, const NativeFunction& func) {
    os << "{native fn}";
    return os;
}

// Global operator<< for LambdaValue
inline std::ostream& operator<<(std::ostream& os, const LambdaValue& lambda) {
    os << "{lambda}";
    return os;
}


using ValueVariant = std::variant<
    std::monostate,
    int,
    double,
    float,
    bool,
    std::string,
    std::vector<struct Value>,
    std::unordered_map<std::string, struct Value>,
    std::shared_ptr<ClassInstance>,
    std::shared_ptr<ObjectInstance>,
    const FunctionStmt*,
    NativeFunction,
    LambdaValue
>;

struct Value {
    ValueVariant data;

    // Default constructor
    Value() : data(std::monostate{}) {}

    // Constructor for ValueVariant types
    template<typename T,
             typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Value>>>
    Value(T&& val) : data(std::forward<T>(val)) {}

    // Copy constructor
    Value(const Value& other) : data(other.data) {}
    
    // Move constructor
    Value(Value&& other) : data(std::move(other.data)) {}

    // Assignment operator
    Value& operator=(const Value& other) {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }

    // Move assignment operator
    Value& operator=(Value&& other) {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }

    // Accessors for std::variant functionality
    template<typename T>
    bool holds_alternative() const { return std::holds_alternative<T>(data); }

    template<typename T>
    const T& get() const { return std::get<T>(data); }

    size_t index() const { return data.index(); }

    bool operator==(const Value& other) const {
        if (data.index() != other.data.index()) return false; // Types must match
        return std::visit(overloaded {
            [](std::monostate, std::monostate) { return true; },
            [](int a, int b) { return a == b; },
            [](double a, double b) { return a == b; },
            [](float a, float b) { return a == b; },
            [](bool a, bool b) { return a == b; },
            [](const std::string& a, const std::string& b) { return a == b; },
            [](const std::vector<Value>& a, const std::vector<Value>& b) {
                if (a.size() != b.size()) return false;
                for (size_t i = 0; i < a.size(); ++i) {
                    if (!(a[i] == b[i])) return false;
                }
                return true;
            },
            [](const std::unordered_map<std::string, Value>& a, const std::unordered_map<std::string, Value>& b) {
                if (a.size() != b.size()) return false;
                for (const auto& pair : a) {
                    auto it = b.find(pair.first);
                    if (it == b.end() || !(pair.second == it->second)) return false;
                }
                return true;
            },
            [](const std::shared_ptr<ClassInstance>& a, const std::shared_ptr<ClassInstance>& b) { return a == b; },
            [](const std::shared_ptr<ObjectInstance>& a, const std::shared_ptr<ObjectInstance>& b) { return a == b; },
            [](const FunctionStmt* a, const FunctionStmt* b) { return a == b; },
            [](const NativeFunction& a, const NativeFunction& b) { return a.function == b.function; },
            [](const LambdaValue& a, const LambdaValue& b) { return a == b; },
            [](auto&&, auto&&) { return false; } // Fallback for unmatched types (should not happen if all are listed)
        }, data, other.data);
    }
    
    // Define operator!= in terms of operator==
    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    bool operator<(const Value& other) const {
        if (data.index() != other.data.index()) return this->index() < other.index(); // Order by type
        return std::visit(overloaded {
            [](std::monostate, std::monostate) { return false; }, // Arbitrary for monostate
            [](int a, int b) { return a < b; },
            [](double a, double b) { return a < b; },
            [](float a, float b) { return a < b; },
            [](bool a, bool b) { return a < b; },
            [](const std::string& a, const std::string& b) { return a < b; },
            [](const std::vector<Value>&, const std::vector<Value>&) { return false; }, // Arbitrary for vector
            [](const std::unordered_map<std::string, Value>&, const std::unordered_map<std::string, Value>&) { return false; }, // Arbitrary for map
            [](const std::shared_ptr<ClassInstance>& a, const std::shared_ptr<ClassInstance>& b) { return a < b; }, // Pointer comparison
            [](const std::shared_ptr<ObjectInstance>& a, const std::shared_ptr<ObjectInstance>& b) { return a < b; }, // Pointer comparison
            [](const FunctionStmt* a, const FunctionStmt* b) { return a < b; }, // Pointer comparison
            [](const NativeFunction&, const NativeFunction&) { return false; }, // No meaningful order
            [](const LambdaValue& a, const LambdaValue& b) { return a.body < b.body; }, // Compare by body pointer
            [](auto&&, auto&&) { return false; } // Fallback for unmatched types
        }, data, other.data);
    }
};