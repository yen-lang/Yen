#include "yen/value.h"
#include "yen/compiler.h"
#include "yen/lexer.h"
#include "yen/parser.h"
#include "yen/native_libs.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <filesystem>
#include <thread>

// ============================================================================
// Helper: Convert any Value to a string representation
// ============================================================================
std::string Interpreter::valueToString(const Value& val) {
    return std::visit(overloaded {
        [](std::monostate) -> std::string { return "null"; },
        [](int v) -> std::string { return std::to_string(v); },
        [](double v) -> std::string {
            std::string s = std::to_string(v);
            // Remove trailing zeros after decimal point for cleaner output
            size_t dot = s.find('.');
            if (dot != std::string::npos) {
                size_t last = s.find_last_not_of('0');
                if (last != std::string::npos && last > dot) {
                    s = s.substr(0, last + 1);
                } else if (last == dot) {
                    s = s.substr(0, dot + 2); // Keep at least one digit after dot
                }
            }
            return s;
        },
        [](float v) -> std::string {
            std::string s = std::to_string(v);
            size_t dot = s.find('.');
            if (dot != std::string::npos) {
                size_t last = s.find_last_not_of('0');
                if (last != std::string::npos && last > dot) {
                    s = s.substr(0, last + 1);
                } else if (last == dot) {
                    s = s.substr(0, dot + 2);
                }
            }
            return s;
        },
        [](bool v) -> std::string { return v ? "true" : "false"; },
        [](const std::string& v) -> std::string { return v; },
        [this](const std::vector<Value>& v) -> std::string {
            std::string result = "[";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i > 0) result += ", ";
                result += valueToString(v[i]);
            }
            result += "]";
            return result;
        },
        [this](const std::unordered_map<std::string, Value>& v) -> std::string {
            std::string result = "{";
            bool first = true;
            for (const auto& [key, val] : v) {
                if (!first) result += ", ";
                first = false;
                result += key + ": " + valueToString(val);
            }
            result += "}";
            return result;
        },
        [this](const std::shared_ptr<ClassInstance>& v) -> std::string {
            // Check for toString() method
            auto toStrIt = functions.find(v->className + ".toString");
            if (toStrIt != functions.end()) {
                auto previousEnv = environment;
                environment = std::make_shared<Environment>();
                environment->define("this", Value(v));
                Value result;
                try {
                    execute(toStrIt->second->body.get());
                    result = Value();
                } catch (const Value& returnValue) {
                    result = returnValue;
                }
                environment = previousEnv;
                if (result.holds_alternative<std::string>()) {
                    return result.get<std::string>();
                }
                return valueToString(result);
            }
            // Data class auto-toString: ClassName(field1: value1, field2: value2)
            auto classDefIt = classes.find(v->className);
            if (classDefIt != classes.end() && classDefIt->second->isDataClass) {
                std::string result = v->className + "(";
                const auto& fields = classDefIt->second->fields;
                for (size_t i = 0; i < fields.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += fields[i] + ": ";
                    auto fieldIt = v->fields.find(fields[i]);
                    if (fieldIt != v->fields.end()) {
                        result += valueToString(fieldIt->second);
                    } else {
                        result += "None";
                    }
                }
                result += ")";
                return result;
            }
            return "{instance of " + v->className + "}";
        },
        [](const std::shared_ptr<ObjectInstance>&) -> std::string {
            return "{object}";
        },
        [](const FunctionStmt* v) -> std::string {
            return "{function " + v->name + "}";
        },
        [](const NativeFunction&) -> std::string {
            return "{native fn}";
        },
        [](const LambdaValue&) -> std::string {
            return "{lambda}";
        }
    }, val.data);
}

// ============================================================================
// Helper: Convert any Value to bool (truthiness)
// ============================================================================
bool Interpreter::isTruthy(const Value& val) {
    return std::visit(overloaded {
        [](std::monostate) { return false; },
        [](int v) { return v != 0; },
        [](double v) { return v != 0.0; },
        [](float v) { return v != 0.0f; },
        [](bool v) { return v; },
        [](const std::string& v) { return !v.empty(); },
        [](const std::vector<Value>&) { return true; },
        [](const std::unordered_map<std::string, Value>&) { return true; },
        [](const std::shared_ptr<ClassInstance>&) { return true; },
        [](const std::shared_ptr<ObjectInstance>&) { return true; },
        [](const FunctionStmt*) { return true; },
        [](const NativeFunction&) { return true; },
        [](const LambdaValue&) { return true; }
    }, val.data);
}

// ============================================================================
// Constructor - register all native libraries
// ============================================================================
Interpreter::Interpreter() {
    YenNative::registerAllLibraries(variables);
    initNativeModuleRegistry();
}

void Interpreter::initNativeModuleRegistry() {
    nativeModules["regex"] = YenNative::Regex::registerFunctions;
    nativeModules["net.socket"] = YenNative::NetSocket::registerFunctions;
    nativeModules["net.http"] = YenNative::NetHTTP::registerFunctions;
    nativeModules["os"] = YenNative::OS::registerFunctions;
    nativeModules["async"] = YenNative::Async::registerFunctions;
    // Phase 5: Standard libraries
    nativeModules["datetime"] = YenNative::DateTime::registerFunctions;
    nativeModules["testing"] = YenNative::Testing::registerFunctions;
    nativeModules["color"] = YenNative::Color::registerFunctions;
    nativeModules["set"] = YenNative::Set::registerFunctions;
    nativeModules["path"] = YenNative::Path::registerFunctions;
    nativeModules["csv"] = YenNative::CSV::registerFunctions;
    nativeModules["event"] = YenNative::Event::registerFunctions;
    // Aliases for convenience
    nativeModules["net"] = [](std::unordered_map<std::string, Value>& g) {
        YenNative::NetSocket::registerFunctions(g);
        YenNative::NetHTTP::registerFunctions(g);
    };
}

bool Interpreter::loadNativeModule(const std::string& modulePath) {
    auto it = nativeModules.find(modulePath);
    if (it == nativeModules.end()) return false;
    // Only load once
    if (loadedNativeModules.count(modulePath)) return true;
    loadedNativeModules.insert(modulePath);
    it->second(variables);
    return true;
}

// ============================================================================
// Expression evaluation
// ============================================================================
Value Interpreter::evalExpr(const Expression* expr) {
    // ---- NumberExpr: check isInteger flag ----
    if (auto num = dynamic_cast<const NumberExpr*>(expr)) {
        if (num->isInteger) {
            return static_cast<int>(num->value);
        }
        return num->value;  // double
    }

    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        return lit->value;
    }

    // ---- CastExpr ----
    if (auto castExpr = dynamic_cast<const CastExpr*>(expr)) {
        Value val = evalExpr(castExpr->expression.get());
        const std::string& targetType = castExpr->targetType;

        if (targetType == "int" || targetType == "int32" || targetType == "int64") {
            if (val.holds_alternative<double>()) {
                return static_cast<int>(val.get<double>());
            } else if (val.holds_alternative<float>()) {
                return static_cast<int>(val.get<float>());
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
            } else if (val.holds_alternative<float>()) {
                return static_cast<double>(val.get<float>());
            } else if (val.holds_alternative<bool>()) {
                return val.get<bool>() ? 1.0 : 0.0;
            } else if (val.holds_alternative<std::string>()) {
                return std::stod(val.get<std::string>());
            }
        } else if (targetType == "bool") {
            return isTruthy(val);
        } else if (targetType == "string" || targetType == "str") {
            return valueToString(val);
        }

        throw std::runtime_error("Invalid cast from " +
            std::string(val.data.index() == 0 ? "null" : "unknown") +
            " to " + targetType);
    }

    // ---- LambdaExpr (expression or block body) ----
    if (auto lambdaExpr = dynamic_cast<const LambdaExpr*>(expr)) {
        auto captured = std::make_shared<std::unordered_map<std::string, Value>>(variables);
        LambdaValue lambda = lambdaExpr->blockBody
            ? LambdaValue(lambdaExpr->parameters, nullptr, lambdaExpr->blockBody.get(), captured)
            : LambdaValue(lambdaExpr->parameters, lambdaExpr->body.get(), captured);

        // Capture default expressions
        if (!lambdaExpr->parameterDefaults.empty()) {
            auto defaults = std::make_shared<std::vector<const Expression*>>();
            for (const auto& d : lambdaExpr->parameterDefaults) {
                defaults->push_back(d ? d.get() : nullptr);
            }
            lambda.defaultExprs = defaults;
        }

        return lambda;
    }

    // ---- RangeExpr ----
    if (auto rangeExpr = dynamic_cast<const RangeExpr*>(expr)) {
        Value startVal = evalExpr(rangeExpr->start.get());
        Value endVal = evalExpr(rangeExpr->end.get());

        int start, end;
        if (startVal.holds_alternative<int>()) {
            start = startVal.get<int>();
        } else if (startVal.holds_alternative<double>()) {
            start = static_cast<int>(startVal.get<double>());
        } else {
            throw std::runtime_error("Range start must be numeric.");
        }

        if (endVal.holds_alternative<int>()) {
            end = endVal.get<int>();
        } else if (endVal.holds_alternative<double>()) {
            end = static_cast<int>(endVal.get<double>());
        } else {
            throw std::runtime_error("Range end must be numeric.");
        }

        std::vector<Value> result;
        if (rangeExpr->inclusive) {
            for (int i = start; i <= end; ++i) {
                result.push_back(i);
            }
        } else {
            for (int i = start; i < end; ++i) {
                result.push_back(i);
            }
        }

        return result;
    }

    // ---- IsExpr: type checking ----
    if (auto isExpr = dynamic_cast<const IsExpr*>(expr)) {
        Value obj = evalExpr(isExpr->object.get());
        const std::string& typeName = isExpr->typeName;

        if (typeName == "int") return Value(obj.holds_alternative<int>());
        if (typeName == "float") return Value(obj.holds_alternative<double>() || obj.holds_alternative<float>());
        if (typeName == "string" || typeName == "str") return Value(obj.holds_alternative<std::string>());
        if (typeName == "bool") return Value(obj.holds_alternative<bool>());
        if (typeName == "list") return Value(obj.holds_alternative<std::vector<Value>>());
        if (typeName == "map") return Value(obj.holds_alternative<std::unordered_map<std::string, Value>>());
        if (typeName == "function" || typeName == "func") {
            return Value(obj.holds_alternative<const FunctionStmt*>() ||
                        obj.holds_alternative<NativeFunction>() ||
                        obj.holds_alternative<LambdaValue>());
        }
        if (typeName == "null" || typeName == "None") return Value(obj.holds_alternative<std::monostate>());

        // Check class name (including inheritance chain)
        if (obj.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = obj.get<std::shared_ptr<ClassInstance>>();
            std::string cn = instance->className;
            while (!cn.empty()) {
                if (cn == typeName) return Value(true);
                // Check traits
                auto traitIt = classTraits.find(cn);
                if (traitIt != classTraits.end()) {
                    for (const auto& t : traitIt->second) {
                        if (t == typeName) return Value(true);
                    }
                }
                // Walk parent chain
                auto classIt = classes.find(cn);
                if (classIt != classes.end() && !classIt->second->parentName.empty()) {
                    cn = classIt->second->parentName;
                } else {
                    break;
                }
            }
            return Value(false);
        }

        return Value(false);
    }

    // ---- SuperExpr: super.method ----
    if (auto superExpr = dynamic_cast<const SuperExpr*>(expr)) {
        // Get 'this' from current environment
        Value thisVal = environment->get("this");
        if (!thisVal.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            throw std::runtime_error("'super' used outside of class context.");
        }
        auto instance = thisVal.get<std::shared_ptr<ClassInstance>>();

        // Find parent class
        std::string parentClass;
        auto classIt = classes.find(currentClassName.empty() ? instance->className : currentClassName);
        if (classIt != classes.end()) {
            parentClass = classIt->second->parentName;
        }
        if (parentClass.empty()) {
            parentClass = instance->parentClassName;
        }
        if (parentClass.empty()) {
            throw std::runtime_error("'super' used in class with no parent.");
        }

        // Look up parent method
        std::string methodKey = parentClass + "." + superExpr->methodName;
        auto funcIt = functions.find(methodKey);
        if (funcIt == functions.end()) {
            throw std::runtime_error("Method '" + superExpr->methodName + "' not found in parent class '" + parentClass + "'.");
        }

        // Return the function pointer - it will be called via finishAccessAndCall -> CallExpr
        return funcIt->second;
    }

    // ---- OptionalGetExpr: obj?.field ----
    if (auto optGet = dynamic_cast<const OptionalGetExpr*>(expr)) {
        Value object = evalExpr(optGet->object.get());
        if (object.holds_alternative<std::monostate>()) {
            return Value();  // null propagation
        }
        // Delegate to normal field access logic
        if (object.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = object.get<std::shared_ptr<ClassInstance>>();
            auto it = instance->fields.find(optGet->name);
            if (it != instance->fields.end()) return it->second;
            return Value();  // field not found → null
        }
        if (object.holds_alternative<std::unordered_map<std::string, Value>>()) {
            const auto& map = object.get<std::unordered_map<std::string, Value>>();
            auto it = map.find(optGet->name);
            if (it != map.end()) return it->second;
            return Value();
        }
        return Value();
    }

    // ---- ThisExpr ----
    if (auto thisExpr = dynamic_cast<const ThisExpr*>(expr)) {
        return environment->get("this");
    }

    // ---- GetExpr: field access ----
    if (auto getExpr = dynamic_cast<const GetExpr*>(expr)) {
        Value object = evalExpr(getExpr->object.get());

        if (object.holds_alternative<std::shared_ptr<ObjectInstance>>()) {
            auto instance = object.get<std::shared_ptr<ObjectInstance>>();
            auto it = instance->fields.find(getExpr->name);
            if (it != instance->fields.end()) {
                return it->second;
            }
            throw std::runtime_error("Field '" + getExpr->name + "' not found in ObjectInstance.");
        } else if (object.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = object.get<std::shared_ptr<ClassInstance>>();

            // Check access modifier
            std::string accessKey = instance->className + "." + getExpr->name;
            auto accessIt = accessModifiers.find(accessKey);
            if (accessIt != accessModifiers.end() && accessIt->second == AccessModifier::Private) {
                if (currentClassName != instance->className) {
                    throw std::runtime_error("Cannot access private member '" + getExpr->name + "' of class '" + instance->className + "'.");
                }
            }

            // Check for getter
            std::string getterKey = instance->className + ".__getter_" + getExpr->name;
            auto getterIt = functions.find(getterKey);
            if (getterIt != functions.end()) {
                auto previousEnv = environment;
                environment = std::make_shared<Environment>();
                environment->define("this", Value(instance));
                auto savedClassName = currentClassName;
                currentClassName = instance->className;
                Value result;
                try {
                    execute(getterIt->second->body.get());
                    result = Value();
                } catch (const Value& returnValue) {
                    result = returnValue;
                }
                currentClassName = savedClassName;
                environment = previousEnv;
                return result;
            }

            auto it = instance->fields.find(getExpr->name);
            if (it != instance->fields.end()) {
                // Check if this is a lazy field that hasn't been evaluated yet
                if (it->second.holds_alternative<std::monostate>()) {
                    // Check if there's a lazy initializer
                    auto classIt = classes.find(instance->className);
                    if (classIt != classes.end()) {
                        for (const auto& [lazyName, lazyExpr] : classIt->second->lazyFields) {
                            if (lazyName == getExpr->name && lazyExpr) {
                                // Evaluate with 'this' bound
                                auto previousEnv = environment;
                                environment = std::make_shared<Environment>();
                                environment->define("this", Value(instance));
                                Value result = evalExpr(lazyExpr.get());
                                environment = previousEnv;
                                // Cache the result
                                instance->fields[getExpr->name] = result;
                                return result;
                            }
                        }
                    }
                }
                return it->second;
            }
            // Check for static fields/methods: ClassName.staticField
            std::string staticKey = instance->className + "." + getExpr->name;
            auto staticVarIt = variables.find(staticKey);
            if (staticVarIt != variables.end()) {
                return staticVarIt->second;
            }

            // Check for class methods (walk inheritance chain)
            std::string cn = instance->className;
            while (!cn.empty()) {
                std::string methodName = cn + "." + getExpr->name;
                auto funcIt = functions.find(methodName);
                if (funcIt != functions.end()) {
                    return funcIt->second;
                }
                auto classIt = classes.find(cn);
                if (classIt != classes.end() && !classIt->second->parentName.empty()) {
                    cn = classIt->second->parentName;
                } else {
                    break;
                }
            }
            throw std::runtime_error("Field '" + getExpr->name + "' not found in ClassInstance.");
        } else if (object.holds_alternative<std::unordered_map<std::string, Value>>()) {
            const auto& map = object.get<std::unordered_map<std::string, Value>>();
            auto it = map.find(getExpr->name);
            if (it != map.end()) {
                return it->second;
            }
            throw std::runtime_error("Field '" + getExpr->name + "' not found in map.");
        }
        throw std::runtime_error("Attempt to access a field on something that is not an object or class instance.");
    }

    // ---- VariableExpr ----
    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        // First check environment (for function parameters and local scope)
        if (environment && environment->values.count(var->name)) {
            return environment->values[var->name];
        }

        // Then check global variables
        auto it = variables.find(var->name);
        if (it != variables.end()) {
            return it->second;
        }

        // Check if it's a function
        auto funcIt = functions.find(var->name);
        if (funcIt != functions.end()) {
            return funcIt->second;
        }

        // Check if it's a class (constructor-like usage)
        auto classIt = classes.find(var->name);
        if (classIt != classes.end()) {
            auto instance = std::make_shared<ClassInstance>();
            instance->className = var->name;
            // Copy fields from class (and parent chain)
            std::string cn = var->name;
            while (!cn.empty()) {
                auto cIt = classes.find(cn);
                if (cIt != classes.end()) {
                    for (const auto& field : cIt->second->fields) {
                        if (instance->fields.find(field) == instance->fields.end()) {
                            instance->fields[field] = Value();
                        }
                    }
                    cn = cIt->second->parentName;
                } else {
                    break;
                }
            }
            // Set parent class name for inheritance chain
            if (!classIt->second->parentName.empty()) {
                instance->parentClassName = classIt->second->parentName;
            }
            return instance;
        }

        throw std::runtime_error("Undefined variable: " + var->name);
    }

    // ---- BoolExpr ----
    if (auto b = dynamic_cast<const BoolExpr*>(expr)) {
        return b->value;
    }

    // ---- InputExpr ----
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

    // ---- ListExpr (with spread support) ----
    if (auto list = dynamic_cast<const ListExpr*>(expr)) {
        std::vector<Value> result;
        for (const auto& e : list->elements) {
            if (auto spread = dynamic_cast<const SpreadExpr*>(e.get())) {
                Value spreadVal = evalExpr(spread->expression.get());
                if (spreadVal.holds_alternative<std::vector<Value>>()) {
                    const auto& inner = spreadVal.get<std::vector<Value>>();
                    result.insert(result.end(), inner.begin(), inner.end());
                } else {
                    throw std::runtime_error("Spread operator requires a list.");
                }
            } else {
                result.push_back(evalExpr(e.get()));
            }
        }
        return result;
    }

    // ---- MapExpr: evaluate key-value pairs ----
    if (auto mapExpr = dynamic_cast<const MapExpr*>(expr)) {
        std::unordered_map<std::string, Value> result;
        for (const auto& [keyExpr, valExpr] : mapExpr->pairs) {
            Value key = evalExpr(keyExpr.get());
            Value val = evalExpr(valExpr.get());
            if (!key.holds_alternative<std::string>()) {
                // Convert key to string if not already
                result[valueToString(key)] = val;
            } else {
                result[key.get<std::string>()] = val;
            }
        }
        return result;
    }

    // ---- IndexExpr ----
    if (auto indexExpr = dynamic_cast<const IndexExpr*>(expr)) {
        Value container = evalExpr(indexExpr->listExpr.get());
        Value index = evalExpr(indexExpr->indexExpr.get());

        if (container.holds_alternative<std::vector<Value>>()) {
            const auto& list = container.get<std::vector<Value>>();
            int idx;
            if (index.holds_alternative<int>()) {
                idx = index.get<int>();
            } else if (index.holds_alternative<double>()) {
                idx = static_cast<int>(index.get<double>());
            } else {
                throw std::runtime_error("List index must be an integer.");
            }

            // Negative indexing: list[-1] = last element
            if (idx < 0) idx += static_cast<int>(list.size());

            if (idx < 0 || idx >= static_cast<int>(list.size()))
                throw std::runtime_error("List index out of bounds.");

            return list[idx];
        }
        else if (container.holds_alternative<std::unordered_map<std::string, Value>>()) {
            const auto& map = container.get<std::unordered_map<std::string, Value>>();

            std::string key;
            if (index.holds_alternative<std::string>()) {
                key = index.get<std::string>();
            } else {
                key = valueToString(index);
            }

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
        else if (container.holds_alternative<std::string>()) {
            // String indexing: return character at index
            const auto& str = container.get<std::string>();
            int idx;
            if (index.holds_alternative<int>()) {
                idx = index.get<int>();
            } else if (index.holds_alternative<double>()) {
                idx = static_cast<int>(index.get<double>());
            } else {
                throw std::runtime_error("String index must be an integer.");
            }
            // Negative indexing: str[-1] = last char
            if (idx < 0) idx += static_cast<int>(str.size());
            if (idx < 0 || idx >= static_cast<int>(str.size()))
                throw std::runtime_error("String index out of bounds.");
            return std::string(1, str[idx]);
        }
        else {
            throw std::runtime_error("Attempt to index something that is neither a list, struct, nor class instance.");
        }
    }

    // ---- SliceExpr: list[start:end], str[start:end] ----
    if (auto sliceExpr = dynamic_cast<const SliceExpr*>(expr)) {
        Value container = evalExpr(sliceExpr->object.get());

        auto getInt = [](const Value& v) -> int {
            if (v.holds_alternative<int>()) return v.get<int>();
            if (v.holds_alternative<double>()) return static_cast<int>(v.get<double>());
            throw std::runtime_error("Slice index must be numeric.");
        };

        if (container.holds_alternative<std::vector<Value>>()) {
            const auto& list = container.get<std::vector<Value>>();
            int size = static_cast<int>(list.size());
            int start = 0, end = size;
            if (sliceExpr->start) {
                start = getInt(evalExpr(sliceExpr->start.get()));
                if (start < 0) start += size;
                if (start < 0) start = 0;
            }
            if (sliceExpr->end) {
                end = getInt(evalExpr(sliceExpr->end.get()));
                if (end < 0) end += size;
                if (end > size) end = size;
            }
            if (start >= end) return std::vector<Value>();
            return std::vector<Value>(list.begin() + start, list.begin() + end);
        }
        else if (container.holds_alternative<std::string>()) {
            const auto& str = container.get<std::string>();
            int size = static_cast<int>(str.size());
            int start = 0, end = size;
            if (sliceExpr->start) {
                start = getInt(evalExpr(sliceExpr->start.get()));
                if (start < 0) start += size;
                if (start < 0) start = 0;
            }
            if (sliceExpr->end) {
                end = getInt(evalExpr(sliceExpr->end.get()));
                if (end < 0) end += size;
                if (end > size) end = size;
            }
            if (start >= end) return std::string("");
            return str.substr(start, end - start);
        }
        throw std::runtime_error("Slice requires a list or string.");
    }

    // ---- InterpolatedStringExpr: lex+parse expressions inside ${...} ----
    if (auto interp = dynamic_cast<const InterpolatedStringExpr*>(expr)) {
        std::string result;
        const std::string& src = interp->raw;

        size_t pos = 0;
        while (true) {
            size_t start = src.find("${", pos);
            if (start == std::string::npos) {
                result += src.substr(pos);
                break;
            }

            result += src.substr(pos, start - pos);

            // Find matching closing brace, accounting for nested braces
            size_t braceDepth = 1;
            size_t i = start + 2;
            while (i < src.size() && braceDepth > 0) {
                if (src[i] == '{') braceDepth++;
                else if (src[i] == '}') braceDepth--;
                if (braceDepth > 0) i++;
            }

            if (braceDepth != 0)
                throw std::runtime_error("Invalid string interpolation: unmatched '{'.");

            std::string exprText = src.substr(start + 2, i - start - 2);

            // Lex, parse, and evaluate the expression
            Lexer lexer(exprText);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto exprNode = parser.parseExpression();

            if (parser.hadError() || !exprNode) {
                throw std::runtime_error("Error parsing interpolated expression: " + exprText);
            }

            Value val = evalExpr(exprNode.get());
            result += valueToString(val);

            pos = i + 1;
        }

        return result;
    }

    // ---- TernaryExpr: condition ? then : else ----
    if (auto ternaryExpr = dynamic_cast<const TernaryExpr*>(expr)) {
        Value cond = evalExpr(ternaryExpr->condition.get());
        if (isTruthy(cond)) {
            return evalExpr(ternaryExpr->thenExpr.get());
        } else {
            return evalExpr(ternaryExpr->elseExpr.get());
        }
    }

    // ---- NullCoalesceExpr: expr ?? default ----
    if (auto nullCoalesce = dynamic_cast<const NullCoalesceExpr*>(expr)) {
        Value left = evalExpr(nullCoalesce->left.get());
        if (!left.holds_alternative<std::monostate>()) {
            return left;
        }
        return evalExpr(nullCoalesce->right.get());
    }

    // ---- SpreadExpr: ...expr (evaluated in context of list/call) ----
    if (auto spreadExpr = dynamic_cast<const SpreadExpr*>(expr)) {
        return evalExpr(spreadExpr->expression.get());
    }

    // ---- PipeExpr: left |> right => right(left) ----
    if (auto pipeExpr = dynamic_cast<const PipeExpr*>(expr)) {
        Value leftVal = evalExpr(pipeExpr->value.get());
        Value funcVal = evalExpr(pipeExpr->function.get());

        // Call the function with left as the first argument
        std::vector<Value> args;
        args.push_back(leftVal);
        return call(funcVal, args);
    }

    // ---- CallExpr ----
    if (auto callExpr = dynamic_cast<const CallExpr*>(expr)) {
        // Check if callee is a GetExpr (method call)
        if (auto getExpr = dynamic_cast<const GetExpr*>(callExpr->callee.get())) {
            Value object = evalExpr(getExpr->object.get());

            // Method call on ClassInstance
            if (object.holds_alternative<std::shared_ptr<ClassInstance>>()) {
                auto instance = object.get<std::shared_ptr<ClassInstance>>();

                // Walk inheritance chain to find method
                const FunctionStmt* method = nullptr;
                std::string foundInClass;
                std::string cn = instance->className;
                while (!cn.empty()) {
                    std::string methodName = cn + "." + getExpr->name;
                    auto funcIt = functions.find(methodName);
                    if (funcIt != functions.end()) {
                        method = funcIt->second;
                        foundInClass = cn;
                        break;
                    }
                    auto classIt = classes.find(cn);
                    if (classIt != classes.end() && !classIt->second->parentName.empty()) {
                        cn = classIt->second->parentName;
                    } else {
                        break;
                    }
                }

                if (method) {
                    // Check access modifier
                    std::string accessKey = foundInClass + "." + getExpr->name;
                    auto accessIt = accessModifiers.find(accessKey);
                    if (accessIt != accessModifiers.end() && accessIt->second == AccessModifier::Private) {
                        if (currentClassName != instance->className) {
                            throw std::runtime_error("Cannot access private method '" + getExpr->name + "' of class '" + instance->className + "'.");
                        }
                    }

                    // Check for abstract method (no body)
                    if (!method->body) {
                        throw std::runtime_error("Cannot call abstract method '" + getExpr->name + "'.");
                    }

                    std::vector<Value> arguments;
                    for (const auto& argExpr : callExpr->arguments) {
                        arguments.push_back(evalExpr(argExpr.get()));
                    }

                    // Fill in defaults
                    if (arguments.size() < method->parameters.size()) {
                        for (size_t i = arguments.size(); i < method->parameters.size(); ++i) {
                            if (i < method->parameterDefaults.size() && method->parameterDefaults[i]) {
                                arguments.push_back(evalExpr(method->parameterDefaults[i].get()));
                            }
                        }
                    }

                    // Save current environment
                    auto previousEnv = environment;
                    environment = std::make_shared<Environment>();
                    environment->define("this", instance);

                    auto savedClassName = currentClassName;
                    currentClassName = instance->className;

                    // Bind parameters
                    for (size_t i = 0; i < method->parameters.size() && i < arguments.size(); ++i) {
                        environment->define(method->parameters[i], arguments[i]);
                    }

                    Value result;
                    try {
                        execute(method->body.get());
                        result = Value();
                    } catch (const Value& returnValue) {
                        result = returnValue;
                    }

                    currentClassName = savedClassName;
                    environment = previousEnv;

                    // Method chaining: if method returns null (no explicit return) and
                    // method is not init/toString, return 'this' for chaining
                    if (result.holds_alternative<std::monostate>() &&
                        getExpr->name != "init" && getExpr->name != "toString") {
                        return Value(instance);
                    }

                    return result;
                }

                // Check field as callable
                auto fieldIt = instance->fields.find(getExpr->name);
                if (fieldIt != instance->fields.end()) {
                    std::vector<Value> arguments;
                    for (const auto& argExpr : callExpr->arguments) {
                        arguments.push_back(evalExpr(argExpr.get()));
                    }
                    return call(fieldIt->second, arguments);
                }

                // Built-in clone() method
                if (getExpr->name == "clone") {
                    // Check for __clone override
                    std::string cloneKey = instance->className + ".__clone";
                    auto cloneIt = functions.find(cloneKey);
                    if (cloneIt != functions.end() && cloneIt->second->body) {
                        auto previousEnv = environment;
                        environment = std::make_shared<Environment>();
                        environment->define("this", Value(instance));
                        Value result;
                        try {
                            execute(cloneIt->second->body.get());
                            result = Value();
                        } catch (const Value& returnValue) {
                            result = returnValue;
                        }
                        environment = previousEnv;
                        return result;
                    }
                    // Default shallow clone
                    auto cloned = std::make_shared<ClassInstance>();
                    cloned->className = instance->className;
                    cloned->parentClassName = instance->parentClassName;
                    cloned->fields = instance->fields;
                    return Value(cloned);
                }

                throw std::runtime_error("Method '" + getExpr->name + "' not found in class '" + instance->className + "'.");
            }

            // Method calls on primitives: "hello".upper(), [1,2,3].length(), etc.
            std::vector<Value> primArgs;
            for (const auto& argExpr : callExpr->arguments) {
                primArgs.push_back(evalExpr(argExpr.get()));
            }
            const std::string& method = getExpr->name;

            // String methods
            if (object.holds_alternative<std::string>()) {
                const auto& str = object.get<std::string>();
                if (method == "length") return Value(static_cast<int>(str.size()));
                if (method == "upper") {
                    std::string r = str;
                    std::transform(r.begin(), r.end(), r.begin(), ::toupper);
                    return Value(r);
                }
                if (method == "lower") {
                    std::string r = str;
                    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
                    return Value(r);
                }
                if (method == "trim") {
                    std::string r = str;
                    size_t s = r.find_first_not_of(" \t\n\r");
                    size_t e = r.find_last_not_of(" \t\n\r");
                    if (s == std::string::npos) return Value(std::string(""));
                    return Value(r.substr(s, e - s + 1));
                }
                if (method == "split") {
                    std::string delim = primArgs.empty() ? " " : valueToString(primArgs[0]);
                    std::vector<Value> parts;
                    size_t pos = 0;
                    while (true) {
                        size_t found = str.find(delim, pos);
                        if (found == std::string::npos) {
                            parts.push_back(Value(str.substr(pos)));
                            break;
                        }
                        parts.push_back(Value(str.substr(pos, found - pos)));
                        pos = found + delim.size();
                    }
                    return Value(parts);
                }
                if (method == "replace") {
                    if (primArgs.size() < 2) throw std::runtime_error("replace() requires 2 arguments.");
                    std::string from = valueToString(primArgs[0]);
                    std::string to = valueToString(primArgs[1]);
                    std::string r = str;
                    size_t pos = 0;
                    while ((pos = r.find(from, pos)) != std::string::npos) {
                        r.replace(pos, from.length(), to);
                        pos += to.length();
                    }
                    return Value(r);
                }
                if (method == "contains") {
                    std::string sub = valueToString(primArgs[0]);
                    return Value(str.find(sub) != std::string::npos);
                }
                if (method == "starts_with") {
                    std::string prefix = valueToString(primArgs[0]);
                    return Value(str.substr(0, prefix.size()) == prefix);
                }
                if (method == "ends_with") {
                    std::string suffix = valueToString(primArgs[0]);
                    if (suffix.size() > str.size()) return Value(false);
                    return Value(str.substr(str.size() - suffix.size()) == suffix);
                }
                if (method == "reverse") {
                    std::string r(str.rbegin(), str.rend());
                    return Value(r);
                }
                if (method == "chars") {
                    std::vector<Value> chars;
                    for (char c : str) chars.push_back(Value(std::string(1, c)));
                    return Value(chars);
                }
                // Check extension methods
                auto extIt = extensionMethods.find("String." + method);
                if (extIt != extensionMethods.end()) {
                    std::vector<Value> extArgs = {Value(str)};
                    extArgs.insert(extArgs.end(), primArgs.begin(), primArgs.end());
                    return call(Value(extIt->second), extArgs);
                }
                throw std::runtime_error("Unknown string method: " + method);
            }

            // List methods
            if (object.holds_alternative<std::vector<Value>>()) {
                auto list = object.get<std::vector<Value>>();
                if (method == "length") return Value(static_cast<int>(list.size()));
                if (method == "push") {
                    list.push_back(primArgs[0]);
                    return Value(list);
                }
                if (method == "pop") {
                    if (list.empty()) throw std::runtime_error("pop() on empty list.");
                    Value last = list.back();
                    list.pop_back();
                    return last;
                }
                if (method == "reverse") {
                    std::reverse(list.begin(), list.end());
                    return Value(list);
                }
                if (method == "sort") {
                    std::sort(list.begin(), list.end());
                    return Value(list);
                }
                if (method == "contains") {
                    for (const auto& item : list) {
                        if (item == primArgs[0]) return Value(true);
                    }
                    return Value(false);
                }
                if (method == "join") {
                    std::string delim = primArgs.empty() ? "" : valueToString(primArgs[0]);
                    std::string r;
                    for (size_t i = 0; i < list.size(); ++i) {
                        if (i > 0) r += delim;
                        r += valueToString(list[i]);
                    }
                    return Value(r);
                }
                throw std::runtime_error("Unknown list method: " + method);
            }

            // Number methods
            if (object.holds_alternative<int>()) {
                int n = object.get<int>();
                if (method == "abs") return Value(std::abs(n));
                throw std::runtime_error("Unknown int method: " + method);
            }
            if (object.holds_alternative<double>()) {
                double n = object.get<double>();
                if (method == "abs") return Value(std::abs(n));
                if (method == "floor") return Value(static_cast<int>(std::floor(n)));
                if (method == "ceil") return Value(static_cast<int>(std::ceil(n)));
                if (method == "round") return Value(static_cast<int>(std::round(n)));
                throw std::runtime_error("Unknown float method: " + method);
            }
        }

        // Built-in higher-order functions: map, filter, reduce
        if (auto varExpr = dynamic_cast<const VariableExpr*>(callExpr->callee.get())) {
            if (varExpr->name == "map" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("map() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                std::vector<Value> result;
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    result.push_back(call(funcVal, callArgs));
                }
                return Value(result);
            }
            if (varExpr->name == "filter" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("filter() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                std::vector<Value> result;
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    Value pred = call(funcVal, callArgs);
                    if (isTruthy(pred)) result.push_back(item);
                }
                return Value(result);
            }
            if (varExpr->name == "reduce" && callExpr->arguments.size() == 3) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value accum = evalExpr(callExpr->arguments[1].get());
                Value funcVal = evalExpr(callExpr->arguments[2].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("reduce() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {accum, item};
                    accum = call(funcVal, callArgs);
                }
                return accum;
            }
            if (varExpr->name == "foreach" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("foreach() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    call(funcVal, callArgs);
                }
                return Value();
            }
            // ---- sort_by(list, fn) ----
            if (varExpr->name == "sort_by" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("sort_by() first argument must be a list.");
                auto vec = listVal.get<std::vector<Value>>();
                std::sort(vec.begin(), vec.end(), [&](const Value& a, const Value& b) {
                    std::vector<Value> argsA = {a};
                    std::vector<Value> argsB = {b};
                    Value keyA = call(funcVal, argsA);
                    Value keyB = call(funcVal, argsB);
                    return keyA < keyB;
                });
                return Value(vec);
            }
            // ---- find(list, fn) ----
            if (varExpr->name == "find" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("find() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    Value pred = call(funcVal, callArgs);
                    if (isTruthy(pred)) return item;
                }
                return Value();  // None/null if not found
            }
            // ---- any(list, fn) ----
            if (varExpr->name == "any" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("any() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    if (isTruthy(call(funcVal, callArgs))) return Value(true);
                }
                return Value(false);
            }
            // ---- all(list, fn) ----
            if (varExpr->name == "all" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("all() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    if (!isTruthy(call(funcVal, callArgs))) return Value(false);
                }
                return Value(true);
            }
            // ---- flat_map(list, fn) ----
            if (varExpr->name == "flat_map" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("flat_map() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                std::vector<Value> result;
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    Value mapped = call(funcVal, callArgs);
                    if (mapped.holds_alternative<std::vector<Value>>()) {
                        const auto& inner = mapped.get<std::vector<Value>>();
                        result.insert(result.end(), inner.begin(), inner.end());
                    } else {
                        result.push_back(mapped);
                    }
                }
                return Value(result);
            }
            // ---- zip(list1, list2) ----
            if (varExpr->name == "zip" && callExpr->arguments.size() == 2) {
                Value listVal1 = evalExpr(callExpr->arguments[0].get());
                Value listVal2 = evalExpr(callExpr->arguments[1].get());
                if (!listVal1.holds_alternative<std::vector<Value>>() || !listVal2.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("zip() arguments must be lists.");
                const auto& l1 = listVal1.get<std::vector<Value>>();
                const auto& l2 = listVal2.get<std::vector<Value>>();
                size_t minLen = std::min(l1.size(), l2.size());
                std::vector<Value> result;
                for (size_t i = 0; i < minLen; ++i) {
                    std::vector<Value> pair = {l1[i], l2[i]};
                    result.push_back(Value(pair));
                }
                return Value(result);
            }
            // ---- enumerate(list) ----
            if (varExpr->name == "enumerate" && callExpr->arguments.size() == 1) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("enumerate() argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                std::vector<Value> result;
                for (size_t i = 0; i < list.size(); ++i) {
                    std::vector<Value> pair = {Value(static_cast<int>(i)), list[i]};
                    result.push_back(Value(pair));
                }
                return Value(result);
            }
            // ---- take(list, n) ----
            if (varExpr->name == "take" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value nVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("take() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                int n = 0;
                if (nVal.holds_alternative<int>()) n = nVal.get<int>();
                else if (nVal.holds_alternative<double>()) n = static_cast<int>(nVal.get<double>());
                if (n < 0) n = 0;
                if (n > static_cast<int>(list.size())) n = static_cast<int>(list.size());
                return Value(std::vector<Value>(list.begin(), list.begin() + n));
            }
            // ---- drop(list, n) ----
            if (varExpr->name == "drop" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value nVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("drop() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                int n = 0;
                if (nVal.holds_alternative<int>()) n = nVal.get<int>();
                else if (nVal.holds_alternative<double>()) n = static_cast<int>(nVal.get<double>());
                if (n < 0) n = 0;
                if (n > static_cast<int>(list.size())) n = static_cast<int>(list.size());
                return Value(std::vector<Value>(list.begin() + n, list.end()));
            }
            // ---- map_filter(list, fn) - map + filter: fn returns null to skip ----
            if (varExpr->name == "map_filter" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("map_filter() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                std::vector<Value> result;
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    Value mapped = call(funcVal, callArgs);
                    if (!mapped.holds_alternative<std::monostate>()) {
                        result.push_back(mapped);
                    }
                }
                return Value(result);
            }
            // ---- group_by(list, fn) - groups into map by key ----
            if (varExpr->name == "group_by" && callExpr->arguments.size() == 2) {
                Value listVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!listVal.holds_alternative<std::vector<Value>>())
                    throw std::runtime_error("group_by() first argument must be a list.");
                const auto& list = listVal.get<std::vector<Value>>();
                std::unordered_map<std::string, Value> result;
                for (const auto& item : list) {
                    std::vector<Value> callArgs = {item};
                    Value key = call(funcVal, callArgs);
                    std::string keyStr = valueToString(key);
                    if (result.find(keyStr) == result.end()) {
                        result[keyStr] = std::vector<Value>();
                    }
                    auto& group = const_cast<std::vector<Value>&>(result[keyStr].get<std::vector<Value>>());
                    group.push_back(item);
                }
                return Value(result);
            }
            // ---- map_map_values(map, fn) - transform map values ----
            if (varExpr->name == "map_map_values" && callExpr->arguments.size() == 2) {
                Value mapVal = evalExpr(callExpr->arguments[0].get());
                Value funcVal = evalExpr(callExpr->arguments[1].get());
                if (!mapVal.holds_alternative<std::unordered_map<std::string, Value>>())
                    throw std::runtime_error("map_map_values() first argument must be a map.");
                const auto& map = mapVal.get<std::unordered_map<std::string, Value>>();
                std::unordered_map<std::string, Value> result;
                for (const auto& [key, val] : map) {
                    std::vector<Value> callArgs = {val};
                    result[key] = call(funcVal, callArgs);
                }
                return Value(result);
            }
        }

        // Handle super.method() calls with proper 'this' binding
        if (auto superExpr = dynamic_cast<const SuperExpr*>(callExpr->callee.get())) {
            Value thisVal = environment->get("this");
            if (!thisVal.holds_alternative<std::shared_ptr<ClassInstance>>()) {
                throw std::runtime_error("'super' used outside of class context.");
            }
            auto instance = thisVal.get<std::shared_ptr<ClassInstance>>();

            // Find parent class
            std::string parentClass;
            auto classIt2 = classes.find(currentClassName.empty() ? instance->className : currentClassName);
            if (classIt2 != classes.end()) {
                parentClass = classIt2->second->parentName;
            }
            if (parentClass.empty()) {
                parentClass = instance->parentClassName;
            }
            if (parentClass.empty()) {
                throw std::runtime_error("'super' used in class with no parent.");
            }

            // Walk parent chain to find method
            const FunctionStmt* superMethod = nullptr;
            std::string searchClass = parentClass;
            while (!searchClass.empty()) {
                auto funcIt = functions.find(searchClass + "." + superExpr->methodName);
                if (funcIt != functions.end()) {
                    superMethod = funcIt->second;
                    break;
                }
                auto cIt = classes.find(searchClass);
                if (cIt != classes.end() && !cIt->second->parentName.empty()) {
                    searchClass = cIt->second->parentName;
                } else {
                    break;
                }
            }

            if (!superMethod) {
                throw std::runtime_error("Method '" + superExpr->methodName + "' not found in parent class '" + parentClass + "'.");
            }

            std::vector<Value> arguments;
            for (const auto& argExpr : callExpr->arguments) {
                arguments.push_back(evalExpr(argExpr.get()));
            }

            // Fill defaults
            if (arguments.size() < superMethod->parameters.size()) {
                for (size_t i = arguments.size(); i < superMethod->parameters.size(); ++i) {
                    if (i < superMethod->parameterDefaults.size() && superMethod->parameterDefaults[i]) {
                        arguments.push_back(evalExpr(superMethod->parameterDefaults[i].get()));
                    }
                }
            }

            auto previousEnv = environment;
            environment = std::make_shared<Environment>();
            environment->define("this", Value(instance));
            auto savedClassName = currentClassName;
            currentClassName = parentClass;

            for (size_t i = 0; i < superMethod->parameters.size() && i < arguments.size(); ++i) {
                environment->define(superMethod->parameters[i], arguments[i]);
            }

            Value result;
            try {
                execute(superMethod->body.get());
                result = Value();
            } catch (const Value& returnValue) {
                result = returnValue;
            }

            currentClassName = savedClassName;
            environment = previousEnv;
            return result;
        }

        // Normal function call
        Value callee = evalExpr(callExpr->callee.get());
        std::vector<Value> arguments;

        // Track variable references for native function write-back (pass-by-ref semantics)
        std::vector<std::string> argVarNames;
        for (const auto& argExpr : callExpr->arguments) {
            // Handle spread in function call arguments
            if (auto spread = dynamic_cast<const SpreadExpr*>(argExpr.get())) {
                Value spreadVal = evalExpr(spread->expression.get());
                if (spreadVal.holds_alternative<std::vector<Value>>()) {
                    const auto& inner = spreadVal.get<std::vector<Value>>();
                    for (const auto& item : inner) {
                        arguments.push_back(item);
                        argVarNames.push_back("");
                    }
                } else {
                    throw std::runtime_error("Spread operator requires a list.");
                }
            } else {
                arguments.push_back(evalExpr(argExpr.get()));
                if (auto varRef = dynamic_cast<const VariableExpr*>(argExpr.get())) {
                    argVarNames.push_back(varRef->name);
                } else {
                    argVarNames.push_back("");
                }
            }
        }

        // Named arguments: reorder args to match parameter positions
        if (!callExpr->argumentNames.empty()) {
            const FunctionStmt* targetFunc = nullptr;
            if (callee.holds_alternative<const FunctionStmt*>()) {
                targetFunc = callee.get<const FunctionStmt*>();
            }
            if (targetFunc && targetFunc->parameters.size() > 0) {
                std::vector<Value> reordered(targetFunc->parameters.size(), Value());
                std::vector<bool> filled(targetFunc->parameters.size(), false);
                // First pass: place positional args
                size_t posIdx = 0;
                for (size_t i = 0; i < callExpr->argumentNames.size(); ++i) {
                    if (callExpr->argumentNames[i].empty()) {
                        if (posIdx < reordered.size()) {
                            reordered[posIdx] = arguments[i];
                            filled[posIdx] = true;
                        }
                        posIdx++;
                    }
                }
                // Second pass: place named args
                for (size_t i = 0; i < callExpr->argumentNames.size(); ++i) {
                    if (!callExpr->argumentNames[i].empty()) {
                        for (size_t p = 0; p < targetFunc->parameters.size(); ++p) {
                            if (targetFunc->parameters[p] == callExpr->argumentNames[i]) {
                                reordered[p] = arguments[i];
                                filled[p] = true;
                                break;
                            }
                        }
                    }
                }
                // Fill unfilled with defaults
                for (size_t i = 0; i < reordered.size(); ++i) {
                    if (!filled[i] && i < targetFunc->parameterDefaults.size() && targetFunc->parameterDefaults[i]) {
                        reordered[i] = evalExpr(targetFunc->parameterDefaults[i].get());
                        filled[i] = true;
                    }
                }
                arguments = std::move(reordered);
            }
        }

        Value result = call(callee, arguments);

        // Write-back: if callee was a native function, update mutable variables
        if (callee.holds_alternative<NativeFunction>()) {
            for (size_t i = 0; i < argVarNames.size(); ++i) {
                if (!argVarNames[i].empty() && immutableVars.find(argVarNames[i]) == immutableVars.end()) {
                    if (environment && environment->values.count(argVarNames[i])) {
                        environment->values[argVarNames[i]] = arguments[i];
                    } else if (variables.count(argVarNames[i])) {
                        variables[argVarNames[i]] = arguments[i];
                    }
                }
            }
        }

        return result;
    }

    // ---- ChainedComparisonExpr ----
    if (auto chain = dynamic_cast<const ChainedComparisonExpr*>(expr)) {
        // Evaluate all operands once
        std::vector<Value> vals;
        for (const auto& operand : chain->operands) {
            vals.push_back(evalExpr(operand.get()));
        }
        // Check each adjacent pair: vals[i] op[i] vals[i+1]
        for (size_t i = 0; i < chain->operators.size(); ++i) {
            Value left = vals[i];
            Value right = vals[i + 1];
            bool result = false;
            auto cmpOp = chain->operators[i];
            // Handle int/double comparisons
            auto toDouble = [](const Value& v) -> double {
                if (v.holds_alternative<int>()) return static_cast<double>(v.get<int>());
                if (v.holds_alternative<double>()) return v.get<double>();
                throw std::runtime_error("Chained comparison requires numeric operands.");
            };
            double l = toDouble(left), r = toDouble(right);
            switch (cmpOp) {
                case BinaryOp::Less:         result = l < r; break;
                case BinaryOp::LessEqual:    result = l <= r; break;
                case BinaryOp::Greater:      result = l > r; break;
                case BinaryOp::GreaterEqual: result = l >= r; break;
                default: result = false; break;
            }
            if (!result) return Value(false);
        }
        return Value(true);
    }

    // ---- ComposeExpr: f >>> g → creates a ClassInstance marker for composition ----
    if (auto comp = dynamic_cast<const ComposeExpr*>(expr)) {
        Value leftFunc = evalExpr(comp->left.get());
        Value rightFunc = evalExpr(comp->right.get());
        // Store as a special ClassInstance with className "__compose__"
        auto composed = std::make_shared<ClassInstance>();
        composed->className = "__compose__";
        composed->fields["__left"] = leftFunc;
        composed->fields["__right"] = rightFunc;
        return Value(composed);
    }

    // ---- WalrusExpr: let x := expr → assign and return value ----
    if (auto walrus = dynamic_cast<const WalrusExpr*>(expr)) {
        Value val = evalExpr(walrus->expression.get());
        if (environment) {
            environment->define(walrus->name, val);
        } else {
            variables[walrus->name] = val;
        }
        return val;
    }

    // ---- ListComprehensionExpr ----
    if (auto listComp = dynamic_cast<const ListComprehensionExpr*>(expr)) {
        Value iterableVal = evalExpr(listComp->iterable.get());
        std::vector<Value> result;

        auto iterate = [&](const std::vector<Value>& items) {
            for (const auto& item : items) {
                auto savedVars = variables;
                variables[listComp->varName] = item;
                if (listComp->condition) {
                    Value condVal = evalExpr(listComp->condition.get());
                    if (!isTruthy(condVal)) {
                        variables = savedVars;
                        continue;
                    }
                }
                result.push_back(evalExpr(listComp->body.get()));
                variables = savedVars;
            }
        };

        if (iterableVal.holds_alternative<std::vector<Value>>()) {
            iterate(iterableVal.get<std::vector<Value>>());
        } else {
            throw std::runtime_error("List comprehension requires an iterable.");
        }
        return Value(result);
    }

    // ---- MapComprehensionExpr ----
    if (auto mapComp = dynamic_cast<const MapComprehensionExpr*>(expr)) {
        Value iterableVal = evalExpr(mapComp->iterable.get());
        std::unordered_map<std::string, Value> result;

        if (iterableVal.holds_alternative<std::vector<Value>>()) {
            const auto& items = iterableVal.get<std::vector<Value>>();
            for (const auto& item : items) {
                auto savedVars = variables;
                variables[mapComp->varName] = item;
                if (mapComp->condition) {
                    Value condVal = evalExpr(mapComp->condition.get());
                    if (!isTruthy(condVal)) {
                        variables = savedVars;
                        continue;
                    }
                }
                std::string key = valueToString(evalExpr(mapComp->keyExpr.get()));
                Value val = evalExpr(mapComp->valueExpr.get());
                result[key] = val;
                variables = savedVars;
            }
        } else {
            throw std::runtime_error("Map comprehension requires an iterable.");
        }
        return Value(result);
    }

    // ---- BinaryExpr ----
    if (auto bin = dynamic_cast<const BinaryExpr*>(expr)) {
        Value left = evalExpr(bin->left.get());
        Value right = evalExpr(bin->right.get());

        // Operator overloading: check for dunder methods on ClassInstance
        if (left.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = left.get<std::shared_ptr<ClassInstance>>();
            std::string dunder;
            switch (bin->op) {
                case BinaryOp::Add: dunder = "__add"; break;
                case BinaryOp::Sub: dunder = "__sub"; break;
                case BinaryOp::Mul: dunder = "__mul"; break;
                case BinaryOp::Div: dunder = "__div"; break;
                case BinaryOp::Mod: dunder = "__mod"; break;
                case BinaryOp::Equal: dunder = "__eq"; break;
                case BinaryOp::Less: dunder = "__lt"; break;
                case BinaryOp::Greater: dunder = "__gt"; break;
                default: break;
            }
            if (!dunder.empty()) {
                // Walk inheritance chain
                std::string cn = instance->className;
                while (!cn.empty()) {
                    auto funcIt = functions.find(cn + "." + dunder);
                    if (funcIt != functions.end() && funcIt->second->body) {
                        auto previousEnv = environment;
                        environment = std::make_shared<Environment>();
                        environment->define("this", Value(instance));
                        environment->define(funcIt->second->parameters[0], right);
                        auto savedClassName = currentClassName;
                        currentClassName = instance->className;
                        Value result;
                        try {
                            execute(funcIt->second->body.get());
                            result = Value();
                        } catch (const Value& returnValue) {
                            result = returnValue;
                        }
                        currentClassName = savedClassName;
                        environment = previousEnv;
                        return result;
                    }
                    auto classIt = classes.find(cn);
                    if (classIt != classes.end() && !classIt->second->parentName.empty()) {
                        cn = classIt->second->parentName;
                    } else {
                        break;
                    }
                }
            }

            // Data class auto-equality
            if (bin->op == BinaryOp::Equal && right.holds_alternative<std::shared_ptr<ClassInstance>>()) {
                auto rightInst = right.get<std::shared_ptr<ClassInstance>>();
                if (instance->className == rightInst->className) {
                    auto classDefIt = classes.find(instance->className);
                    if (classDefIt != classes.end() && classDefIt->second->isDataClass) {
                        for (const auto& field : classDefIt->second->fields) {
                            auto lIt = instance->fields.find(field);
                            auto rIt = rightInst->fields.find(field);
                            if (lIt == instance->fields.end() || rIt == rightInst->fields.end()) return Value(false);
                            if (!(lIt->second == rIt->second)) return Value(false);
                        }
                        return Value(true);
                    }
                }
            }

            // Fallback: __cmp protocol for comparison operators
            if (bin->op == BinaryOp::Less || bin->op == BinaryOp::Greater ||
                bin->op == BinaryOp::LessEqual || bin->op == BinaryOp::GreaterEqual ||
                bin->op == BinaryOp::Equal) {
                std::string cn = instance->className;
                while (!cn.empty()) {
                    auto cmpIt = functions.find(cn + ".__cmp");
                    if (cmpIt != functions.end() && cmpIt->second->body) {
                        auto previousEnv = environment;
                        environment = std::make_shared<Environment>();
                        environment->define("this", Value(instance));
                        environment->define(cmpIt->second->parameters[0], right);
                        Value result;
                        try {
                            execute(cmpIt->second->body.get());
                            result = Value();
                        } catch (const Value& returnValue) {
                            result = returnValue;
                        }
                        environment = previousEnv;
                        int cmpVal = 0;
                        if (result.holds_alternative<int>()) cmpVal = result.get<int>();
                        else if (result.holds_alternative<double>()) cmpVal = static_cast<int>(result.get<double>());
                        switch (bin->op) {
                            case BinaryOp::Less: return Value(cmpVal < 0);
                            case BinaryOp::Greater: return Value(cmpVal > 0);
                            case BinaryOp::LessEqual: return Value(cmpVal <= 0);
                            case BinaryOp::GreaterEqual: return Value(cmpVal >= 0);
                            case BinaryOp::Equal: return Value(cmpVal == 0);
                            default: break;
                        }
                    }
                    auto classIt = classes.find(cn);
                    if (classIt != classes.end() && !classIt->second->parentName.empty()) {
                        cn = classIt->second->parentName;
                    } else {
                        break;
                    }
                }
            }
        }

        switch (bin->op) {
            case BinaryOp::Add:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l + r); },
                    [](float l, float r) { return Value(l + r); },
                    [](double l, double r) { return Value(l + r); },
                    [](int l, float r) { return Value(static_cast<float>(l) + r); },
                    [](float l, int r) { return Value(l + static_cast<float>(r)); },
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

                    // List concatenation
                    [](const std::vector<Value>& l, const std::vector<Value>& r) {
                        std::vector<Value> result = l;
                        result.insert(result.end(), r.begin(), r.end());
                        return Value(result);
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
                    [](int l, float r) { return Value(static_cast<float>(l) - r); },
                    [](float l, int r) { return Value(l - static_cast<float>(r)); },
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
                    [](int l, float r) { return Value(static_cast<float>(l) * r); },
                    [](float l, int r) { return Value(l * static_cast<float>(r)); },
                    [](int l, double r) { return Value(static_cast<double>(l) * r); },
                    [](double l, int r) { return Value(l * static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) * r); },
                    [](double l, float r) { return Value(l * static_cast<double>(r)); },
                    // String multiplication: "abc" * 3 or 3 * "abc"
                    [](const std::string& s, int n) -> Value {
                        if (n <= 0) return Value(std::string(""));
                        std::string result;
                        result.reserve(s.size() * n);
                        for (int i = 0; i < n; ++i) result += s;
                        return Value(result);
                    },
                    [](int n, const std::string& s) -> Value {
                        if (n <= 0) return Value(std::string(""));
                        std::string result;
                        result.reserve(s.size() * n);
                        for (int i = 0; i < n; ++i) result += s;
                        return Value(result);
                    },
                    [](auto, auto) -> Value {
                        throw std::runtime_error("Incompatible types for * operator.");
                    }
                }, left.data, right.data);

            // ---- BUG FIX: int / int returns int (integer division) ----
            case BinaryOp::Div:
                return std::visit(overloaded {
                    [](int l, int r) -> Value {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(l / r);  // Integer division
                    },
                    [](float l, float r) -> Value {
                        if (r == 0.0f) throw std::runtime_error("Division by zero.");
                        return Value(l / r);
                    },
                    [](double l, double r) -> Value {
                        if (r == 0.0) throw std::runtime_error("Division by zero.");
                        return Value(l / r);
                    },
                    [](int l, float r) -> Value {
                        if (r == 0.0f) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<float>(l) / r);
                    },
                    [](float l, int r) -> Value {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(l / static_cast<float>(r));
                    },
                    [](int l, double r) -> Value {
                        if (r == 0.0) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<double>(l) / r);
                    },
                    [](double l, int r) -> Value {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(l / static_cast<double>(r));
                    },
                    [](float l, double r) -> Value {
                        if (r == 0.0) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<double>(l) / r);
                    },
                    [](double l, float r) -> Value {
                        if (r == 0.0f) throw std::runtime_error("Division by zero.");
                        return Value(l / static_cast<double>(r));
                    },
                    [](auto, auto) -> Value {
                        throw std::runtime_error("Incompatible types for / operator.");
                    }
                }, left.data, right.data);

            // ---- NEW: Modulo operator ----
            case BinaryOp::Mod:
                return std::visit(overloaded {
                    [](int l, int r) -> Value {
                        if (r == 0) throw std::runtime_error("Modulo by zero.");
                        return Value(l % r);
                    },
                    [](double l, double r) -> Value {
                        if (r == 0.0) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(l, r));
                    },
                    [](float l, float r) -> Value {
                        if (r == 0.0f) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(static_cast<double>(l), static_cast<double>(r)));
                    },
                    [](int l, double r) -> Value {
                        if (r == 0.0) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(static_cast<double>(l), r));
                    },
                    [](double l, int r) -> Value {
                        if (r == 0) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(l, static_cast<double>(r)));
                    },
                    [](int l, float r) -> Value {
                        if (r == 0.0f) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(static_cast<double>(l), static_cast<double>(r)));
                    },
                    [](float l, int r) -> Value {
                        if (r == 0) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(static_cast<double>(l), static_cast<double>(r)));
                    },
                    [](float l, double r) -> Value {
                        if (r == 0.0) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(static_cast<double>(l), r));
                    },
                    [](double l, float r) -> Value {
                        if (r == 0.0f) throw std::runtime_error("Modulo by zero.");
                        return Value(std::fmod(l, static_cast<double>(r)));
                    },
                    [](auto, auto) -> Value {
                        throw std::runtime_error("Incompatible types for % operator.");
                    }
                }, left.data, right.data);

            // ---- NEW: Exponentiation (always returns double) ----
            case BinaryOp::Pow: {
                double base = 0.0, exp = 0.0;
                if (left.holds_alternative<int>()) base = static_cast<double>(left.get<int>());
                else if (left.holds_alternative<double>()) base = left.get<double>();
                else if (left.holds_alternative<float>()) base = static_cast<double>(left.get<float>());
                else throw std::runtime_error("Incompatible types for ** operator.");

                if (right.holds_alternative<int>()) exp = static_cast<double>(right.get<int>());
                else if (right.holds_alternative<double>()) exp = right.get<double>();
                else if (right.holds_alternative<float>()) exp = static_cast<double>(right.get<float>());
                else throw std::runtime_error("Incompatible types for ** operator.");

                return Value(std::pow(base, exp));
            }

            case BinaryOp::Equal: return Value(left == right);
            case BinaryOp::NotEqual: return Value(left != right);

            case BinaryOp::Less:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l < r); },
                    [](float l, float r) { return Value(l < r); },
                    [](double l, double r) { return Value(l < r); },
                    [](int l, float r) { return Value(static_cast<float>(l) < r); },
                    [](float l, int r) { return Value(l < static_cast<float>(r)); },
                    [](int l, double r) { return Value(static_cast<double>(l) < r); },
                    [](double l, int r) { return Value(l < static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) < r); },
                    [](double l, float r) { return Value(l < static_cast<double>(r)); },
                    [](const std::string& l, const std::string& r) { return Value(l < r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for < operator."); }
                }, left.data, right.data);

            case BinaryOp::Greater:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l > r); },
                    [](float l, float r) { return Value(l > r); },
                    [](double l, double r) { return Value(l > r); },
                    [](int l, float r) { return Value(static_cast<float>(l) > r); },
                    [](float l, int r) { return Value(l > static_cast<float>(r)); },
                    [](int l, double r) { return Value(static_cast<double>(l) > r); },
                    [](double l, int r) { return Value(l > static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) > r); },
                    [](double l, float r) { return Value(l > static_cast<double>(r)); },
                    [](const std::string& l, const std::string& r) { return Value(l > r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for > operator."); }
                }, left.data, right.data);

            case BinaryOp::GreaterEqual:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l >= r); },
                    [](float l, float r) { return Value(l >= r); },
                    [](double l, double r) { return Value(l >= r); },
                    [](int l, float r) { return Value(static_cast<float>(l) >= r); },
                    [](float l, int r) { return Value(l >= static_cast<float>(r)); },
                    [](int l, double r) { return Value(static_cast<double>(l) >= r); },
                    [](double l, int r) { return Value(l >= static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) >= r); },
                    [](double l, float r) { return Value(l >= static_cast<double>(r)); },
                    [](const std::string& l, const std::string& r) { return Value(l >= r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for >= operator."); }
                }, left.data, right.data);

            case BinaryOp::LessEqual:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l <= r); },
                    [](float l, float r) { return Value(l <= r); },
                    [](double l, double r) { return Value(l <= r); },
                    [](int l, float r) { return Value(static_cast<float>(l) <= r); },
                    [](float l, int r) { return Value(l <= static_cast<float>(r)); },
                    [](int l, double r) { return Value(static_cast<double>(l) <= r); },
                    [](double l, int r) { return Value(l <= static_cast<double>(r)); },
                    [](float l, double r) { return Value(static_cast<double>(l) <= r); },
                    [](double l, float r) { return Value(l <= static_cast<double>(r)); },
                    [](const std::string& l, const std::string& r) { return Value(l <= r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for <= operator."); }
                }, left.data, right.data);

            case BinaryOp::And:
                return std::visit(overloaded {
                    [](bool l, bool r) { return Value(l && r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Invalid types for operator '&&'."); }
                }, left.data, right.data);

            case BinaryOp::Or:
                return std::visit(overloaded {
                    [](bool l, bool r) { return Value(l || r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Invalid types for operator '||'."); }
                }, left.data, right.data);

            // ---- NEW: Bitwise AND (int only) ----
            case BinaryOp::BitAnd: {
                if (left.holds_alternative<int>() && right.holds_alternative<int>()) {
                    return Value(left.get<int>() & right.get<int>());
                }
                throw std::runtime_error("Bitwise AND requires integer operands.");
            }

            // ---- NEW: Bitwise OR (int only) ----
            case BinaryOp::BitOr: {
                if (left.holds_alternative<int>() && right.holds_alternative<int>()) {
                    return Value(left.get<int>() | right.get<int>());
                }
                throw std::runtime_error("Bitwise OR requires integer operands.");
            }

            // ---- NEW: Bitwise XOR (int only) ----
            case BinaryOp::BitXor: {
                if (left.holds_alternative<int>() && right.holds_alternative<int>()) {
                    return Value(left.get<int>() ^ right.get<int>());
                }
                throw std::runtime_error("Bitwise XOR requires integer operands.");
            }

            // ---- NEW: Left shift (int only) ----
            case BinaryOp::Shl: {
                if (left.holds_alternative<int>() && right.holds_alternative<int>()) {
                    return Value(left.get<int>() << right.get<int>());
                }
                throw std::runtime_error("Left shift requires integer operands.");
            }

            // ---- NEW: Right shift (int only) ----
            case BinaryOp::Shr: {
                if (left.holds_alternative<int>() && right.holds_alternative<int>()) {
                    return Value(left.get<int>() >> right.get<int>());
                }
                throw std::runtime_error("Right shift requires integer operands.");
            }

            // ---- NEW: In operator (membership) ----
            case BinaryOp::In: {
                // x in list → check if x is in the list
                if (right.holds_alternative<std::vector<Value>>()) {
                    const auto& list = right.get<std::vector<Value>>();
                    for (const auto& item : list) {
                        if (left == item) return Value(true);
                    }
                    return Value(false);
                }
                // key in map → check if key exists
                if (right.holds_alternative<std::unordered_map<std::string, Value>>()) {
                    const auto& map = right.get<std::unordered_map<std::string, Value>>();
                    std::string key;
                    if (left.holds_alternative<std::string>()) {
                        key = left.get<std::string>();
                    } else {
                        key = valueToString(left);
                    }
                    return Value(map.find(key) != map.end());
                }
                // substr in string → substring check
                if (right.holds_alternative<std::string>() && left.holds_alternative<std::string>()) {
                    return Value(right.get<std::string>().find(left.get<std::string>()) != std::string::npos);
                }
                throw std::runtime_error("'in' operator requires a list, map, or string on the right side.");
            }
            case BinaryOp::NotIn: {
                // x not in list → !(x in list)
                if (right.holds_alternative<std::vector<Value>>()) {
                    const auto& list = right.get<std::vector<Value>>();
                    for (const auto& item : list) {
                        if (left == item) return Value(false);
                    }
                    return Value(true);
                }
                if (right.holds_alternative<std::unordered_map<std::string, Value>>()) {
                    const auto& map = right.get<std::unordered_map<std::string, Value>>();
                    std::string key = left.holds_alternative<std::string>() ? left.get<std::string>() : valueToString(left);
                    return Value(map.find(key) == map.end());
                }
                if (right.holds_alternative<std::string>() && left.holds_alternative<std::string>()) {
                    return Value(right.get<std::string>().find(left.get<std::string>()) == std::string::npos);
                }
                throw std::runtime_error("'not in' operator requires a list, map, or string on the right side.");
            }
        }
    }

    // ---- UnaryExpr ----
    if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        Value operand = evalExpr(unary->right.get());

        // Operator overloading for unary neg on ClassInstance
        if (unary->op == UnaryOp::Neg && operand.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = operand.get<std::shared_ptr<ClassInstance>>();
            std::string cn = instance->className;
            while (!cn.empty()) {
                auto funcIt = functions.find(cn + ".__neg");
                if (funcIt != functions.end() && funcIt->second->body) {
                    auto previousEnv = environment;
                    environment = std::make_shared<Environment>();
                    environment->define("this", Value(instance));
                    Value result;
                    try {
                        execute(funcIt->second->body.get());
                        result = Value();
                    } catch (const Value& returnValue) {
                        result = returnValue;
                    }
                    environment = previousEnv;
                    return result;
                }
                auto classIt = classes.find(cn);
                if (classIt != classes.end() && !classIt->second->parentName.empty()) {
                    cn = classIt->second->parentName;
                } else {
                    break;
                }
            }
        }

        switch (unary->op) {
            case UnaryOp::Not:
                return std::visit(overloaded {
                    [](bool b) { return Value(!b); },
                    [](auto) -> Value { throw std::runtime_error("Operator '!' requires boolean."); }
                }, operand.data);

            // ---- NEW: Unary negation ----
            case UnaryOp::Neg:
                return std::visit(overloaded {
                    [](int v) { return Value(-v); },
                    [](double v) { return Value(-v); },
                    [](float v) { return Value(-v); },
                    [](auto) -> Value { throw std::runtime_error("Operator '-' requires numeric type."); }
                }, operand.data);

            // ---- NEW: Bitwise NOT (int only) ----
            case UnaryOp::BitNot: {
                if (operand.holds_alternative<int>()) {
                    return Value(~operand.get<int>());
                }
                throw std::runtime_error("Bitwise NOT requires integer operand.");
            }
        }
    }

    throw std::runtime_error("Invalid expression.");
}

// ============================================================================
// Module registration
// ============================================================================
void Interpreter::register_module(const std::string& name, std::shared_ptr<Environment> env) {
    modules[name] = env;

    // Expose module functions globally with prefix (e.g., math.sqrt)
    for (const auto& [funcName, funcValue] : env->values) {
        std::string globalName = name.empty() ? funcName : name + "." + funcName;
        variables[globalName] = funcValue;
    }
}

// ============================================================================
// Function/callable invocation
// ============================================================================
Value Interpreter::call(const Value& callee, std::vector<Value>& args) {
    // Handle composed functions (f >>> g)
    if (callee.holds_alternative<std::shared_ptr<ClassInstance>>()) {
        auto inst = callee.get<std::shared_ptr<ClassInstance>>();
        if (inst->className == "__compose__") {
            Value leftFunc = inst->fields["__left"];
            Value rightFunc = inst->fields["__right"];
            std::vector<Value> leftArgs = args;
            Value intermediate = call(leftFunc, leftArgs);
            std::vector<Value> rightArgs = {intermediate};
            return call(rightFunc, rightArgs);
        }
        // Handle enum variant constructor
        if (inst->className == "__enum_ctor__") {
            std::string enumName = inst->fields["__enum_name"].get<std::string>();
            std::string variantName = inst->fields["__variant_name"].get<std::string>();
            auto paramList = inst->fields["__params"].get<std::vector<Value>>();
            if (args.size() != paramList.size()) {
                throw std::runtime_error(enumName + "." + variantName + " expects " +
                    std::to_string(paramList.size()) + " arguments but got " + std::to_string(args.size()) + ".");
            }
            auto instance = std::make_shared<ClassInstance>();
            instance->className = enumName + "." + variantName;
            for (size_t j = 0; j < paramList.size(); ++j) {
                instance->fields[paramList[j].get<std::string>()] = args[j];
            }
            return Value(instance);
        }
    }

    // Handle NativeFunction
    if (callee.holds_alternative<NativeFunction>()) {
        const auto& nativeFunc = callee.get<NativeFunction>();

        // Check arity (negative arity = variable args)
        if (nativeFunc.arity >= 0 && args.size() != static_cast<size_t>(nativeFunc.arity)) {
            throw std::runtime_error("Expected " + std::to_string(nativeFunc.arity) +
                                   " arguments but got " + std::to_string(args.size()) + ".");
        }

        return nativeFunc.function(args);
    }

    // Handle Lambda/closure
    if (callee.holds_alternative<LambdaValue>()) {
        const auto& lambda = callee.get<LambdaValue>();

        // Fill in defaults for missing args
        if (args.size() < lambda.parameters.size() && lambda.defaultExprs) {
            for (size_t i = args.size(); i < lambda.parameters.size(); ++i) {
                if (i < lambda.defaultExprs->size() && (*lambda.defaultExprs)[i]) {
                    args.push_back(evalExpr((*lambda.defaultExprs)[i]));
                } else {
                    break;
                }
            }
        }

        if (args.size() != lambda.parameters.size()) {
            throw std::runtime_error("Expected " + std::to_string(lambda.parameters.size()) +
                                   " arguments but got " + std::to_string(args.size()) + ".");
        }

        auto savedVars = variables;

        if (lambda.captured_env) {
            variables = *lambda.captured_env;
        }

        for (size_t i = 0; i < lambda.parameters.size(); ++i) {
            variables[lambda.parameters[i]] = args[i];
        }

        Value result;
        if (lambda.blockBody) {
            // Block body lambda: execute statements, catch return value
            try {
                execute(lambda.blockBody);
                result = Value();  // No explicit return → null
            } catch (const Value& returnValue) {
                result = returnValue;
            }
        } else {
            result = evalExpr(lambda.body);
        }

        variables = savedVars;

        return result;
    }

    // Handle user-defined FunctionStmt
    if (callee.holds_alternative<const FunctionStmt*>()) {
        const FunctionStmt* func = callee.get<const FunctionStmt*>();

        // Fill in defaults for missing args
        if (args.size() < func->parameters.size()) {
            for (size_t i = args.size(); i < func->parameters.size(); ++i) {
                if (i < func->parameterDefaults.size() && func->parameterDefaults[i]) {
                    args.push_back(evalExpr(func->parameterDefaults[i].get()));
                } else {
                    break;
                }
            }
        }

        if (args.size() != func->parameters.size()) {
            throw std::runtime_error("Expected " + std::to_string(func->parameters.size()) +
                                   " arguments but got " + std::to_string(args.size()) + ".");
        }

        auto previousEnv = environment;
        environment = std::make_shared<Environment>();

        for (size_t i = 0; i < func->parameters.size(); ++i) {
            environment->define(func->parameters[i], args[i]);
        }

        Value result;
        try {
            execute(func->body.get());
            result = Value();  // Return null if no explicit return
        } catch (const Value& returnValue) {
            result = returnValue;
        }

        environment = previousEnv;

        return result;
    }

    // Handle class constructor calls: ClassName(args...) → create instance + call init
    if (callee.holds_alternative<std::shared_ptr<ClassInstance>>()) {
        auto instance = callee.get<std::shared_ptr<ClassInstance>>();

        // Check for unimplemented abstract methods
        auto classIt = classes.find(instance->className);
        if (classIt != classes.end()) {
            // Walk the full method list to find any abstract methods
            std::string cn = instance->className;
            while (!cn.empty()) {
                auto cIt = classes.find(cn);
                if (cIt != classes.end()) {
                    for (const auto& method : cIt->second->methods) {
                        if (!method->body) {
                            // Abstract method - verify child has implementation
                            std::string implKey = instance->className + "." + method->name;
                            auto implIt = functions.find(implKey);
                            if (implIt == functions.end() || !implIt->second->body) {
                                throw std::runtime_error("Cannot instantiate class '" + instance->className +
                                    "': abstract method '" + method->name + "' not implemented.");
                            }
                        }
                    }
                    cn = cIt->second->parentName;
                } else {
                    break;
                }
            }
        }

        // Look for an init method (walk inheritance chain)
        const FunctionStmt* initFunc = nullptr;
        std::string cn = instance->className;
        while (!cn.empty()) {
            auto initIt = functions.find(cn + ".init");
            if (initIt != functions.end()) {
                initFunc = initIt->second;
                break;
            }
            auto cIt = classes.find(cn);
            if (cIt != classes.end() && !cIt->second->parentName.empty()) {
                cn = cIt->second->parentName;
            } else {
                break;
            }
        }

        if (initFunc) {
            // Fill in defaults
            if (args.size() < initFunc->parameters.size()) {
                for (size_t i = args.size(); i < initFunc->parameters.size(); ++i) {
                    if (i < initFunc->parameterDefaults.size() && initFunc->parameterDefaults[i]) {
                        args.push_back(evalExpr(initFunc->parameterDefaults[i].get()));
                    }
                }
            }

            if (args.size() != initFunc->parameters.size()) {
                throw std::runtime_error("Constructor " + instance->className + " expected " +
                    std::to_string(initFunc->parameters.size()) + " arguments but got " +
                    std::to_string(args.size()) + ".");
            }

            auto previousEnv = environment;
            environment = std::make_shared<Environment>();

            auto savedClassName = currentClassName;
            currentClassName = instance->className;

            // Bind 'this' to the instance
            environment->define("this", Value(instance));

            for (size_t i = 0; i < initFunc->parameters.size(); ++i) {
                environment->define(initFunc->parameters[i], args[i]);
            }

            try {
                execute(initFunc->body.get());
            } catch (const Value&) {
                // Return from init is ignored
            }

            currentClassName = savedClassName;
            environment = previousEnv;
        } else if (!args.empty()) {
            // Data class auto-init: assign args to fields in order
            auto classDefIt = classes.find(instance->className);
            if (classDefIt != classes.end() && classDefIt->second->isDataClass) {
                const auto& fields = classDefIt->second->fields;
                if (args.size() != fields.size()) {
                    throw std::runtime_error("Data class " + instance->className + " expected " +
                        std::to_string(fields.size()) + " arguments but got " + std::to_string(args.size()) + ".");
                }
                for (size_t i = 0; i < fields.size(); ++i) {
                    instance->fields[fields[i]] = args[i];
                }
            } else {
                throw std::runtime_error("Class " + instance->className + " has no init method but was called with arguments.");
            }
        }

        // Initialize lazy fields as monostate (to be evaluated on first access)
        if (classIt != classes.end()) {
            for (const auto& [lazyName, lazyExpr] : classIt->second->lazyFields) {
                if (instance->fields.find(lazyName) == instance->fields.end()) {
                    instance->fields[lazyName] = Value(); // monostate
                }
            }
        }

        return Value(instance);
    }

    throw std::runtime_error("Cannot call non-function value.");
}

// ============================================================================
// Statement execution
// ============================================================================
void Interpreter::execute(const Statement* stmt) {
    // ---- PrintStmt ----
    if (auto print = dynamic_cast<const PrintStmt*>(stmt)) {
        Value val = evalExpr(print->expression.get());
        std::cout << valueToString(val) << std::endl;
    }
    // ---- LetStmt ----
    else if (auto let = dynamic_cast<const LetStmt*>(stmt)) {
        // Check if expression is a struct constructor
        if (auto varExpr = dynamic_cast<const VariableExpr*>(let->expression.get())) {
            auto it = structs.find(varExpr->name);
            if (it != structs.end()) {
                std::unordered_map<std::string, Value> instance;
                for (const auto& field : it->second->fields) {
                    instance[field] = Value();
                }
                variables[let->name] = instance;
                if (!let->isMutable) {
                    immutableVars.insert(let->name);
                }
                return;
            }
        }
        Value val = evalExpr(let->expression.get());
        variables[let->name] = val;
        if (!let->isMutable) {
            immutableVars.insert(let->name);
        }
    }
    // ---- ConstStmt ----
    else if (auto constStmt = dynamic_cast<const ConstStmt*>(stmt)) {
        Value val = evalExpr(constStmt->expression.get());
        variables[constStmt->name] = val;
        immutableVars.insert(constStmt->name);
    }
    // ---- AssignStmt ----
    else if (auto assign = dynamic_cast<const AssignStmt*>(stmt)) {
        if (immutableVars.find(assign->name) != immutableVars.end()) {
            throw std::runtime_error("Cannot assign to immutable variable: " + assign->name);
        }
        Value val = evalExpr(assign->expression.get());
        // Check environment first
        if (environment && environment->values.count(assign->name)) {
            environment->values[assign->name] = val;
            return;
        }
        if (variables.find(assign->name) == variables.end()) {
            throw std::runtime_error("Undeclared variable: " + assign->name);
        }
        variables[assign->name] = val;
    }
    // ---- NEW: CompoundAssignStmt (+=, -=, *=, /=, %=) ----
    else if (auto compAssign = dynamic_cast<const CompoundAssignStmt*>(stmt)) {
        if (immutableVars.find(compAssign->name) != immutableVars.end()) {
            throw std::runtime_error("Cannot assign to immutable variable: " + compAssign->name);
        }

        // Get current value
        Value currentVal;
        bool inEnv = false;
        if (environment && environment->values.count(compAssign->name)) {
            currentVal = environment->values[compAssign->name];
            inEnv = true;
        } else if (variables.count(compAssign->name)) {
            currentVal = variables[compAssign->name];
        } else {
            throw std::runtime_error("Undeclared variable: " + compAssign->name);
        }

        // Evaluate the right-hand side
        Value rhsVal = evalExpr(compAssign->expression.get());

        // Create a temporary BinaryExpr to evaluate the operation
        // We reuse evalExpr's BinaryExpr logic by constructing temporary expressions
        auto leftExpr = std::make_unique<LiteralExpr>(currentVal);
        auto rightExpr = std::make_unique<LiteralExpr>(rhsVal);
        BinaryExpr tempBin(std::move(leftExpr), compAssign->op, std::move(rightExpr));

        Value result = evalExpr(&tempBin);

        if (inEnv) {
            environment->values[compAssign->name] = result;
        } else {
            variables[compAssign->name] = result;
        }
    }
    // ---- SetStmt ----
    else if (auto set = dynamic_cast<const SetStmt*>(stmt)) {
        auto object = evalExpr(set->object.get());
        auto index = evalExpr(set->index.get());
        auto value = evalExpr(set->value.get());

        if (object.holds_alternative<std::shared_ptr<ObjectInstance>>()) {
            auto instance = object.get<std::shared_ptr<ObjectInstance>>();
            if (!index.holds_alternative<std::string>()) throw std::runtime_error("Property key must be string.");
            instance->fields[index.get<std::string>()] = value;
        } else if (object.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = object.get<std::shared_ptr<ClassInstance>>();
            if (!index.holds_alternative<std::string>()) throw std::runtime_error("Property key must be string.");
            const std::string& propName = index.get<std::string>();

            // Check access modifier
            std::string accessKey = instance->className + "." + propName;
            auto accessIt = accessModifiers.find(accessKey);
            if (accessIt != accessModifiers.end() && accessIt->second == AccessModifier::Private) {
                if (currentClassName != instance->className) {
                    throw std::runtime_error("Cannot access private member '" + propName + "' of class '" + instance->className + "'.");
                }
            }

            // Check for setter
            std::string setterKey = instance->className + ".__setter_" + propName;
            auto setterIt = functions.find(setterKey);
            if (setterIt != functions.end()) {
                auto previousEnv = environment;
                environment = std::make_shared<Environment>();
                environment->define("this", Value(instance));
                environment->define(setterIt->second->parameters[0], value);
                auto savedClassName = currentClassName;
                currentClassName = instance->className;
                try {
                    execute(setterIt->second->body.get());
                } catch (const Value&) { /* ignore return */ }
                currentClassName = savedClassName;
                environment = previousEnv;
            } else {
                instance->fields[propName] = value;
            }
        } else {
            throw std::runtime_error("Trying to set property on something that is not an object or class instance.");
        }
    }
    // ---- IfStmt ----
    else if (auto ifstmt = dynamic_cast<const IfStmt*>(stmt)) {
        Value cond = evalExpr(ifstmt->condition.get());
        if (isTruthy(cond)) {
            execute(ifstmt->thenBranch.get());
        } else if (ifstmt->elseBranch) {
            execute(ifstmt->elseBranch.get());
        }
    }
    // ---- BlockStmt ----
    else if (auto block = dynamic_cast<const BlockStmt*>(stmt)) {
        deferStack.push_back({});

        try {
            for (const auto& inner : block->statements) {
                execute(inner.get());
            }
        } catch (...) {
            executeDeferredStatements();
            throw;
        }

        executeDeferredStatements();
    }
    // ---- FunctionStmt ----
    else if (auto func = dynamic_cast<const FunctionStmt*>(stmt)) {
        functions[func->name] = func;
    }
    // ---- StructStmt ----
    else if (auto structStmt = dynamic_cast<const StructStmt*>(stmt)) {
        structs[structStmt->name] = const_cast<StructStmt*>(structStmt);
    }
    // ---- ClassStmt ----
    else if (auto classStmt = dynamic_cast<const ClassStmt*>(stmt)) {
        // If there's a parent class, copy parent methods first (unless overridden)
        if (!classStmt->parentName.empty()) {
            // Check sealed
            auto sealedIt = sealedClasses.find(classStmt->parentName);
            if (sealedIt != sealedClasses.end() && sealedIt->second != currentFile) {
                throw std::runtime_error("Cannot extend sealed class '" + classStmt->parentName + "'.");
            }

            // Copy parent methods to child (if child doesn't override)
            std::string parentCn = classStmt->parentName;
            while (!parentCn.empty()) {
                auto parentClassIt = classes.find(parentCn);
                if (parentClassIt != classes.end()) {
                    for (const auto& method : parentClassIt->second->methods) {
                        std::string childKey = classStmt->name + "." + method->name;
                        if (functions.find(childKey) == functions.end()) {
                            functions[childKey] = method.get();
                        }
                    }
                    // Copy parent getters/setters
                    for (const auto& getter : parentClassIt->second->getters) {
                        std::string key = classStmt->name + ".__getter_" + getter->name;
                        if (functions.find(key) == functions.end()) {
                            functions[key] = getter.get();
                        }
                    }
                    for (const auto& setter : parentClassIt->second->setters) {
                        std::string key = classStmt->name + ".__setter_" + setter->name;
                        if (functions.find(key) == functions.end()) {
                            functions[key] = setter.get();
                        }
                    }
                    parentCn = parentClassIt->second->parentName;
                } else {
                    break;
                }
            }
        }

        // Register child methods (overriding parent if same name)
        for (size_t i = 0; i < classStmt->methods.size(); ++i) {
            const auto& method = classStmt->methods[i];
            functions[classStmt->name + "." + method->name] = method.get();
            // Store access modifiers
            if (i < classStmt->methodAccess.size()) {
                accessModifiers[classStmt->name + "." + method->name] = classStmt->methodAccess[i];
            }
        }

        // Store field access modifiers
        for (size_t i = 0; i < classStmt->fields.size(); ++i) {
            if (i < classStmt->fieldAccess.size()) {
                accessModifiers[classStmt->name + "." + classStmt->fields[i]] = classStmt->fieldAccess[i];
            }
        }

        // Register static methods
        for (const auto& method : classStmt->staticMethods) {
            functions[classStmt->name + "." + method->name] = method.get();
        }

        // Evaluate and register static fields
        for (const auto& [fieldName, expr] : classStmt->staticFields) {
            Value val = evalExpr(expr.get());
            variables[classStmt->name + "." + fieldName] = val;
        }

        // Register getters and setters
        for (const auto& getter : classStmt->getters) {
            functions[classStmt->name + ".__getter_" + getter->name] = getter.get();
        }
        for (const auto& setter : classStmt->setters) {
            functions[classStmt->name + ".__setter_" + setter->name] = setter.get();
        }

        // Handle sealed
        if (classStmt->isSealed) {
            sealedClasses[classStmt->name] = currentFile;
        }

        // Handle data class: auto-generate init, toString, __eq
        if (classStmt->isDataClass && !classStmt->fields.empty()) {
            // Don't generate if user defined these methods
            if (functions.find(classStmt->name + ".init") == functions.end()) {
                // init is handled dynamically at instantiation
            }
            // toString is handled in valueToString (already checks for toString method)
            // We register a marker so the system knows this is a data class
        }

        // Handle multiple trait implementation (mixins)
        for (const auto& traitName : classStmt->implTraits) {
            auto traitIt = traits.find(traitName);
            if (traitIt != traits.end()) {
                // Copy default methods from trait to class (class methods take precedence)
                for (const auto& requiredMethod : traitIt->second) {
                    std::string classKey = classStmt->name + "." + requiredMethod;
                    if (functions.find(classKey) == functions.end()) {
                        // Check for default implementation
                        auto defaultIt = traitDefaultMethods.find(traitName + "." + requiredMethod);
                        if (defaultIt != traitDefaultMethods.end()) {
                            functions[classKey] = defaultIt->second;
                        }
                    }
                }
                // Register class-trait association for 'is' operator
                classTraits[classStmt->name].push_back(traitName);
            }
        }

        // Store lazy field initializers for lazy properties
        for (const auto& [fieldName, initExpr] : classStmt->lazyFields) {
            // Store lazy initializer as a function-like thing
            // We'll store the AST pointer indexed by "ClassName.__lazy_fieldName"
            // and handle in GetExpr
            variables[classStmt->name + ".__lazy_" + fieldName] = Value(std::string("__lazy__"));
        }

        classes[classStmt->name] = const_cast<ClassStmt*>(classStmt);
    }
    // ---- ReturnStmt: handle nullptr value (bare return;) ----
    else if (auto ret = dynamic_cast<const ReturnStmt*>(stmt)) {
        if (ret->value) {
            Value val = evalExpr(ret->value.get());
            throw val;
        } else {
            // Bare "return;" returns monostate (null)
            throw Value(std::monostate{});
        }
    }
    // ---- ExpressionStmt ----
    else if (auto exprStmt = dynamic_cast<const ExpressionStmt*>(stmt)) {
        evalExpr(exprStmt->expression.get());
    }
    // ---- IndexAssignStmt ----
    else if (auto indexAssign = dynamic_cast<const IndexAssignStmt*>(stmt)) {
        auto varExpr = dynamic_cast<VariableExpr*>(indexAssign->listExpr.get());
        if (!varExpr) {
            throw std::runtime_error("The index expression must be a variable.");
        }

        // Look up the variable in environment or globals
        Value* varPtr = nullptr;
        if (environment && environment->values.count(varExpr->name)) {
            varPtr = &environment->values[varExpr->name];
        } else if (variables.count(varExpr->name)) {
            varPtr = &variables[varExpr->name];
        } else {
            throw std::runtime_error("Undeclared variable: " + varExpr->name);
        }

        Value& var = *varPtr;
        Value idxVal = evalExpr(indexAssign->indexExpr.get());
        Value value = evalExpr(indexAssign->valueExpr.get());

        if (var.holds_alternative<std::vector<Value>>()) {
            auto& vec = var.get<std::vector<Value>>();
            int idx;
            if (idxVal.holds_alternative<int>()) {
                idx = idxVal.get<int>();
            } else if (idxVal.holds_alternative<double>()) {
                idx = static_cast<int>(idxVal.get<double>());
            } else {
                throw std::runtime_error("List index must be an integer.");
            }

            // Negative indexing
            if (idx < 0) idx += static_cast<int>(vec.size());

            if (idx < 0 || idx >= static_cast<int>(vec.size()))
                throw std::runtime_error("Invalid index for list assignment.");

            const_cast<std::vector<Value>&>(vec)[idx] = value;
        }
        else if (var.holds_alternative<std::unordered_map<std::string, Value>>()) {
            auto& map = var.get<std::unordered_map<std::string, Value>>();
            std::string key;
            if (idxVal.holds_alternative<std::string>()) {
                key = idxVal.get<std::string>();
            } else {
                key = valueToString(idxVal);
            }

            const_cast<std::unordered_map<std::string, Value>&>(map)[key] = value;
        }
        else if (var.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            auto instance = var.get<std::shared_ptr<ClassInstance>>();
            if (!idxVal.holds_alternative<std::string>())
                throw std::runtime_error("Class field index must be a string.");
            const std::string& key = idxVal.get<std::string>();
            instance->fields[key] = value;
        }
        else {
            throw std::runtime_error("Attempt to index something that is neither a list, struct, nor class instance for assignment.");
        }
    }
    // ---- WhileStmt: uses BreakSignal/ContinueSignal ----
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        while (true) {
            Value cond = evalExpr(whileStmt->condition.get());
            if (!isTruthy(cond)) break;

            try {
                execute(whileStmt->body.get());
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
    // ---- LoopStmt: uses BreakSignal/ContinueSignal ----
    else if (auto loopStmt = dynamic_cast<const LoopStmt*>(stmt)) {
        while (true) {
            try {
                execute(loopStmt->body.get());
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
    // ---- ForStmt: iterator variable NOT added to immutableVars ----
    else if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        Value listVal = evalExpr(forStmt->iterable.get());

        if (!listVal.holds_alternative<std::vector<Value>>()) {
            // Try string iteration
            if (listVal.holds_alternative<std::string>()) {
                const auto& str = listVal.get<std::string>();
                for (size_t i = 0; i < str.size(); ++i) {
                    variables[forStmt->var] = std::string(1, str[i]);
                    try {
                        execute(forStmt->body.get());
                    } catch (const BreakSignal&) {
                        break;
                    } catch (const ContinueSignal&) {
                        continue;
                    }
                }
                return;
            }
            // Iterable protocol: check for __iter/__next on ClassInstance
            if (listVal.holds_alternative<std::shared_ptr<ClassInstance>>()) {
                auto instance = listVal.get<std::shared_ptr<ClassInstance>>();
                auto iterIt = functions.find(instance->className + ".__iter");
                auto nextIt = functions.find(instance->className + ".__next");
                if (iterIt != functions.end() && nextIt != functions.end()) {
                    // Call __iter() to get iterator
                    auto previousEnv = environment;
                    environment = std::make_shared<Environment>();
                    environment->define("this", Value(instance));
                    try {
                        execute(iterIt->second->body.get());
                    } catch (const Value&) {}
                    environment = previousEnv;

                    // Loop calling __next() until None
                    while (true) {
                        previousEnv = environment;
                        environment = std::make_shared<Environment>();
                        environment->define("this", Value(instance));
                        Value nextVal;
                        try {
                            execute(nextIt->second->body.get());
                            nextVal = Value(); // No return = None
                        } catch (const Value& returnValue) {
                            nextVal = returnValue;
                        }
                        environment = previousEnv;

                        if (nextVal.holds_alternative<std::monostate>()) break;

                        variables[forStmt->var] = nextVal;
                        try {
                            execute(forStmt->body.get());
                        } catch (const BreakSignal&) {
                            break;
                        } catch (const ContinueSignal&) {
                            continue;
                        }
                    }
                    return;
                }
            }
            throw std::runtime_error("Invalid iterable in for loop.");
        }

        const auto& vec = listVal.get<std::vector<Value>>();

        for (const auto& item : vec) {
            // BUG FIX: Do NOT add forStmt->var to immutableVars
            variables[forStmt->var] = item;

            try {
                execute(forStmt->body.get());
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
    // ---- BreakStmt: throw dedicated signal type ----
    else if (dynamic_cast<const BreakStmt*>(stmt)) {
        throw BreakSignal{};
    }
    // ---- ContinueStmt: throw dedicated signal type ----
    else if (dynamic_cast<const ContinueStmt*>(stmt)) {
        throw ContinueSignal{};
    }
    // ---- EnumStmt ----
    else if (auto en = dynamic_cast<const EnumStmt*>(stmt)) {
        // Create a ClassInstance to represent the enum itself (for Shape.Circle access)
        auto enumObj = std::make_shared<ClassInstance>();
        enumObj->className = "__enum__";

        int value = 0;
        for (size_t i = 0; i < en->values.size(); ++i) {
            const auto& name = en->values[i];
            // Check if variant has associated data
            if (i < en->variantParams.size() && !en->variantParams[i].empty()) {
                // Create a constructor marker for this variant
                auto ctor = std::make_shared<ClassInstance>();
                ctor->className = "__enum_ctor__";
                ctor->fields["__enum_name"] = Value(en->name);
                ctor->fields["__variant_name"] = Value(name);
                std::vector<Value> paramNames;
                for (const auto& p : en->variantParams[i]) {
                    paramNames.push_back(Value(p));
                }
                ctor->fields["__params"] = Value(paramNames);
                variables[en->name + "." + name] = Value(ctor);
                enumObj->fields[name] = Value(ctor);
            } else {
                // Simple enum variant (no params) - store as integer
                variables[en->name + "." + name] = value;
                enumObj->fields[name] = Value(value);
                value++;
            }
        }
        // Register the enum object so Shape.Circle works via GetExpr
        variables[en->name] = Value(enumObj);
    }
    // ---- MatchStmt ----
    else if (auto m = dynamic_cast<const MatchStmt*>(stmt)) {
        Value val = evalExpr(m->expr.get());

        bool matched = false;
        for (const auto& arm : m->arms) {
            std::unordered_map<std::string, Value> bindings;

            if (matchPattern(arm.pattern.get(), val, bindings)) {
                auto savedVars = variables;

                for (const auto& [name, value] : bindings) {
                    variables[name] = value;
                }

                try {
                    execute(arm.body.get());
                } catch (...) {
                    variables = savedVars;
                    throw;
                }

                variables = savedVars;
                matched = true;
                break;
            }
        }

        if (!matched) {
            throw std::runtime_error("No pattern matched in match expression.");
        }
    }
    // ---- SwitchStmt ----
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
    // ---- DeferStmt ----
    else if (auto deferStmt = dynamic_cast<const DeferStmt*>(stmt)) {
        if (deferStack.empty()) {
            throw std::runtime_error("Defer statement outside of scope.");
        }
        deferStack.back().push_back(deferStmt->statement.get());
    }
    // ---- AssertStmt ----
    else if (auto assertStmt = dynamic_cast<const AssertStmt*>(stmt)) {
        Value condition = evalExpr(assertStmt->condition.get());

        if (!isTruthy(condition)) {
            std::string message = assertStmt->message.empty()
                ? "Assertion failed"
                : "Assertion failed: " + assertStmt->message;
            throw std::runtime_error(message);
        }
    }
    // ---- NEW: TryCatchStmt (with optional finally) ----
    else if (auto tryCatch = dynamic_cast<const TryCatchStmt*>(stmt)) {
        bool hadError = false;
        std::exception_ptr exToRethrow = nullptr;

        try {
            execute(tryCatch->tryBlock.get());
        } catch (const std::runtime_error& e) {
            hadError = true;
            std::string errorMsg = e.what();

            // Multi-catch: filter by error types if specified
            if (!tryCatch->errorTypes.empty()) {
                bool matched = false;
                for (const auto& type : tryCatch->errorTypes) {
                    // Match if error message contains the type name
                    if (errorMsg.find(type) != std::string::npos) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    // Re-throw if no type matched
                    if (tryCatch->finallyBlock) {
                        try { execute(tryCatch->finallyBlock.get()); } catch (...) {}
                    }
                    throw;
                }
            }

            // Bind the error message to the error variable
            variables[tryCatch->errorVar] = errorMsg;

            try {
                execute(tryCatch->catchBlock.get());
            } catch (...) {
                variables.erase(tryCatch->errorVar);
                if (tryCatch->finallyBlock) {
                    // Execute finally even on re-throw
                    try { execute(tryCatch->finallyBlock.get()); } catch (...) {}
                }
                throw;
            }

            variables.erase(tryCatch->errorVar);
        } catch (...) {
            // Non-runtime_error exceptions (e.g. return values, break/continue)
            if (tryCatch->finallyBlock) {
                try { execute(tryCatch->finallyBlock.get()); } catch (...) {}
            }
            throw;
        }

        // Execute finally block (normal path)
        if (tryCatch->finallyBlock) {
            execute(tryCatch->finallyBlock.get());
        }
    }
    // ---- NEW: ThrowStmt ----
    else if (auto throwStmt = dynamic_cast<const ThrowStmt*>(stmt)) {
        Value val = evalExpr(throwStmt->expression.get());
        throw std::runtime_error(valueToString(val));
    }
    // ---- NEW: ImportStmt ----
    else if (auto importStmt = dynamic_cast<const ImportStmt*>(stmt)) {
        // First check native module registry (e.g., import 'net.http')
        if (loadNativeModule(importStmt->path)) {
            return;  // Native module loaded successfully
        }

        std::string filePath = importStmt->path;

        // If the path doesn't end with .yen, add the extension
        if (filePath.size() < 4 || filePath.substr(filePath.size() - 4) != ".yen") {
            filePath += ".yen";
        }

        // Resolve relative to current file
        std::filesystem::path resolvedPath;
        if (std::filesystem::path(filePath).is_relative()) {
            if (!currentFile.empty()) {
                resolvedPath = std::filesystem::path(currentFile).parent_path() / filePath;
            } else {
                resolvedPath = std::filesystem::current_path() / filePath;
            }
        } else {
            resolvedPath = filePath;
        }

        // Normalize the path
        resolvedPath = std::filesystem::weakly_canonical(resolvedPath);
        std::string canonicalPath = resolvedPath.string();

        // Check for circular imports
        if (importedFiles.count(canonicalPath)) {
            return;  // Already imported, skip
        }
        importedFiles.insert(canonicalPath);

        // Read the file
        std::ifstream file(canonicalPath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open import file: " + canonicalPath);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();
        file.close();

        // Save current file context
        std::string savedFile = currentFile;
        currentFile = canonicalPath;

        // Lex, parse, execute
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto stmts = parser.parse();

        if (parser.hadError()) {
            currentFile = savedFile;
            throw std::runtime_error("Parse error in imported file: " + canonicalPath);
        }

        // Execute the imported file's statements
        for (const auto& s : stmts) {
            execute(s.get());
        }

        // Restore file context
        currentFile = savedFile;
    }
    // ---- NEW: DoWhileStmt ----
    else if (auto doWhile = dynamic_cast<const DoWhileStmt*>(stmt)) {
        do {
            try {
                execute(doWhile->body.get());
            } catch (const BreakSignal&) {
                return;
            } catch (const ContinueSignal&) {
                // Continue to condition check
            }
        } while (isTruthy(evalExpr(doWhile->condition.get())));
    }
    // ---- NEW: DestructureLetStmt ----
    else if (auto destructure = dynamic_cast<const DestructureLetStmt*>(stmt)) {
        Value val = evalExpr(destructure->expression.get());
        if (!val.holds_alternative<std::vector<Value>>()) {
            throw std::runtime_error("Destructuring requires a list value.");
        }
        const auto& list = val.get<std::vector<Value>>();
        for (size_t i = 0; i < destructure->names.size(); ++i) {
            if (destructure->names[i] == "_") continue;  // Skip wildcard
            if (i < list.size()) {
                variables[destructure->names[i]] = list[i];
            } else {
                variables[destructure->names[i]] = Value();  // null for missing
            }
            if (!destructure->isMutable) {
                immutableVars.insert(destructure->names[i]);
            }
        }
    }
    // ---- NEW: GoStmt (goroutine-style concurrency) ----
    else if (auto goStmt = dynamic_cast<const GoStmt*>(stmt)) {
        auto callExpr = dynamic_cast<const CallExpr*>(goStmt->expression.get());
        if (callExpr) {
            // It's a function call: evaluate callee and args on the current thread
            Value callee = evalExpr(callExpr->callee.get());
            std::vector<Value> args;
            for (const auto& arg : callExpr->arguments) {
                if (auto spread = dynamic_cast<const SpreadExpr*>(arg.get())) {
                    Value spreadVal = evalExpr(spread->expression.get());
                    if (spreadVal.holds_alternative<std::vector<Value>>()) {
                        auto& vec = spreadVal.get<std::vector<Value>>();
                        args.insert(args.end(), vec.begin(), vec.end());
                    }
                } else {
                    args.push_back(evalExpr(arg.get()));
                }
            }

            // Create an independent Interpreter copy for the goroutine
            // This avoids race conditions from sharing variables/environment
            auto goroutineInterp = std::make_shared<Interpreter>(*this);

            std::thread t([goroutineInterp, callee, args]() mutable {
                try {
                    goroutineInterp->call(callee, args);
                } catch (const std::exception& e) {
                    std::cerr << "[goroutine error] " << e.what() << std::endl;
                } catch (...) {
                    // Silently ignore return value signals (e.g. thrown Value for return)
                }
            });
            t.detach();
        } else {
            // Evaluate the expression to get a callable (bare lambda/function, no args)
            Value callable = evalExpr(goStmt->expression.get());
            if (callable.holds_alternative<LambdaValue>() || callable.holds_alternative<NativeFunction>() || callable.holds_alternative<const FunctionStmt*>()) {
                auto goroutineInterp = std::make_shared<Interpreter>(*this);

                std::thread t([goroutineInterp, callable]() mutable {
                    try {
                        std::vector<Value> emptyArgs;
                        goroutineInterp->call(callable, emptyArgs);
                    } catch (const std::exception& e) {
                        std::cerr << "[goroutine error] " << e.what() << std::endl;
                    } catch (...) {}
                });
                t.detach();
            } else {
                throw std::runtime_error("go: expression must be a function call or callable.");
            }
        }
    }
    // ---- IncrementStmt: i++ / i-- ----
    else if (auto incStmt = dynamic_cast<const IncrementStmt*>(stmt)) {
        if (immutableVars.find(incStmt->name) != immutableVars.end()) {
            throw std::runtime_error("Cannot modify immutable variable: " + incStmt->name);
        }

        Value* varPtr = nullptr;
        if (environment && environment->values.count(incStmt->name)) {
            varPtr = &environment->values[incStmt->name];
        } else if (variables.count(incStmt->name)) {
            varPtr = &variables[incStmt->name];
        } else {
            throw std::runtime_error("Undeclared variable: " + incStmt->name);
        }

        Value& val = *varPtr;
        if (val.holds_alternative<int>()) {
            const_cast<int&>(val.get<int>()) += incStmt->isIncrement ? 1 : -1;
        } else if (val.holds_alternative<double>()) {
            const_cast<double&>(val.get<double>()) += incStmt->isIncrement ? 1.0 : -1.0;
        } else {
            throw std::runtime_error("Increment/decrement requires a numeric variable.");
        }
    }
    // ---- ForDestructureStmt: for [a, b] in list { ... } ----
    else if (auto forDestructure = dynamic_cast<const ForDestructureStmt*>(stmt)) {
        Value listVal = evalExpr(forDestructure->iterable.get());

        if (!listVal.holds_alternative<std::vector<Value>>()) {
            throw std::runtime_error("For-in destructuring requires a list.");
        }

        const auto& vec = listVal.get<std::vector<Value>>();

        for (const auto& item : vec) {
            if (!item.holds_alternative<std::vector<Value>>()) {
                throw std::runtime_error("For-in destructuring: each element must be a list.");
            }
            const auto& inner = item.get<std::vector<Value>>();

            for (size_t i = 0; i < forDestructure->vars.size(); ++i) {
                if (forDestructure->vars[i] == "_") continue;
                if (i < inner.size()) {
                    variables[forDestructure->vars[i]] = inner[i];
                } else {
                    variables[forDestructure->vars[i]] = Value();
                }
            }

            try {
                execute(forDestructure->body.get());
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
    // ---- RepeatStmt ----
    else if (auto repeatStmt = dynamic_cast<const RepeatStmt*>(stmt)) {
        Value countVal = evalExpr(repeatStmt->count.get());
        int count = 0;
        if (countVal.holds_alternative<int>()) count = countVal.get<int>();
        else if (countVal.holds_alternative<double>()) count = static_cast<int>(countVal.get<double>());
        else throw std::runtime_error("repeat count must be numeric.");

        for (int i = 0; i < count; ++i) {
            if (!repeatStmt->varName.empty()) {
                variables[repeatStmt->varName] = i;
            }
            try {
                execute(repeatStmt->body.get());
            } catch (const BreakSignal&) {
                break;
            } catch (const ContinueSignal&) {
                continue;
            }
        }
    }
    // ---- ExtendStmt ----
    else if (auto extendStmt = dynamic_cast<const ExtendStmt*>(stmt)) {
        for (const auto& method : extendStmt->methods) {
            extensionMethods[extendStmt->typeName + "." + method->name] = method.get();
        }
    }
    // ---- ObjectDestructureLetStmt ----
    else if (auto objDestructure = dynamic_cast<const ObjectDestructureLetStmt*>(stmt)) {
        Value val = evalExpr(objDestructure->expression.get());
        for (const auto& fieldName : objDestructure->fieldNames) {
            if (val.holds_alternative<std::shared_ptr<ClassInstance>>()) {
                auto instance = val.get<std::shared_ptr<ClassInstance>>();
                auto it = instance->fields.find(fieldName);
                variables[fieldName] = (it != instance->fields.end()) ? it->second : Value();
            } else if (val.holds_alternative<std::unordered_map<std::string, Value>>()) {
                const auto& map = val.get<std::unordered_map<std::string, Value>>();
                auto it = map.find(fieldName);
                variables[fieldName] = (it != map.end()) ? it->second : Value();
            } else {
                throw std::runtime_error("Object destructuring requires a class instance or map.");
            }
            if (!objDestructure->isMutable) {
                immutableVars.insert(fieldName);
            }
        }
    }
    // ---- TraitStmt ----
    else if (auto traitStmt = dynamic_cast<const TraitStmt*>(stmt)) {
        traits[traitStmt->name] = traitStmt->requiredMethods;
        // Register default methods
        for (const auto& method : traitStmt->defaultMethods) {
            traitDefaultMethods[traitStmt->name + "." + method->name] = method.get();
        }
    }
    // ---- ImplStmt ----
    else if (auto implStmt = dynamic_cast<const ImplStmt*>(stmt)) {
        // Verify trait exists
        auto traitIt = traits.find(implStmt->traitName);
        if (traitIt == traits.end()) {
            throw std::runtime_error("Unknown trait: " + implStmt->traitName);
        }

        // Register any methods from the impl block
        for (const auto& method : implStmt->methods) {
            functions[implStmt->className + "." + method->name] = method.get();
        }

        // Copy trait default methods (if class doesn't have them)
        for (auto& [key, func] : traitDefaultMethods) {
            if (key.substr(0, implStmt->traitName.size() + 1) == implStmt->traitName + ".") {
                std::string methodName = key.substr(implStmt->traitName.size() + 1);
                std::string classKey = implStmt->className + "." + methodName;
                if (functions.find(classKey) == functions.end()) {
                    functions[classKey] = func;
                }
            }
        }

        // Verify all required methods are implemented
        for (const auto& required : traitIt->second) {
            std::string classKey = implStmt->className + "." + required;
            if (functions.find(classKey) == functions.end()) {
                throw std::runtime_error("Class '" + implStmt->className + "' does not implement required method '" +
                    required + "' from trait '" + implStmt->traitName + "'.");
            }
        }

        // Register class→trait association
        classTraits[implStmt->className].push_back(implStmt->traitName);
    }
    // ---- ExportStmt ----
    else if (auto exportStmt = dynamic_cast<const ExportStmt*>(stmt)) {
        // Execute the contained statement (export is just a marker for the module system)
        if (exportStmt->statement) {
            execute(exportStmt->statement.get());
        }
    }
    // ---- ExternBlock ----
    else if (auto externBlock = dynamic_cast<const ExternBlock*>(stmt)) {
        // Extern blocks declare foreign functions; in the interpreter we just register them
        // as placeholders. Actual FFI is handled by native libraries.
        for (const auto& func : externBlock->functions) {
            // Register as a stub that throws if called
            // (real implementations should be provided by native libraries)
            if (variables.find(func->name) == variables.end()) {
                // Only register if not already provided by a native library
                // We don't throw here; the extern declaration is informational
            }
        }
    }
}

// ============================================================================
// Execute deferred statements in LIFO order
// ============================================================================
void Interpreter::executeDeferredStatements() {
    if (deferStack.empty()) return;

    auto& defers = deferStack.back();

    // Execute in reverse order (LIFO)
    for (auto it = defers.rbegin(); it != defers.rend(); ++it) {
        try {
            execute(*it);
        } catch (const std::exception& e) {
            std::cerr << "Error in deferred statement: " << e.what() << std::endl;
        } catch (const BreakSignal&) {
            // Ignore break/continue in deferred statements
        } catch (const ContinueSignal&) {
            // Ignore break/continue in deferred statements
        }
    }

    deferStack.pop_back();
}

// ============================================================================
// Pattern matching
// ============================================================================
bool Interpreter::matchPattern(const Pattern* pattern, const Value& value,
                                std::unordered_map<std::string, Value>& bindings) {
    // Wildcard pattern: always matches
    if (dynamic_cast<const WildcardPattern*>(pattern)) {
        return true;
    }

    // Literal pattern: exact match (with numeric type coercion)
    if (auto literal = dynamic_cast<const LiteralPattern*>(pattern)) {
        if (value == literal->value) return true;
        // Cross-type numeric comparison
        auto getNumeric = [](const Value& v, double& out) -> bool {
            if (v.holds_alternative<int>()) { out = v.get<int>(); return true; }
            if (v.holds_alternative<double>()) { out = v.get<double>(); return true; }
            if (v.holds_alternative<float>()) { out = v.get<float>(); return true; }
            return false;
        };
        double a, b;
        if (getNumeric(value, a) && getNumeric(literal->value, b)) return a == b;
        return false;
    }

    // Variable pattern: always matches and binds
    if (auto var = dynamic_cast<const VariablePattern*>(pattern)) {
        bindings[var->name] = value;
        return true;
    }

    // Range pattern: check if value is in range
    if (auto range = dynamic_cast<const RangePattern*>(pattern)) {
        int val = 0;
        if (value.holds_alternative<int>()) {
            val = value.get<int>();
        } else if (value.holds_alternative<double>()) {
            val = static_cast<int>(value.get<double>());
        } else {
            return false;
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
        if (value.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            const auto& instance = value.get<std::shared_ptr<ClassInstance>>();

            if (instance->className != structPat->structName) {
                return false;
            }

            for (const auto& [fieldName, fieldPattern] : structPat->fields) {
                auto it = instance->fields.find(fieldName);
                if (it == instance->fields.end()) {
                    return false;
                }

                if (!matchPattern(fieldPattern.get(), it->second, bindings)) {
                    return false;
                }
            }

            return true;
        }

        if (value.holds_alternative<std::shared_ptr<ObjectInstance>>()) {
            const auto& obj = value.get<std::shared_ptr<ObjectInstance>>();

            for (const auto& [fieldName, fieldPattern] : structPat->fields) {
                auto it = obj->fields.find(fieldName);
                if (it == obj->fields.end()) {
                    return false;
                }

                if (!matchPattern(fieldPattern.get(), it->second, bindings)) {
                    return false;
                }
            }

            return true;
        }

        // Also support matching against unordered_map (struct instances)
        if (value.holds_alternative<std::unordered_map<std::string, Value>>()) {
            const auto& map = value.get<std::unordered_map<std::string, Value>>();

            for (const auto& [fieldName, fieldPattern] : structPat->fields) {
                auto it = map.find(fieldName);
                if (it == map.end()) {
                    return false;
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

        auto savedVars = variables;
        for (const auto& [name, val] : tempBindings) {
            variables[name] = val;
        }

        Value guardResult = evalExpr(guarded->guard.get());

        variables = savedVars;

        if (isTruthy(guardResult)) {
            for (const auto& [name, val] : tempBindings) {
                bindings[name] = val;
            }
            return true;
        }

        return false;
    }

    return false;
}

// ============================================================================
// Execute a vector of statements
// ============================================================================
void Interpreter::execute(const std::vector<std::unique_ptr<Statement>>& statements) {
    for (const auto& stmt : statements) {
        execute(stmt.get());
    }
}
