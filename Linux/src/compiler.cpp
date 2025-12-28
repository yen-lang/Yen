#include "value.h"
#include "compiler.h"
#include <iostream>
#include <algorithm>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Value Interpreter::evalExpr(const Expression* expr) {
    if (auto num = dynamic_cast<const NumberExpr*>(expr)) {
        return num->value;
    }

    if (auto lit = dynamic_cast<const LiteralExpr*>(expr)) {
        return lit->value;
    }
    if (auto thisExpr = dynamic_cast<const ThisExpr*>(expr)) {
        return environment->get("this"); 
    }
    if (auto getExpr = dynamic_cast<const GetExpr*>(expr)) {
        Value object = evalExpr(getExpr->object.get());
    
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(object)) {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(object);
            auto it = instance->fields.find(getExpr->name);
            if (it != instance->fields.end()) {
                return it->second;
            } else {
                throw std::runtime_error("Field '" + getExpr->name + "' not found.");
            }
        }
    
        throw std::runtime_error("Attempt to access a field on something that is not an object.");
    }        
    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        auto it = variables.find(var->name);
        if (it == variables.end())
            throw std::runtime_error("Undefined variable: " + var->name);
        return it->second;
    }
    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        auto it = variables.find(var->name);
        if (it == variables.end()) {
            auto classIt = classes.find(var->name);
            if (classIt != classes.end()) {
                // Create new instance
                auto instance = std::make_shared<ClassInstance>();
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
    if (auto get = dynamic_cast<const GetExpr*>(expr)) {
        Value object = evalExpr(get->object.get());
    
        if (std::holds_alternative<std::shared_ptr<ObjectInstance>>(object)) {
            auto instance = std::get<std::shared_ptr<ObjectInstance>>(object);
            auto it = instance->fields.find(get->name);
            if (it != instance->fields.end()) {
                return it->second;
            } else {
                throw std::runtime_error("Field '" + get->name + "' not found.");
            }
        }
    
        throw std::runtime_error("Attempt to access a field on something that is not an object.");
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
    
        if (std::holds_alternative<std::vector<Value>>(container)) {
            // It's a list
            const auto& list = std::get<std::vector<Value>>(container);
    
            if (!std::holds_alternative<int>(index))
                throw std::runtime_error("List index must be an integer.");
    
            int idx = std::get<int>(index);
            if (idx < 0 || idx >= (int)list.size())
                throw std::runtime_error("List index out of bounds.");
    
            return list[idx];
        }
        else if (std::holds_alternative<std::unordered_map<std::string, Value>>(container)) {
            // It's a struct
            const auto& map = std::get<std::unordered_map<std::string, Value>>(container);
    
            if (!std::holds_alternative<std::string>(index))
                throw std::runtime_error("Struct field must be a string.");
    
            const std::string& key = std::get<std::string>(index);
    
            auto it = map.find(key);
            if (it == map.end())
                throw std::runtime_error("Field '" + key + "' not found in struct.");
    
            return it->second;
        }
        else {
            throw std::runtime_error("Attempt to index something that is neither a list nor a struct.");
        }
    }    
    if (auto index = dynamic_cast<const IndexExpr*>(expr)) {
        auto listVal = evalExpr(index->listExpr.get());
        auto idxVal = evalExpr(index->indexExpr.get());
    
        if (!std::holds_alternative<std::vector<Value>>(listVal))
            throw std::runtime_error("Attempt to index something that is not a list.");
    
        int idx = std::get<int>(idxVal);
        auto& list = std::get<std::vector<Value>>(listVal);
    
        if (idx < 0 || idx >= (int)list.size())
            throw std::runtime_error("List index out of bounds.");
    
        return list[idx];
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
            if (std::holds_alternative<std::string>(val)) result += std::get<std::string>(val);
            else if (std::holds_alternative<int>(val)) result += std::to_string(std::get<int>(val));
            else if (std::holds_alternative<double>(val)) result += std::to_string(std::get<float>(val));
            else if (std::holds_alternative<bool>(val)) result += std::get<bool>(val) ? "true" : "false";
            else result += "null";
    
            pos = end + 1;
        }
    
        return result;
    }
    
    if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        if (auto var = dynamic_cast<VariableExpr*>(call->callee.get())) {
            if (var->name == "int" || var->name == "float" || var->name == "str" || var->name == "bool") {
                if (call->arguments.size() != 1)
                    throw std::runtime_error("Casting expects exactly 1 argument.");

                Value arg = evalExpr(call->arguments[0].get());

                if (var->name == "int") {
                    if (std::holds_alternative<std::string>(arg))
                        return std::stoi(std::get<std::string>(arg));
                    if (std::holds_alternative<float>(arg))
                        return static_cast<int>(std::get<double>(arg));
                    return std::get<int>(arg);
                }

                if (var->name == "float") {
                    if (std::holds_alternative<std::string>(arg))
                        return std::stof(std::get<std::string>(arg));
                    if (std::holds_alternative<int>(arg))
                        return static_cast<float>(std::get<int>(arg));
                    return std::get<double>(arg);
                }

                if (var->name == "str") {
                    if (std::holds_alternative<int>(arg))
                        return std::to_string(std::get<int>(arg));
                    if (std::holds_alternative<float>(arg))
                        return std::to_string(std::get<double>(arg));
                    if (std::holds_alternative<bool>(arg))
                        return std::get<bool>(arg) ? "true" : "false";
                    return std::get<std::string>(arg);
                }

                if (var->name == "bool") {
                    if (std::holds_alternative<int>(arg))
                        return std::get<int>(arg) != 0;
                    if (std::holds_alternative<float>(arg))
                        return std::get<float>(arg) != 0.0f;
                    if (std::holds_alternative<std::string>(arg))
                        return !std::get<std::string>(arg).empty();
                    return std::get<bool>(arg);
                }        
            }

            if (var->name == "len") {
                if (call->arguments.size() != 1)
                    throw std::runtime_error("len expects exactly 1 argument.");
            
                Value arg = evalExpr(call->arguments[0].get());
            
                if (std::holds_alternative<std::string>(arg)) {
                    return static_cast<int>(std::get<std::string>(arg).size());
                } else if (std::holds_alternative<std::vector<Value>>(arg)) {
                    return static_cast<int>(std::get<std::vector<Value>>(arg).size());
                } else {
                    throw std::runtime_error("len only works with strings or lists.");
                }
            }
            if (var->name == "push") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("push expects 2 arguments: list and value.");

                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("push: The first argument must be the name of a list variable.");

                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("push: variable is not a list.");

                Value val = evalExpr(call->arguments[1].get());
                std::get<std::vector<Value>>(listVal).push_back(val);
                return Value(); // void
            }
            if (var->name == "pop") {
                if (call->arguments.size() != 1)
                    throw std::runtime_error("pop expects 1 argument: list.");
            
                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("pop: the argument must be the name of a list variable.");
            
                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("pop: variable is not a list.");
            
                auto& vec = std::get<std::vector<Value>>(listVal);
                if (vec.empty())
                    throw std::runtime_error("pop: list is empty.");
            
                Value removed = vec.back();
                vec.pop_back();
                return removed;
            }
            
            if (var->name == "insert") {
                if (call->arguments.size() != 3)
                    throw std::runtime_error("insert expects 3 arguments: list, position, value.");
            
                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("insert: the first argument must be the name of a list variable.");
            
                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("insert: variable is not a list.");
            
                Value posVal = evalExpr(call->arguments[1].get());
                if (!std::holds_alternative<int>(posVal))
                    throw std::runtime_error("insert: position must be an integer.");
            
                int pos = std::get<int>(posVal);
                auto& vec = std::get<std::vector<Value>>(listVal);
                if (pos < 0 || pos > static_cast<int>(vec.size()))
                    throw std::runtime_error("insert: position out of list bounds.");
            
                Value val = evalExpr(call->arguments[2].get());
                vec.insert(vec.begin() + pos, val);
                return Value(); // void
            }
            
            if (var->name == "remove_at") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("remove_at expects 2 arguments: list and position.");
            
                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("remove_at: the first argument must be the name of a list variable.");
            
                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("remove_at: variable is not a list.");
            
                Value posVal = evalExpr(call->arguments[1].get());
                if (!std::holds_alternative<int>(posVal))
                    throw std::runtime_error("remove_at: position must be an integer.");
            
                int pos = std::get<int>(posVal);
                auto& vec = std::get<std::vector<Value>>(listVal);
                if (pos < 0 || pos >= static_cast<int>(vec.size()))
                    throw std::runtime_error("remove_at: invalid position.");
            
                Value removed = vec[pos];
                vec.erase(vec.begin() + pos);
                return removed;
            }
            
            if (var->name == "clear") {
                if (call->arguments.size() != 1)
                    throw std::runtime_error("clear expects 1 argument: the list.");
            
                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("clear: the argument must be the name of a list variable.");
            
                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("clear: variable is not a list.");
            
                std::get<std::vector<Value>>(listVal).clear();
                return Value(); // void
            }
            
            if (var->name == "contains") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("contains expects 2 arguments: list and value.");
            
                Value listVal = evalExpr(call->arguments[0].get());
                Value val = evalExpr(call->arguments[1].get());
            
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("contains: first argument is not a list.");
            
                const auto& vec = std::get<std::vector<Value>>(listVal);
                for (const auto& item : vec) {
                    if (item == val)
                        return true;
                }
                return false;
            }
            
            if (var->name == "reverse") {
                if (call->arguments.size() != 1)
                    throw std::runtime_error("reverse expects 1 argument (list).");
            
                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("reverse: argument must be a variable.");
            
                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("reverse: argument is not a list.");
            
                std::reverse(std::get<std::vector<Value>>(listVal).begin(), std::get<std::vector<Value>>(listVal).end());
                return Value();
            }
            
            if (var->name == "sort") {
                if (call->arguments.size() != 1)
                    throw std::runtime_error("sort expects 1 argument (list).");
            
                auto listExpr = dynamic_cast<const VariableExpr*>(call->arguments[0].get());
                if (!listExpr)
                    throw std::runtime_error("sort: argument must be a variable.");
            
                Value& listVal = variables[listExpr->name];
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("sort: argument is not a list.");
            
                auto& vec = std::get<std::vector<Value>>(listVal);
            
                for (const auto& item : vec) {
                    if (std::holds_alternative<std::unordered_map<std::string, Value>>(item)) {
                        throw std::runtime_error("sort: cannot sort a list of structs.");
                    }
                }
            
                std::sort(vec.begin(), vec.end(), [](const Value& a, const Value& b) {
                    if (a.index() != b.index())
                        throw std::runtime_error("sort: different types in list.");
                    return a < b;
                });
            
                return Value();
            }
            
            if (var->name == "join") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("join expects 2 arguments.");
            
                Value listVal = evalExpr(call->arguments[0].get());
                Value sepVal = evalExpr(call->arguments[1].get());
            
                if (!std::holds_alternative<std::vector<Value>>(listVal) || !std::holds_alternative<std::string>(sepVal))
                    throw std::runtime_error("join expects (list, separator).");
            
                const auto& list = std::get<std::vector<Value>>(listVal);
                const std::string& sep = std::get<std::string>(sepVal);
            
                std::string result;
                for (size_t i = 0; i < list.size(); ++i) {
                    const auto& item = list[i];
                    if (std::holds_alternative<std::string>(item))
                        result += std::get<std::string>(item);
                    else if (std::holds_alternative<int>(item))
                        result += std::to_string(std::get<int>(item));
                    else if (std::holds_alternative<float>(item))
                        result += std::to_string(std::get<float>(item));
                    else if (std::holds_alternative<bool>(item))
                        result += std::get<bool>(item) ? "true" : "false";
                    else
                        throw std::runtime_error("join only supports lists of strings, ints, floats, or bools.");
            
                    if (i + 1 < list.size()) result += sep;
                }
            
                return result;
            }
            
            if (var->name == "split") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("split expects 2 arguments.");
            
                Value strVal = evalExpr(call->arguments[0].get());
                Value sepVal = evalExpr(call->arguments[1].get());
            
                if (!std::holds_alternative<std::string>(strVal) || !std::holds_alternative<std::string>(sepVal))
                    throw std::runtime_error("split expects (string, separator).");
            
                const std::string& str = std::get<std::string>(strVal);
                const std::string& sep = std::get<std::string>(sepVal);
                std::vector<Value> result;
            
                size_t pos = 0, prev = 0;
                while ((pos = str.find(sep, prev)) != std::string::npos) {
                    result.push_back(str.substr(prev, pos - prev));
                    prev = pos + sep.length();
                }
                result.push_back(str.substr(prev));
                return result;
            }            
            if (var->name == "map") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("map expects 2 arguments (list, function).");
            
                Value listVal = evalExpr(call->arguments[0].get());
                auto funcVar = dynamic_cast<const VariableExpr*>(call->arguments[1].get());
                if (!funcVar) 
                    throw std::runtime_error("The second argument to map must be a function name.");
            
                auto it = functions.find(funcVar->name);
                if (it == functions.end()) 
                    throw std::runtime_error("Function not found: " + funcVar->name);
            
                const FunctionStmt* func = it->second;
                if (func->parameters.size() != 1)
                    throw std::runtime_error("Function used in map must take 1 parameter.");
            
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("map expects a list.");
            
                std::vector<Value> result;
                for (const auto& item : std::get<std::vector<Value>>(listVal)) {
                    auto oldVars = variables;
                    variables[func->parameters[0]] = item;
                    try {
                        execute(func->body.get());
                    } catch (const Value& retVal) {
                        result.push_back(retVal);
                    }
                    variables = oldVars;
                }
            
                return result;
            }
            
            if (var->name == "filter") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("filter expects 2 arguments (list, function).");
            
                Value listVal = evalExpr(call->arguments[0].get());
                auto funcVar = dynamic_cast<const VariableExpr*>(call->arguments[1].get());
                if (!funcVar) 
                    throw std::runtime_error("The second argument to filter must be a function name.");
            
                auto it = functions.find(funcVar->name);
                if (it == functions.end()) 
                    throw std::runtime_error("Function not found: " + funcVar->name);
            
                const FunctionStmt* func = it->second;
                if (func->parameters.size() != 1)
                    throw std::runtime_error("Function used in filter must take 1 parameter.");
            
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("filter expects a list.");
            
                std::vector<Value> result;
                for (const auto& item : std::get<std::vector<Value>>(listVal)) {
                    auto oldVars = variables;
                    variables[func->parameters[0]] = item;
                    try {
                        execute(func->body.get());
                    } catch (const Value& retVal) {
                        if (std::holds_alternative<bool>(retVal) && std::get<bool>(retVal))
                            result.push_back(item);
                    }
                    variables = oldVars;
                }
            
                return result;
            }
            
            if (var->name == "reduce") {
                if (call->arguments.size() != 3)
                    throw std::runtime_error("reduce expects 3 arguments (list, function, initial value).");
            
                Value listVal = evalExpr(call->arguments[0].get());
                auto funcVar = dynamic_cast<const VariableExpr*>(call->arguments[1].get());
                Value accInit = evalExpr(call->arguments[2].get());
            
                if (!funcVar) 
                    throw std::runtime_error("The second argument to reduce must be a function name.");
            
                auto it = functions.find(funcVar->name);
                if (it == functions.end()) 
                    throw std::runtime_error("Function not found: " + funcVar->name);
            
                const FunctionStmt* func = it->second;
                if (func->parameters.size() != 2)
                    throw std::runtime_error("Function used in reduce must take 2 parameters.");
            
                if (!std::holds_alternative<std::vector<Value>>(listVal))
                    throw std::runtime_error("reduce expects a list.");
            
                Value acc = accInit;
                for (const auto& item : std::get<std::vector<Value>>(listVal)) {
                    auto oldVars = variables;
                    variables[func->parameters[0]] = acc;
                    variables[func->parameters[1]] = item;
            
                    try {
                        execute(func->body.get());
                    } catch (const Value& retVal) {
                        acc = retVal;
                    }
            
                    variables = oldVars;
                }
            
                return acc;
            }
            
            if (var->name == "range") {
                if (call->arguments.size() != 2)
                    throw std::runtime_error("range expects 2 arguments: start and end.");
            
                Value startVal = evalExpr(call->arguments[0].get());
                Value endVal = evalExpr(call->arguments[1].get());
            
                if (!std::holds_alternative<int>(startVal) || !std::holds_alternative<int>(endVal))
                    throw std::runtime_error("range only works with integers.");
            
                int start = std::get<int>(startVal);
                int end = std::get<int>(endVal);
            
                std::vector<Value> rangeVec;
                for (int i = start; i < end; ++i) {
                    rangeVec.push_back(i);
                }
            
                return rangeVec;
            }
            
            // User-defined function
            auto it = functions.find(var->name);
            if (it == functions.end()) {
                throw std::runtime_error("Function not found: " + var->name);
            }
            
            const FunctionStmt* func = it->second;
            
            if (func->parameters.size() != call->arguments.size()) {
                throw std::runtime_error("Incorrect number of arguments for function: " + var->name);
            }
            
            auto oldVars = variables;
            for (size_t i = 0; i < call->arguments.size(); ++i) {
                Value argVal = evalExpr(call->arguments[i].get());
                variables[func->parameters[i]] = argVal;
            }
            
            try {
                execute(func->body.get());
            } catch (const Value& retVal) {
                variables = oldVars;
                return retVal;
            }
            
            variables = oldVars;
            return Value(); // void
        }
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
                }, left, right);
    
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
                }, left, right);
    
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
                }, left, right);
    
            case BinaryOp::Div:
                return std::visit(overloaded {
                    [](int l, int r) {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(static_cast<float>(l) / r);
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
                        return Value(l / r);
                    },
                    [](float l, int r) {
                        if (r == 0) throw std::runtime_error("Division by zero.");
                        return Value(l / r);
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
                }, left, right);    

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
                }, left, right);


            case BinaryOp::Greater:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l > r); },
                    [](float l, float r) { return Value(l > r); },
                    [](int l, float r) { return Value(l > r); },
                    [](float l, int r) { return Value(l > r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for >"); }
                }, left, right);

            case BinaryOp::GreaterEqual:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l >= r); },
                    [](float l, float r) { return Value(l >= r); },
                    [](int l, float r) { return Value(l >= r); },
                    [](float l, int r) { return Value(l >= r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for >="); }
                }, left, right);

            case BinaryOp::LessEqual:
                return std::visit(overloaded {
                    [](int l, int r) { return Value(l <= r); },
                    [](float l, float r) { return Value(l <= r); },
                    [](int l, float r) { return Value(l <= r); },
                    [](float l, int r) { return Value(l <= r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Incompatible types for  <="); }
                }, left, right);

            case BinaryOp::And:
                return std::visit(overloaded {
                    [](bool l, bool r) { return Value(l && r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Invalid types for operator '&&'"); }
                }, left, right);

            case BinaryOp::Or:
                return std::visit(overloaded {
                    [](bool l, bool r) { return Value(l || r); },
                    [](auto, auto) -> Value { throw std::runtime_error("Invalid types for operator '||'"); }
                }, left, right);
        }
    }

    if (auto unary = dynamic_cast<const UnaryExpr*>(expr)) {
        Value right = evalExpr(unary->right.get());

        switch (unary->op) {
            case UnaryOp::Not:
                return std::visit(overloaded {
                    [](bool b) { return Value(!b); },
                    [](auto) -> Value { throw std::runtime_error("Operator '!' requires boolean."); }
                }, right);
        }
    }

    throw std::runtime_error("Invalid expression.");
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
                        } else if constexpr (std::is_same_v<T, std::shared_ptr<ClassInstance>>) {
                            std::cout << "{instance of class}" << std::endl;
                        } else {
                            std::cout << elem;
                        }
                    }, v[i]);
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
                        } else {
                            std::cout << elem;
                        }
                    }, val);
                }
                std::cout << "}" << std::endl;
            } else {
                std::cout << v << std::endl;
            }
        }, val);
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
                return;
            }
        }
        Value val = evalExpr(let->expression.get());
        variables[let->name] = val;
    }
    else if (auto assign = dynamic_cast<const AssignStmt*>(stmt)) {
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
    
        if (!std::holds_alternative<std::shared_ptr<ObjectInstance>>(object)) {
            throw std::runtime_error("Trying to define ownership on something that is not an object.");
        }
        auto instance = std::get<std::shared_ptr<ObjectInstance>>(object);
    
        if (!std::holds_alternative<std::string>(index)) {
            throw std::runtime_error("Property key must be string.");
        }
    
        std::string key = std::get<std::string>(index);
        instance->fields[key] = value;
    }    
    else if (auto ifstmt = dynamic_cast<const IfStmt*>(stmt)) {
        Value cond = evalExpr(ifstmt->condition.get());
        bool condResult = false;

        if (std::holds_alternative<bool>(cond)) {
            condResult = std::get<bool>(cond);
        } else if (std::holds_alternative<int>(cond)) {
            condResult = std::get<int>(cond) != 0;
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
        for (const auto& inner : block->statements) {
            execute(inner.get());
        }
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
    
        if (std::holds_alternative<std::vector<Value>>(var)) {
            auto& vec = std::get<std::vector<Value>>(var);
    
            if (!std::holds_alternative<int>(idxVal))
                throw std::runtime_error("List index must be an integer.");
    
            int idx = std::get<int>(idxVal);
    
            if (idx < 0 || idx >= (int)vec.size())
                throw std::runtime_error("Invalid index for list assignment.");
    
            vec[idx] = value;
        }
        else if (std::holds_alternative<std::unordered_map<std::string, Value>>(var)) {
            auto& map = std::get<std::unordered_map<std::string, Value>>(var);
    
            if (!std::holds_alternative<std::string>(idxVal))
                throw std::runtime_error("Struct field must be a string.");
    
            const std::string& key = std::get<std::string>(idxVal);
    
            if (map.find(key) == map.end())
                throw std::runtime_error("Field '" + key + "' does not exist in struct.");
    
            map[key] = value;
        }
        else {
            throw std::runtime_error("Attempt to index something that is neither a list nor a struct.");
        }
    }    
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        while (true) {
            Value cond = evalExpr(whileStmt->condition.get());
            bool condResult = false;

            if (std::holds_alternative<bool>(cond))
                condResult = std::get<bool>(cond);
            else if (std::holds_alternative<int>(cond))
                condResult = std::get<int>(cond) != 0;
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
    else if (auto forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        Value listVal = evalExpr(forStmt->iterable.get());

        if (!std::holds_alternative<std::vector<Value>>(listVal))
            throw std::runtime_error("Invalid iterable in for.");

        auto& vec = std::get<std::vector<Value>>(listVal);

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
        for (const auto& [pattern, armStmt] : m->arms) {
            auto it = variables.find(pattern);
            if (it == variables.end())
                throw std::runtime_error("Enum value not found: " + pattern);

            if (val == it->second) {
                execute(armStmt.get());
                matched = true;
                break;
            }
        }

        if (!matched) {
            throw std::runtime_error("No values ​​match.");
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
    else if (auto classStmt = dynamic_cast<const ClassStmt*>(stmt)) {
        std::unordered_map<std::string, Value> classFields;
        for (const auto& field : classStmt->fields) {
            classFields[field] = Value(); 
        }
    
        for (const auto& method : classStmt->methods) {
            functions[classStmt->name + "." + method->name] = method.get();
        }
    
        structs[classStmt->name] = new StructStmt(classStmt->name, classStmt->fields);
    }    
}

void Interpreter::execute(const std::vector<std::unique_ptr<Statement>>& statements) {
    for (const auto& stmt : statements) {
        execute(stmt.get());
    }
}
