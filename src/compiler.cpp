#include "yen/value.h"
#include "yen/compiler.h"
#include "yen/lexer.h"
#include "yen/parser.h"
#include "yen/native_libs.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

// Constructor - register all native libraries
Interpreter::Interpreter() {
    YenNative::registerAllLibraries(variables);
}

Value Interpreter::evalExpr(const Expression* expr) {
    if (auto num = dynamic_cast<const NumberExpr*>(expr)) {
        return num->value;
    }

    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        return lit->value;
    }

    if (auto castExpr = dynamic_cast<const CastExpr*>(expr)) {
        Value val = evalExpr(castExpr->expression.get());
        const std::string& targetType = castExpr->targetType;

        // Perform type conversion
        if (targetType == "int" || targetType == "int32" || targetType == "int64") {
            if (val.holds_alternative<double>()) {
                return static_cast<int>(val.get<double>());
            } else if (val.holds_alternative<int>()) {
                return val.get<int>();
            } else if (val.holds_alternative<bool>()) {
                return val.get<bool>() ? 1 : 0;
            } else if (val.holds_alternative<std::string>()) {
                return std::stoi(val.get<std::string>());
            }
        } else if (targetType == "float" || targetType == "float32" || targetType == "float64") {
            if (val.holds_alternative<int>()) {
                return static_cast<double>(val.get<int>());
            } else if (val.holds_alternative<double>()) {
                return val.get<double>();
            } else if (val.holds_alternative<bool>()) {
                return val.get<bool>() ? 1.0 : 0.0;
            } else if (val.holds_alternative<std::string>()) {
                return std::stod(val.get<std::string>());
            }
        } else if (targetType == "bool") {
            if (val.holds_alternative<int>()) {
                return val.get<int>() != 0;
            } else if (val.holds_alternative<double>()) {
                return val.get<double>() != 0.0;
            } else if (val.holds_alternative<bool>()) {
                return val.get<bool>();
            } else if (val.holds_alternative<std::string>()) {
                std::string s = val.get<std::string>();
                return s == "true" || s == "1";
            }
        } else if (targetType == "string" || targetType == "str") {
            if (val.holds_alternative<int>()) {
                return std::to_string(val.get<int>());
            } else if (val.holds_alternative<double>()) {
                return std::to_string(val.get<double>());
            } else if (val.holds_alternative<bool>()) {
                return val.get<bool>() ? std::string("true") : std::string("false");
            } else if (val.holds_alternative<std::string>()) {
                return val.get<std::string>();
            }
        }

        throw std::runtime_error("Invalid cast from " + std::string(val.data.index() == 0 ? "null" : "unknown") + " to " + targetType);
    }

    if (auto lambdaExpr = dynamic_cast<const LambdaExpr*>(expr)) {
        // Capture current environment for closure
        auto captured = std::make_shared<std::unordered_map<std::string, Value>>(variables);
        return LambdaValue(lambdaExpr->parameters, lambdaExpr->body.get(), captured);
    }

    if (auto rangeExpr = dynamic_cast<const RangeExpr*>(expr)) {
        // Evaluate range and generate vector
        Value startVal = evalExpr(rangeExpr->start.get());
        Value endVal = evalExpr(rangeExpr->end.get());

        // Convert to int if needed
        int start, end;
        if (startVal.holds_alternative<int>()) {
            start = startVal.get<int>();
        } else if (startVal.holds_alternative<double>()) {
            start = static_cast<int>(startVal.get<double>());
        } else {
            throw std::runtime_error("Range start must be numeric");
        }

        if (endVal.holds_alternative<int>()) {
            end = endVal.get<int>();
        } else if (endVal.holds_alternative<double>()) {
            end = static_cast<int>(endVal.get<double>());
        } else {
            throw std::runtime_error("Range end must be numeric");
        }

        std::vector<Value> result;
        if (rangeExpr->inclusive) {
            // Inclusive range: start..=end
            for (int i = start; i <= end; ++i) {
                result.push_back(i);
            }
        } else {
            // Exclusive range: start..end
            for (int i = start; i < end; ++i) {
                result.push_back(i);
            }
        }

        return result;
    }

    if (auto thisExpr = dynamic_cast<const ThisExpr*>(expr)) {
        return environment->get("this");
    }
    if (auto getExpr = dynamic_cast<const GetExpr*>(expr)) {
        Value object = evalExpr(getExpr->object.get());
    
        if (object.holds_alternative<std::shared_ptr<ObjectInstance>>()) {
            auto instance = object.get<std::shared_ptr<ObjectInstance>>();
            auto it = instance->fields.find(getExpr->name);
            if (it != instance->fields.end()) {
                return it->second;
            } else {
                throw std::runtime_error("Field '" + getExpr->name + "' not found in ObjectInstance.");
            }
        } else if (object.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = object.get<std::shared_ptr<ClassInstance>>();
            auto it = instance->fields.find(getExpr->name);
            if (it != instance->fields.end()) {
                return it->second;
            } else {
                throw std::runtime_error("Field '" + getExpr->name + "' not found in ClassInstance.");
            }
        }
        throw std::runtime_error("Attempt to access a field on something that is not an object or class instance.");
    }        

    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        // First check environment (for function parameters and local scope)
        if (environment && environment->values.count(var->name)) {
            return environment->values[var->name];
        }

        // Then check global variables
        auto it = variables.find(var->name);
        if (it == variables.end()) {
            // Check if it's a function
            auto funcIt = functions.find(var->name);
            if (funcIt != functions.end()) {
                return funcIt->second;
            }

            // Check if it's a class
            auto classIt = classes.find(var->name);
            if (classIt != classes.end()) {
                // Create new instance
                auto instance = std::make_shared<ClassInstance>();
                instance->className = var->name;
                for (const auto& field : classIt->second->fields) {
                    instance->fields[field] = Value();
                }
                return instance;
            }
            throw std::runtime_error("Undefined variable: " + var->name);
        }
        return it->second;
    }    
    if (auto b = dynamic_cast<const BoolExpr*>(expr)) {
        return b->value;
    }
    
    if (auto input = dynamic_cast<const InputExpr*>(expr)) {
        std::string response;
        std::cout << input->prompt << " ";
        std::getline(std::cin, response);
    
        if (input->type == "int") {
            return std::stoi(response);
        } else if (input->type == "bool") {
            return (response == "true" || response == "1");
        } else {
            return response;
        }
    }
    
    if (auto list = dynamic_cast<const ListExpr*>(expr)) {
        std::vector<Value> result;
        for (const auto& e : list->elements) {
            result.push_back(evalExpr(e.get()));
        }
        return result;
    }
    if (auto indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        Value container = evalExpr(indexExpr->listExpr.get());
        Value index = evalExpr(indexExpr->indexExpr.get());
    
        if (container.holds_alternative<std::vector<Value>>()) {
            // It's a list
            const auto& list = container.get<std::vector<Value>>();

            // Accept both int and double (convert double to int)
            int idx;
            if (index.holds_alternative<int>()) {
                idx = index.get<int>();
            } else if (index.holds_alternative<double>()) {
                idx = static_cast<int>(index.get<double>());
            } else {
                throw std::runtime_error("List index must be an integer.");
            }

            if (idx < 0 || idx >= (int)list.size())
                throw std::runtime_error("List index out of bounds.");

            return list[idx];
        }
        else if (container.holds_alternative<std::unordered_map<std::string, Value>>()) {
            // It's a struct
            const auto& map = container.get<std::unordered_map<std::string, Value>>();
    
            if (!index.holds_alternative<std::string>())
                throw std::runtime_error("Struct field must be a string.");
    
            const std::string& key = index.get<std::string>();
    
            auto it = map.find(key);
            if (it == map.end())
                throw std::runtime_error("Field '" + key + "' not found in struct.");
    
            return it->second;
        }
        else if (container.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = container.get<std::shared_ptr<ClassInstance>>();
            if (!index.holds_alternative<std::string>())
                throw std::runtime_error("Class field index must be a string.");
            const std::string& key = index.get<std::string>();
            auto it = instance->fields.find(key);
            if (it == instance->fields.end())
                throw std::runtime_error("Field '" + key + "' not found in class instance.");
            return it->second;
        }
        else {
            throw std::runtime_error("Attempt to index something that is neither a list, struct, nor class instance.");
        }
    }

    if (auto interp = dynamic_cast<const InterpolatedStringExpr*>(expr)) {
        std::string result;
        std::string src = interp->raw;
    
        size_t pos = 0;
        while (true) {
            size_t start = src.find("${", pos);
            if (start == std::string::npos) {
                result += src.substr(pos);
                break;
            }
    
            result += src.substr(pos, start - pos);
            size_t end = src.find("}", start);
            if (end == std::string::npos) 
                throw std::runtime_error("Invalid string interpolation.");
    
            std::string varName = src.substr(start + 2, end - start - 2);
            Value val = variables[varName];
            if (val.holds_alternative<std::string>()) result += val.get<std::string>();
            else if (val.holds_alternative<int>()) result += std::to_string(val.get<int>());
            else if (val.holds_alternative<double>()) result += std::to_string(val.get<double>());
            else if (val.holds_alternative<bool>()) result += val.get<bool>() ? "true" : "false";
            else result += "null";
    
            pos = end + 1;
        }
    
        return result;
    }
    
    if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        Value callee = evalExpr(call->callee.get());

        std::vector<Value> arguments;
        for (const auto& argExpr : call->arguments) {
            arguments.push_back(evalExpr(argExpr.get()));
        }

        return this->call(callee, arguments);
    }
    if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        Value left = evalExpr(bin->left.get());
        Value right = evalExpr(bin->right.get());
    
        switch (bin->op) {
            case BinaryOp::Add:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l + r); },
                    [](float l, float r) { return Value(l + r); },
                    [](double l, double r) { return Value(l + r); },
                    [](int l, float r) { return Value(l + r); },
                    [](float l, int r) { return Value(l + r); },
                    [](int l, double r) { return Value(static_cast<double>(l) + r); },
                    [](double l, int r) { return Value(l + static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) + r); },
                    [](double l, float r) { return Value(l + static_cast<double>(r)); },
    
                    [](const std::string& l, const std::string& r) { return Value(l + r); },
    
                    [](const std::string& l, auto r) -> Value {
                        using T = std::decay_t<decltype(r)>;
                        if constexpr (std::is_same_v<T, std::monostate>) {
                            return Value(l + "null");
                        } else if constexpr (std::is_same_v<T, bool>) {
                            return Value(l + (r ? "true" : "false"));
                        } else if constexpr (std::is_arithmetic_v<T>) {
                            return Value(l + std::to_string(r));
                        } else {
                            throw std::runtime_error("Invalid type for string concatenation (right side).");
                        }
                    },
    
                    [](auto l, const std::string& r) -> Value {
                        using T = std::decay_t<decltype(l)>;
                        if constexpr (std::is_same_v<T, std::monostate>) {
                            return Value("null" + r);
                        } else if constexpr (std::is_same_v<T, bool>) {
                            return Value((l ? "true" : "false") + r);
                        } else if constexpr (std::is_arithmetic_v<T>) {
                            return Value(std::to_string(l) + r);
                        } else {
                            throw std::runtime_error("Invalid type for string concatenation (left side).");
                        }
                    },
    
                    [](auto, auto) -> Value {
                        throw std::runtime_error("Incompatible types for + operator.");
                    }
                }, left.data, right.data);
    
            case BinaryOp::Sub:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l - r); },
                    [](float l, float r) { return Value(l - r); },
                    [](double l, double r) { return Value(l - r); },
                    [](int l, float r) { return Value(l - r); },
                    [](float l, int r) { return Value(l - r); },
                    [](int l, double r) { return Value(static_cast<double>(l) - r); },
                    [](double l, int r) { return Value(l - static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) - r); },
                    [](double l, float r) { return Value(l - static_cast<double>(r)); },
                    [](auto, auto) -> Value { 
                        throw std::runtime_error("Incompatible types for - operator."); 
                    }
                }, left.data, right.data);
    
            case BinaryOp::Mul:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l * r); },
                    [](float l, float r) { return Value(l * r); },
                    [](double l, double r) { return Value(l * r); },
                    [](int l, float r) { return Value(l * r); },
                    [](float l, int r) { return Value(l * r); },
                    [](int l, double r) { return Value(static_cast<double>(l) * r); },
                    [](double l, int r) { return Value(l * static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) * r); },
                    [](double l, float r) { return Value(l * static_cast<double>(r)); },
                    [](auto, auto) -> Value { 
                        throw std::runtime_error("Incompatible types for * operator."); 
                    }
                }, left.data, right.data);
    
            case BinaryOp::Div:
                return std::visit(overloaded {
                    [](int l, int r) {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<double>(l) / r);
                    },
                    [](float l, float r) {
                        if (r == 0.0f) throw std::runtime_error("Division by zero.");
                        return Value(l / r);
                    },
                    [](double l, double r) {
                        if (r == 0.0) throw std::runtime_error("Division by zero.");
                        return Value(l / r);
                    },
                    [](int l, float r) {
                        if (r == 0.0f) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<double>(l) / r);
                    },
                    [](float l, int r) {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(l / static_cast<double>(r));
                    },
                    [](int l, double r) {
                        if (r == 0.0) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<double>(l) / r);
                    },
                    [](double l, int r) {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(l / static_cast<double>(r));
                    },
                    [](float l, double r) {
                        if (r == 0.0) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<double>(l) / r);
                    },
                    [](double l, float r) {
                        if (r == 0.0f) throw std::runtime_error("Division by zero.");
                        return Value(l / static_cast<double>(r));
                    },
                    [](auto, auto) -> Value { 
                        throw std::runtime_error("Incompatible types for / operator."); 
                    }
                }, left.data, right.data);    

            case BinaryOp::Equal: return left == right;
            case BinaryOp::NotEqual: return left != right;

            case BinaryOp::Less:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l < r); },
                    [](float l, float r) { return Value(l < r); },
                    [](double l, double r) { return Value(l < r); },
                    [](int l, float r) { return Value(l < r); },
                    [](float l, int r) { return Value(l < r); },
                    [](int l, double r) { return Value(static_cast<double>(l) < r); },
                    [](double l, int r) { return Value(l < static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) < r); },
                    [](double l, float r) { return Value(l < static_cast<double>(r)); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for <"); }
                }, left.data, right.data);


            case BinaryOp::Greater:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l > r); },
                    [](float l, float r) { return Value(l > r); },
                    [](double l, double r) { return Value(l > r); },
                    [](int l, float r) { return Value(l > r); },
                    [](float l, int r) { return Value(l > r); },
                    [](int l, double r) { return Value(static_cast<double>(l) > r); },
                    [](double l, int r) { return Value(l > static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) > r); },
                    [](double l, float r) { return Value(l > static_cast<double>(r)); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for >"); }
                }, left.data, right.data);

            case BinaryOp::GreaterEqual:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l >= r); },
                    [](float l, float r) { return Value(l >= r); },
                    [](double l, double r) { return Value(l >= r); },
                    [](int l, float r) { return Value(l >= r); },
                    [](float l, int r) { return Value(l >= r); },
                    [](int l, double r) { return Value(static_cast<double>(l) >= r); },
                    [](double l, int r) { return Value(l >= static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) >= r); },
                    [](double l, float r) { return Value(l >= static_cast<double>(r)); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for >="); }
                }, left.data, right.data);

            case BinaryOp::LessEqual:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l <= r); },
                    [](float l, float r) { return Value(l <= r); },
                    [](double l, double r) { return Value(l <= r); },
                    [](int l, float r) { return Value(l <= r); },
                    [](float l, int r) { return Value(l <= r); },
                    [](int l, double r) { return Value(static_cast<double>(l) <= r); },
                    [](double l, int r) { return Value(l <= static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) <= r); },
                    [](double l, float r) { return Value(l <= static_cast<double>(r)); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for  <="); }
                }, left.data, right.data);

            case BinaryOp::And:
                return std::visit(overloaded {
                    [](bool l, bool r) { return Value(l && r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Invalid types for operator '&&'"); }
                }, left.data, right.data);

            case BinaryOp::Or:
                return std::visit(overloaded {
                    [](bool l, bool r) { return Value(l || r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Invalid types for operator '||'"); }
                }, left.data, right.data);
        }
    }

    if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        Value right = evalExpr(unary->right.get());

        switch (unary->op) {
            case UnaryOp::Not:
                return std::visit(overloaded {
                    [](bool b) { return Value(!b); },
                    [](auto) -> Value { throw std::runtime_error("Operator '!' requires boolean."); }
                }, right.data);
        }
    }

    throw std::runtime_error("Invalid expression.");
}

void Interpreter::register_module(const std::string& name, std::shared_ptr<Environment> env) {
    modules[name] = env;

    // Expose module functions globally with prefix (e.g., math.sqrt)
    for (const auto& [funcName, funcValue] : env->values) {
        std::string globalName = name.empty() ? funcName : name + "." + funcName;
        variables[globalName] = funcValue;
    }
}

Value Interpreter::call(const Value& callee, const std::vector<Value>& args) {
    // Handle NativeFunction
    if (callee.holds_alternative<NativeFunction>()) {
        const auto& nativeFunc = callee.get<NativeFunction>();

        // Check arity (negative arity = variable args)
        if (nativeFunc.arity >= 0 && args.size() != static_cast<size_t>(nativeFunc.arity)) {
            throw std::runtime_error("Expected " + std::to_string(nativeFunc.arity) +
                                   " arguments but got " + std::to_string(args.size()) + ".");
        }

        // Call native function
        return nativeFunc.function(const_cast<std::vector<Value>&>(args));
    }

    // Handle Lambda/closure
    if (callee.holds_alternative<LambdaValue>()) {
        const auto& lambda = callee.get<LambdaValue>();

        // Check arity
        if (args.size() != lambda.parameters.size()) {
            throw std::runtime_error("Expected " + std::to_string(lambda.parameters.size()) +
                                   " arguments but got " + std::to_string(args.size()) + ".");
        }

        // Save current variables
        auto savedVars = variables;

        // Restore captured environment (for closure)
        if (lambda.captured_env) {
            variables = *lambda.captured_env;
        }

        // Bind parameters
        for (size_t i = 0; i < lambda.parameters.size(); ++i) {
            variables[lambda.parameters[i]] = args[i];
        }

        // Evaluate lambda body
        Value result = evalExpr(lambda.body);

        // Restore original variables
        variables = savedVars;

        return result;
    }

    // Handle user-defined FunctionStmt
    if (callee.holds_alternative<const FunctionStmt*>()) {
        const FunctionStmt* func = callee.get<const FunctionStmt*>();

        // Check arity
        if (args.size() != func->parameters.size()) {
            throw std::runtime_error("Expected " + std::to_string(func->parameters.size()) +
                                   " arguments but got " + std::to_string(args.size()) + ".");
        }

        // Save current environment
        auto previousEnv = environment;

        // Create new environment for function scope
        environment = std::make_shared<Environment>();

        // Bind parameters
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            environment->define(func->parameters[i], args[i]);
        }

        // Execute function body
        Value result;
        try {
            execute(func->body.get());
            result = Value(); // Return null if no explicit return
        } catch (const Value& returnValue) {
            result = returnValue;
        }

        // Restore environment
        environment = previousEnv;

        return result;
    }

    // Handle method calls on ClassInstance
    if (callee.holds_alternative<std::shared_ptr<ClassInstance>>()) {
        // This case handles: instance() - trying to call an instance as function
        throw std::runtime_error("Cannot call a class instance as a function.");
    }

    throw std::runtime_error("Cannot call non-function value.");
}

void Interpreter::execute(const Statement* stmt) {
    if (auto print = dynamic_cast<const PrintStmt*>(stmt)) {
        Value val = evalExpr(print->expression.get());
        std::visit([](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>) {
                std::cout << (v ? "true" : "false") << std::endl;
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                std::cout << "null" << std::endl;
            } else if constexpr (std::is_same_v<T, std::vector<Value>>) {
                std::cout << "[";
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::visit([](auto&& elem) {
                        using E = std::decay_t<decltype(elem)>;
                        if constexpr (std::is_same_v<E, std::monostate>) {
                            std::cout << "null";
                        } else if constexpr (std::is_same_v<E, std::vector<Value>>) {
                            std::cout << "[list]";
                        } else if constexpr (std::is_same_v<E, std::unordered_map<std::string, Value>>) {
                            std::cout << "{struct}";
                        } else if constexpr (std::is_same_v<E, std::shared_ptr<ClassInstance>>) { // Changed T to E
                            std::cout << "{instance of class}";
                        } else if constexpr (std::is_same_v<E, const FunctionStmt*>) { // Handle FunctionStmt* in list
                            std::cout << "{function}";
                        } else if constexpr (std::is_same_v<E, NativeFunction>) { // Handle NativeFunction in list
                            std::cout << "{native fn}";
                        }else {
                            std::cout << elem;
                        }
                    }, v[i].data); // Pass data member
                }
                std::cout << "]" << std::endl;
            } else if constexpr (std::is_same_v<T, std::unordered_map<std::string, Value>>) {
                std::cout << "{";
                bool first = true;
                for (const auto& [key, val] : v) {
                    if (!first) std::cout << ", ";
                    first = false;
                    std::cout << key << ": ";
                    std::visit([](auto&& elem) {
                        using E = std::decay_t<decltype(elem)>;
                        if constexpr (std::is_same_v<E, std::monostate>) {
                            std::cout << "null";
                        } else if constexpr (std::is_same_v<E, std::vector<Value>>) {
                            std::cout << "[list]";
                        } else if constexpr (std::is_same_v<E, std::unordered_map<std::string, Value>>) {
                            std::cout << "{struct}";
                        } else if constexpr (std::is_same_v<E, const FunctionStmt*>) { // Handle FunctionStmt* in map
                            std::cout << "{function}";
                        } else if constexpr (std::is_same_v<E, NativeFunction>) { // Handle NativeFunction in map
                            std::cout << "{native fn}";
                        }else {
                            std::cout << elem;
                        }
                    }, val.data); // Pass data member
                }
                std::cout << "}" << std::endl;
            } else {
                std::cout << v << std::endl;
            }
        }, val.data);
    }
    else if (auto let = dynamic_cast<const LetStmt*>(stmt)) {
        if (auto varExpr = dynamic_cast<const VariableExpr*>(let->expression.get())) {
            auto it = structs.find(varExpr->name);
            if (it != structs.end()) {
                std::unordered_map<std::string, Value> instance;
                for (const auto& field : it->second->fields) {
                    instance[field] = Value();
                }
                variables[let->name] = instance;
                // Store mutability info
                if (!let->isMutable) {
                    immutableVars.insert(let->name);
                }
                return;
            }
        }
        Value val = evalExpr(let->expression.get());
        variables[let->name] = val;
        // Store mutability info
        if (!let->isMutable) {
            immutableVars.insert(let->name);
        }
    }
    else if (auto constStmt = dynamic_cast<const ConstStmt*>(stmt)) {
        // Constants are evaluated at "compile time" (here, execution time)
        Value val = evalExpr(constStmt->expression.get());
        variables[constStmt->name] = val;
        // Constants are always immutable
        immutableVars.insert(constStmt->name);
    }
    else if (auto assign = dynamic_cast<const AssignStmt*>(stmt)) {
        // Check if variable is immutable
        if (immutableVars.find(assign->name) != immutableVars.end()) {
            throw std::runtime_error("Cannot assign to immutable variable: " + assign->name);
        }
        Value val = evalExpr(assign->expression.get());
        if (variables.find(assign->name) == variables.end()) {
            throw std::runtime_error("Undeclared variable: " + assign->name);
        }
        variables[assign->name] = val;
    }
    else if (auto set = dynamic_cast<const SetStmt*>(stmt)) {
        auto object = evalExpr(set->object.get());
        auto index = evalExpr(set->index.get());
        auto value = evalExpr(set->value.get());
    
        if (object.holds_alternative<std::shared_ptr<ObjectInstance>>()) {
            auto instance = object.get<std::shared_ptr<ObjectInstance>>();
            if (!index.holds_alternative<std::string>()) throw std::runtime_error("Property key must be string.");
            std::string key = index.get<std::string>();
            instance->fields[key] = value;
        } else if (object.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = object.get<std::shared_ptr<ClassInstance>>();
            if (!index.holds_alternative<std::string>()) throw std::runtime_error("Property key must be string.");
            std::string key = index.get<std::string>();
            instance->fields[key] = value;
        } else {
            throw std::runtime_error("Trying to define ownership on something that is not an object or class instance.");
        }
    }    
    else if (auto ifstmt = dynamic_cast<const IfStmt*>(stmt)) {
        Value cond = evalExpr(ifstmt->condition.get());
        bool condResult = false;

        if (cond.holds_alternative<bool>()) {
            condResult = cond.get<bool>();
        } else if (cond.holds_alternative<int>()) {
            condResult = cond.get<int>() != 0;
        } else {
            throw std::runtime_error("Invalid condition in if.");
        }

        if (condResult) {
            execute(ifstmt->thenBranch.get());
        } else if (ifstmt->elseBranch) {
            execute(ifstmt->elseBranch.get());
        }
    }
    else if (auto block = dynamic_cast<const BlockStmt*>(stmt)) {
        // Push new defer scope
        deferStack.push_back({});

        // Execute block statements
        try {
            for (const auto& inner : block->statements) {
                execute(inner.get());
            }
        } catch (...) {
            // Execute defers even on exception
            executeDeferredStatements();
            throw;
        }

        // Execute deferred statements before exiting scope
        executeDeferredStatements();
    }
    else if (auto func = dynamic_cast<const FunctionStmt*>(stmt)) {
        functions[func->name] = const_cast<FunctionStmt*>(func);
    }
    else if (auto structStmt = dynamic_cast<const StructStmt*>(stmt)) {
        structs[structStmt->name] = const_cast<StructStmt*>(structStmt);
    }
    else if (auto classStmt = dynamic_cast<const ClassStmt*>(stmt)) {
        classes[classStmt->name] = const_cast<ClassStmt*>(classStmt);
    }    
    else if (auto ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        Value val = evalExpr(ret->value.get());
        throw val;
    }
    else if (auto exprStmt = dynamic_cast<const ExpressionStmt*>(stmt)) {
        evalExpr(exprStmt->expression.get());
    }
        else if (auto indexAssign = dynamic_cast<const IndexAssignStmt*>(stmt)) {
            auto varExpr = dynamic_cast<VariableExpr*>(indexAssign->listExpr.get());
            if (!varExpr) {
                throw std::runtime_error("The index expression must be a variable.");
            }
        
            Value& var = variables[varExpr->name];
            Value idxVal = evalExpr(indexAssign->indexExpr.get());
            Value value = evalExpr(indexAssign->valueExpr.get());
        
                    if (var.holds_alternative<std::vector<Value>>()) {
                        auto& vec = var.get<std::vector<Value>>();

                // Accept both int and double (convert double to int)
                int idx;
                if (idxVal.holds_alternative<int>()) {
                    idx = idxVal.get<int>();
                } else if (idxVal.holds_alternative<double>()) {
                    idx = static_cast<int>(idxVal.get<double>());
                } else {
                    throw std::runtime_error("List index must be an integer.");
                }

                if (idx < 0 || idx >= (int)vec.size())
                    throw std::runtime_error("Invalid index for list assignment.");

                const_cast<std::vector<Value>&>(vec)[idx] = value;
            }
                    else if (var.holds_alternative<std::unordered_map<std::string, Value>>()) {
                        auto& map = var.get<std::unordered_map<std::string, Value>>();
                if (!idxVal.holds_alternative<std::string>())
                    throw std::runtime_error("Struct field must be a string.");

                const std::string& key = idxVal.get<std::string>();

                if (map.find(key) == map.end())
                    throw std::runtime_error("Field '" + key + "' does not exist in struct.");

                const_cast<std::unordered_map<std::string, Value>&>(map)[key] = value;
            }
                    else if (var.holds_alternative<std::shared_ptr<ClassInstance>>()) {
                        auto instance = var.get<std::shared_ptr<ClassInstance>>();
                if (!idxVal.holds_alternative<std::string>())
                    throw std::runtime_error("Class field index must be a string.");
                const std::string& key = idxVal.get<std::string>();
                if (instance->fields.find(key) == instance->fields.end())
                    throw std::runtime_error("Field '" + key + "' does not exist in class instance.");
                instance->fields[key] = value;
            }
            else {
                throw std::runtime_error("Attempt to index something that is neither a list, struct, nor class instance for assignment.");
            }
        }    
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        while (true) {
            Value cond = evalExpr(whileStmt->condition.get());
            bool condResult = false;

            if (cond.holds_alternative<bool>())
                condResult = cond.get<bool>();
            else if (cond.holds_alternative<int>())
                condResult = cond.get<int>() != 0;
            else
                throw std::runtime_error("Invalid condition in while.");

            if (!condResult) break;

            try {
                execute(whileStmt->body.get());
            } catch (const std::runtime_error& ex) {
                if (ex.what() == BREAK_SIGNAL) break;
                if (ex.what() == CONTINUE_SIGNAL) continue;
                throw;
            }
        }
    }
    else if (auto loopStmt = dynamic_cast<const LoopStmt*>(stmt)) {
        // Infinite loop - must use break to exit
        while (true) {
            try {
                execute(loopStmt->body.get());
            } catch (const std::runtime_error& ex) {
                if (ex.what() == BREAK_SIGNAL) break;
                if (ex.what() == CONTINUE_SIGNAL) continue;
                throw;
            }
        }
    }
    else if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        Value listVal = evalExpr(forStmt->iterable.get());

        if (!listVal.holds_alternative<std::vector<Value>>())
            throw std::runtime_error("Invalid iterable in for.");

        auto& vec = listVal.get<std::vector<Value>>();

        for (const auto& item : vec) {
            variables[forStmt->var] = item;

            try {
                execute(forStmt->body.get());
            } catch (const std::runtime_error& ex) {
                if (ex.what() == BREAK_SIGNAL) break;
                if (ex.what() == CONTINUE_SIGNAL) continue;
                throw;
            }
        }
    }
    else if (dynamic_cast<const BreakStmt*>(stmt)) {
        throw std::runtime_error(BREAK_SIGNAL);
    }
    else if (dynamic_cast<const ContinueStmt*>(stmt)) {
        throw std::runtime_error(CONTINUE_SIGNAL);
    }
    else if (auto en = dynamic_cast<const EnumStmt*>(stmt)) {
        int value = 0;
        for (const auto& name : en->values) {
            variables[en->name + "." + name] = value++;
        }
    }
    else if (auto m = dynamic_cast<const MatchStmt*>(stmt)) {
        Value val = evalExpr(m->expr.get());

        bool matched = false;
        for (const auto& arm : m->arms) {
            std::unordered_map<std::string, Value> bindings;

            if (matchPattern(arm.pattern.get(), val, bindings)) {
                // Save current variables
                auto savedVars = variables;

                // Add bindings to variables
                for (const auto& [name, value] : bindings) {
                    variables[name] = value;
                }

                // Execute arm body
                try {
                    execute(arm.body.get());
                } catch (...) {
                    // Restore variables before rethrowing
                    variables = savedVars;
                    throw;
                }

                // Restore variables
                variables = savedVars;

                matched = true;
                break;
            }
        }

        if (!matched) {
            throw std::runtime_error("No pattern matched in match expression.");
        }
    }
    else if (auto sw = dynamic_cast<const SwitchStmt*>(stmt)) {
        Value val = evalExpr(sw->expr.get());

        bool matched = false;
        for (const auto& [caseValExpr, caseStmt] : sw->cases) {
            Value caseVal = evalExpr(caseValExpr.get());
            if (val == caseVal) {
                execute(caseStmt.get());
                matched = true;
                break;
            }
        }

        if (!matched && sw->defaultCase) {
            execute(sw->defaultCase.get());
        }
    }
    else if (auto deferStmt = dynamic_cast<const DeferStmt*>(stmt)) {
        // Add to current defer scope
        if (deferStack.empty()) {
            throw std::runtime_error("Defer statement outside of scope");
        }
        deferStack.back().push_back(deferStmt->statement.get());
    }
    else if (auto assertStmt = dynamic_cast<const AssertStmt*>(stmt)) {
        // Evaluate assertion condition
        Value condition = evalExpr(assertStmt->condition.get());

        bool result = false;
        if (condition.holds_alternative<bool>()) {
            result = condition.get<bool>();
        } else if (condition.holds_alternative<int>()) {
            result = condition.get<int>() != 0;
        } else {
            throw std::runtime_error("Assertion condition must be boolean");
        }

        // If assertion fails, throw error
        if (!result) {
            std::string message = assertStmt->message.empty()
                ? "Assertion failed"
                : "Assertion failed: " + assertStmt->message;
            throw std::runtime_error(message);
        }
    }
    else if (auto classStmt = dynamic_cast<const ClassStmt*>(stmt)) {
        std::unordered_map<std::string, Value> classFields;
        for (const auto& field : classStmt->fields) {
            classFields[field] = Value();
        }

        for (const auto& method : classStmt->methods) {
            functions[classStmt->name + "." + method->name] = method.get();
        }

        classes[classStmt->name] = const_cast<ClassStmt*>(classStmt);
    }
}

void Interpreter::executeDeferredStatements() {
    if (deferStack.empty()) return;

    // Get current scope's deferred statements
    auto& defers = deferStack.back();

    // Execute in reverse order (LIFO)
    for (auto it = defers.rbegin(); it != defers.rend(); ++it) {
        try {
            execute(*it);
        } catch (const std::exception& e) {
            std::cerr << "Error in deferred statement: " << e.what() << std::endl;
            // Continue executing other defers even if one fails
        }
    }

    // Pop defer scope
    deferStack.pop_back();
}

// Pattern matching helper function
bool Interpreter::matchPattern(const Pattern* pattern, const Value& value, std::unordered_map<std::string, Value>& bindings) {
    // Wildcard pattern: always matches
    if (auto wildcard = dynamic_cast<const WildcardPattern*>(pattern)) {
        return true;
    }

    // Literal pattern: exact match
    if (auto literal = dynamic_cast<const LiteralPattern*>(pattern)) {
        return value == literal->value;
    }

    // Variable pattern: always matches and binds
    if (auto var = dynamic_cast<const VariablePattern*>(pattern)) {
        bindings[var->name] = value;
        return true;
    }

    // Range pattern: check if value is in range
    if (auto range = dynamic_cast<const RangePattern*>(pattern)) {
        // Convert value to int for comparison
        int val = 0;
        if (value.holds_alternative<int>()) {
            val = value.get<int>();
        } else if (value.holds_alternative<double>()) {
            val = static_cast<int>(value.get<double>());
        } else {
            return false;  // Not a numeric value
        }

        int start = 0, end = 0;
        if (range->start.holds_alternative<int>()) {
            start = range->start.get<int>();
        } else if (range->start.holds_alternative<double>()) {
            start = static_cast<int>(range->start.get<double>());
        }

        if (range->end.holds_alternative<int>()) {
            end = range->end.get<int>();
        } else if (range->end.holds_alternative<double>()) {
            end = static_cast<int>(range->end.get<double>());
        }

        if (range->inclusive) {
            return val >= start && val <= end;
        } else {
            return val >= start && val < end;
        }
    }

    // Tuple pattern: match elements
    if (auto tuple = dynamic_cast<const TuplePattern*>(pattern)) {
        if (!value.holds_alternative<std::vector<Value>>()) {
            return false;
        }

        const auto& vec = value.get<std::vector<Value>>();
        if (vec.size() != tuple->patterns.size()) {
            return false;
        }

        for (size_t i = 0; i < vec.size(); ++i) {
            if (!matchPattern(tuple->patterns[i].get(), vec[i], bindings)) {
                return false;
            }
        }

        return true;
    }

    // Struct pattern: match struct fields
    if (auto structPat = dynamic_cast<const StructPattern*>(pattern)) {
        // Check if value is a ClassInstance with matching name
        if (value.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            const auto& instance = value.get<std::shared_ptr<ClassInstance>>();

            if (instance->className != structPat->structName) {
                return false;
            }

            // Match each field pattern
            for (const auto& [fieldName, fieldPattern] : structPat->fields) {
                auto it = instance->fields.find(fieldName);
                if (it == instance->fields.end()) {
                    return false;  // Field not found
                }

                if (!matchPattern(fieldPattern.get(), it->second, bindings)) {
                    return false;
                }
            }

            return true;
        }

        // Check if value is an ObjectInstance
        if (value.holds_alternative<std::shared_ptr<ObjectInstance>>()) {
            const auto& obj = value.get<std::shared_ptr<ObjectInstance>>();

            // Match each field pattern
            for (const auto& [fieldName, fieldPattern] : structPat->fields) {
                auto it = obj->fields.find(fieldName);
                if (it == obj->fields.end()) {
                    return false;  // Field not found
                }

                if (!matchPattern(fieldPattern.get(), it->second, bindings)) {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    // Or pattern: try each alternative
    if (auto orPat = dynamic_cast<const OrPattern*>(pattern)) {
        for (const auto& subPattern : orPat->patterns) {
            std::unordered_map<std::string, Value> tempBindings;
            if (matchPattern(subPattern.get(), value, tempBindings)) {
                // Merge bindings
                for (const auto& [name, val] : tempBindings) {
                    bindings[name] = val;
                }
                return true;
            }
        }
        return false;
    }

    // Guarded pattern: match pattern then check guard
    if (auto guarded = dynamic_cast<const GuardedPattern*>(pattern)) {
        std::unordered_map<std::string, Value> tempBindings;
        if (!matchPattern(guarded->pattern.get(), value, tempBindings)) {
            return false;
        }

        // Temporarily add bindings to variables for guard evaluation
        auto savedVars = variables;
        for (const auto& [name, val] : tempBindings) {
            variables[name] = val;
        }

        // Evaluate guard
        Value guardResult = evalExpr(guarded->guard.get());

        // Restore variables
        variables = savedVars;

        // Check if guard is true
        bool guardPassed = false;
        if (guardResult.holds_alternative<bool>()) {
            guardPassed = guardResult.get<bool>();
        } else if (guardResult.holds_alternative<int>()) {
            guardPassed = guardResult.get<int>() != 0;
        }

        if (guardPassed) {
            // Guard passed, merge bindings
            for (const auto& [name, val] : tempBindings) {
                bindings[name] = val;
            }
            return true;
        }

        return false;
    }

    return false;
}

void Interpreter::execute(const std::vector<std::unique_ptr<Statement>>& statements) {
    for (const auto& stmt : statements) {
        execute(stmt.get());
    }
}