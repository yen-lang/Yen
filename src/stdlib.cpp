#include "yen/stdlib.h"
#include "yen/value.h"
#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <variant>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

// Math functions
Value stdlib_sqrt(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("sqrt expects 1 argument.");
    if (args[0].holds_alternative<double>()) return std::sqrt(args[0].get<double>());
    if (args[0].holds_alternative<int>()) return std::sqrt(args[0].get<int>());
    throw std::runtime_error("sqrt expects a number.");
}

// System functions
Value stdlib_time(std::vector<Value>& args) {
    if (args.size() != 0) throw std::runtime_error("time expects 0 arguments.");
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

Value stdlib_exit(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("exit expects 1 argument.");
    if (args[0].holds_alternative<int>()) std::exit(args[0].get<int>());
    throw std::runtime_error("exit expects an integer exit code.");
}

// Type conversion functions
Value stdlib_str(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("str() expects 1 argument.");

    return std::visit(overloaded {
        [](const std::string& s) -> Value { return s; },
        [](int i) -> Value { return std::to_string(i); },
        [](double d) -> Value { return std::to_string(d); },
        [](float f) -> Value { return std::to_string(f); },
        [](bool b) -> Value { return b ? "true" : "false"; },
        [](std::monostate) -> Value { return std::string("null"); },
        [](const std::vector<Value>&) -> Value { return std::string("[list]"); },
        [](const std::unordered_map<std::string, Value>&) -> Value { return std::string("{struct}"); },
        [](const std::shared_ptr<ClassInstance>&) -> Value { return std::string("{instance}"); },
        [](const std::shared_ptr<ObjectInstance>&) -> Value { return std::string("{object}"); },
        [](const FunctionStmt*) -> Value { return std::string("{function}"); },
        [](const NativeFunction&) -> Value { return std::string("{native fn}"); },
        [](auto) -> Value { throw std::runtime_error("Cannot convert to string."); }
    }, args[0].data);
}

Value stdlib_int(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("int() expects 1 argument.");

    return std::visit(overloaded {
        [](int i) -> Value { return i; },
        [](double d) -> Value { return static_cast<int>(d); },
        [](float f) -> Value { return static_cast<int>(f); },
        [](bool b) -> Value { return b ? 1 : 0; },
        [](const std::string& s) -> Value {
            try {
                return std::stoi(s);
            } catch (...) {
                throw std::runtime_error("Cannot convert string '" + s + "' to int.");
            }
        },
        [](auto) -> Value { throw std::runtime_error("Cannot convert to int."); }
    }, args[0].data);
}

Value stdlib_float(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("float() expects 1 argument.");

    return std::visit(overloaded {
        [](double d) -> Value { return d; },
        [](float f) -> Value { return static_cast<double>(f); },
        [](int i) -> Value { return static_cast<double>(i); },
        [](bool b) -> Value { return b ? 1.0 : 0.0; },
        [](const std::string& s) -> Value {
            try {
                return std::stod(s);
            } catch (...) {
                throw std::runtime_error("Cannot convert string '" + s + "' to float.");
            }
        },
        [](auto) -> Value { throw std::runtime_error("Cannot convert to float."); }
    }, args[0].data);
}

Value stdlib_type(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("type() expects 1 argument.");

    return std::visit(overloaded {
        [](std::monostate) -> Value { return std::string("null"); },
        [](int) -> Value { return std::string("int"); },
        [](double) -> Value { return std::string("float"); },
        [](float) -> Value { return std::string("float"); },
        [](bool) -> Value { return std::string("bool"); },
        [](const std::string&) -> Value { return std::string("string"); },
        [](const std::vector<Value>&) -> Value { return std::string("list"); },
        [](const std::unordered_map<std::string, Value>&) -> Value { return std::string("struct"); },
        [](const std::shared_ptr<ClassInstance>&) -> Value { return std::string("class"); },
        [](const std::shared_ptr<ObjectInstance>&) -> Value { return std::string("object"); },
        [](const FunctionStmt*) -> Value { return std::string("function"); },
        [](const NativeFunction&) -> Value { return std::string("native_function"); },
        [](auto) -> Value { return std::string("unknown"); }
    }, args[0].data);
}

Value stdlib_len(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("len() expects 1 argument.");

    return std::visit(overloaded {
        [](const std::string& s) -> Value { return static_cast<int>(s.length()); },
        [](const std::vector<Value>& v) -> Value { return static_cast<int>(v.size()); },
        [](const std::unordered_map<std::string, Value>& m) -> Value { return static_cast<int>(m.size()); },
        [](auto) -> Value { throw std::runtime_error("len() requires string, list, or struct."); }
    }, args[0].data);
}

Value stdlib_range(std::vector<Value>& args) {
    if (args.size() < 1 || args.size() > 3) {
        throw std::runtime_error("range() expects 1-3 arguments.");
    }

    int start = 0, stop = 0, step = 1;

    if (args.size() == 1) {
        // range(stop)
        if (!args[0].holds_alternative<int>()) {
            throw std::runtime_error("range() arguments must be integers.");
        }
        stop = args[0].get<int>();
    } else if (args.size() == 2) {
        // range(start, stop)
        if (!args[0].holds_alternative<int>() || !args[1].holds_alternative<int>()) {
            throw std::runtime_error("range() arguments must be integers.");
        }
        start = args[0].get<int>();
        stop = args[1].get<int>();
    } else {
        // range(start, stop, step)
        if (!args[0].holds_alternative<int>() || !args[1].holds_alternative<int>() ||
            !args[2].holds_alternative<int>()) {
            throw std::runtime_error("range() arguments must be integers.");
        }
        start = args[0].get<int>();
        stop = args[1].get<int>();
        step = args[2].get<int>();

        if (step == 0) throw std::runtime_error("range() step cannot be zero.");
    }

    std::vector<Value> result;

    if (step > 0) {
        for (int i = start; i < stop; i += step) {
            result.push_back(Value(i));
        }
    } else {
        for (int i = start; i > stop; i += step) {
            result.push_back(Value(i));
        }
    }

    return result;
}

// List methods (functional style - return modified copies)
Value stdlib_list_push(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("push() expects 2 arguments (list, value).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("push() requires a list.");
    }

    auto& list = const_cast<std::vector<Value>&>(args[0].get<std::vector<Value>>());
    list.push_back(args[1]);
    return Value(); // Returns null; list modified in-place via write-back
}

Value stdlib_list_pop(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("pop() expects 1 argument (list).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("pop() requires a list.");
    }

    auto& list = const_cast<std::vector<Value>&>(args[0].get<std::vector<Value>>());
    if (list.empty()) throw std::runtime_error("pop() on empty list.");

    Value last = list.back();
    list.pop_back();
    return last; // Returns removed element; list modified in-place via write-back
}

Value stdlib_list_insert(std::vector<Value>& args) {
    if (args.size() != 3) throw std::runtime_error("insert() expects 3 arguments (list, index, value).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("insert() requires a list.");
    }
    if (!args[1].holds_alternative<int>()) {
        throw std::runtime_error("insert() index must be integer.");
    }

    auto list = args[0].get<std::vector<Value>>(); // Copy
    int index = args[1].get<int>();

    if (index < 0 || index > static_cast<int>(list.size())) {
        throw std::runtime_error("insert() index out of bounds.");
    }

    list.insert(list.begin() + index, args[2]);
    return list;
}

Value stdlib_list_remove(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("remove() expects 2 arguments (list, value).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("remove() requires a list.");
    }

    auto list = args[0].get<std::vector<Value>>(); // Copy
    const auto& target = args[1];

    auto it = std::find(list.begin(), list.end(), target);
    if (it == list.end()) {
        throw std::runtime_error("remove() value not found in list.");
    }

    list.erase(it);
    return list;
}

Value stdlib_list_contains(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("contains() expects 2 arguments (list, value).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("contains() requires a list.");
    }

    const auto& list = args[0].get<std::vector<Value>>();
    const auto& target = args[1];

    for (const auto& item : list) {
        if (item == target) return true;
    }
    return false;
}

Value stdlib_list_clear(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("clear() expects 1 argument (list).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("clear() requires a list.");
    }

    return std::vector<Value>(); // Return empty list
}

Value stdlib_list_sort(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("sort() expects 1 argument (list).");
    if (!args[0].holds_alternative<std::vector<Value>>()) {
        throw std::runtime_error("sort() requires a list.");
    }

    auto list = args[0].get<std::vector<Value>>(); // Copy
    std::sort(list.begin(), list.end());
    return list; // Return sorted copy
}

// String functions
Value stdlib_string_split(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("split() expects 2 arguments (string, delimiter).");
    if (!args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
        throw std::runtime_error("split() requires two strings.");
    }

    const std::string& str = args[0].get<std::string>();
    const std::string& delimiter = args[1].get<std::string>();

    std::vector<Value> result;

    if (delimiter.empty()) {
        // Split into individual characters
        for (char c : str) {
            result.push_back(Value(std::string(1, c)));
        }
        return result;
    }

    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        result.push_back(Value(str.substr(start, end - start)));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    result.push_back(Value(str.substr(start)));
    return result;
}

Value stdlib_string_join(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("join() expects 2 arguments (list, separator).");
    if (!args[0].holds_alternative<std::vector<Value>>() || !args[1].holds_alternative<std::string>()) {
        throw std::runtime_error("join() requires a list and a string.");
    }

    const auto& list = args[0].get<std::vector<Value>>();
    const std::string& separator = args[1].get<std::string>();

    std::string result;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i > 0) result += separator;

        // Convert element to string
        if (list[i].holds_alternative<std::string>()) {
            result += list[i].get<std::string>();
        } else {
            // Call str() function on element
            std::vector<Value> strArgs = {list[i]};
            Value strResult = stdlib_str(strArgs);
            result += strResult.get<std::string>();
        }
    }

    return result;
}

Value stdlib_string_substring(std::vector<Value>& args) {
    if (args.size() < 2 || args.size() > 3) {
        throw std::runtime_error("substring() expects 2-3 arguments (string, start, [end]).");
    }
    if (!args[0].holds_alternative<std::string>() || !args[1].holds_alternative<int>()) {
        throw std::runtime_error("substring() requires string and integer arguments.");
    }

    const std::string& str = args[0].get<std::string>();
    int start = args[1].get<int>();

    if (start < 0 || start > static_cast<int>(str.length())) {
        throw std::runtime_error("substring() start index out of bounds.");
    }

    if (args.size() == 3) {
        if (!args[2].holds_alternative<int>()) {
            throw std::runtime_error("substring() end must be integer.");
        }
        int end = args[2].get<int>();

        if (end < start || end > static_cast<int>(str.length())) {
            throw std::runtime_error("substring() end index out of bounds.");
        }

        return str.substr(start, end - start);
    }

    return str.substr(start);
}

Value stdlib_string_indexOf(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("indexOf() expects 2 arguments (string, substring).");
    if (!args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
        throw std::runtime_error("indexOf() requires two strings.");
    }

    const std::string& str = args[0].get<std::string>();
    const std::string& substr = args[1].get<std::string>();

    size_t pos = str.find(substr);
    if (pos == std::string::npos) {
        return -1;
    }

    return static_cast<int>(pos);
}

Value stdlib_string_toUpper(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("toUpper() expects 1 argument.");
    if (!args[0].holds_alternative<std::string>()) {
        throw std::runtime_error("toUpper() requires a string.");
    }

    std::string str = args[0].get<std::string>();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

Value stdlib_string_toLower(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("toLower() expects 1 argument.");
    if (!args[0].holds_alternative<std::string>()) {
        throw std::runtime_error("toLower() requires a string.");
    }

    std::string str = args[0].get<std::string>();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

Value stdlib_string_trim(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("trim() expects 1 argument.");
    if (!args[0].holds_alternative<std::string>()) {
        throw std::runtime_error("trim() requires a string.");
    }

    std::string str = args[0].get<std::string>();

    // Trim leading whitespace
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return std::string(""); // All whitespace
    }

    // Trim trailing whitespace
    size_t end = str.find_last_not_of(" \t\n\r\f\v");

    return str.substr(start, end - start + 1);
}

// Helper to extract numeric value
double to_double(const Value& v) {
    if (v.holds_alternative<double>()) return v.get<double>();
    if (v.holds_alternative<float>()) return static_cast<double>(v.get<float>());
    if (v.holds_alternative<int>()) return static_cast<double>(v.get<int>());
    throw std::runtime_error("Expected numeric value.");
}

// Math functions
Value stdlib_math_pow(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("pow() expects 2 arguments.");
    double base = to_double(args[0]);
    double exp = to_double(args[1]);
    return std::pow(base, exp);
}

Value stdlib_math_abs(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("abs() expects 1 argument.");

    if (args[0].holds_alternative<int>()) {
        return std::abs(args[0].get<int>());
    }
    return std::abs(to_double(args[0]));
}

Value stdlib_math_floor(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("floor() expects 1 argument.");
    return std::floor(to_double(args[0]));
}

Value stdlib_math_ceil(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("ceil() expects 1 argument.");
    return std::ceil(to_double(args[0]));
}

Value stdlib_math_round(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("round() expects 1 argument.");
    return std::round(to_double(args[0]));
}

Value stdlib_math_min(std::vector<Value>& args) {
    if (args.size() < 1) throw std::runtime_error("min() expects at least 1 argument.");

    // Check if all inputs are int
    bool allInt = true;
    for (const auto& arg : args) {
        if (!arg.holds_alternative<int>()) {
            allInt = false;
            break;
        }
    }

    if (allInt) {
        int intMin = args[0].get<int>();
        for (size_t i = 1; i < args.size(); ++i) {
            intMin = std::min(intMin, args[i].get<int>());
        }
        return intMin;
    }

    double minVal = to_double(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        minVal = std::min(minVal, to_double(args[i]));
    }
    return minVal;
}

Value stdlib_math_max(std::vector<Value>& args) {
    if (args.size() < 1) throw std::runtime_error("max() expects at least 1 argument.");

    // Check if all inputs are int
    bool allInt = true;
    for (const auto& arg : args) {
        if (!arg.holds_alternative<int>()) {
            allInt = false;
            break;
        }
    }

    if (allInt) {
        int intMax = args[0].get<int>();
        for (size_t i = 1; i < args.size(); ++i) {
            intMax = std::max(intMax, args[i].get<int>());
        }
        return intMax;
    }

    double maxVal = to_double(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        maxVal = std::max(maxVal, to_double(args[i]));
    }
    return maxVal;
}

Value stdlib_math_sin(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("sin() expects 1 argument.");
    return std::sin(to_double(args[0]));
}

Value stdlib_math_cos(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("cos() expects 1 argument.");
    return std::cos(to_double(args[0]));
}

Value stdlib_math_tan(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("tan() expects 1 argument.");
    return std::tan(to_double(args[0]));
}

// File I/O functions
Value stdlib_file_readFile(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("readFile() expects 1 argument.");
    if (!args[0].holds_alternative<std::string>()) {
        throw std::runtime_error("readFile() requires a string path.");
    }

    const std::string& path = args[0].get<std::string>();

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("readFile() failed to open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

Value stdlib_file_writeFile(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("writeFile() expects 2 arguments (path, content).");
    if (!args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
        throw std::runtime_error("writeFile() requires two strings.");
    }

    const std::string& path = args[0].get<std::string>();
    const std::string& content = args[1].get<std::string>();

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("writeFile() failed to open file: " + path);
    }

    file << content;

    if (!file) {
        throw std::runtime_error("writeFile() failed to write to file: " + path);
    }

    return Value(); // Return null on success
}

Value stdlib_file_appendFile(std::vector<Value>& args) {
    if (args.size() != 2) throw std::runtime_error("appendFile() expects 2 arguments (path, content).");
    if (!args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
        throw std::runtime_error("appendFile() requires two strings.");
    }

    const std::string& path = args[0].get<std::string>();
    const std::string& content = args[1].get<std::string>();

    std::ofstream file(path, std::ios::app);
    if (!file) {
        throw std::runtime_error("appendFile() failed to open file: " + path);
    }

    file << content;

    if (!file) {
        throw std::runtime_error("appendFile() failed to write to file: " + path);
    }

    return Value(); // Return null on success
}

Value stdlib_file_fileExists(std::vector<Value>& args) {
    if (args.size() != 1) throw std::runtime_error("fileExists() expects 1 argument.");
    if (!args[0].holds_alternative<std::string>()) {
        throw std::runtime_error("fileExists() requires a string path.");
    }

    const std::string& path = args[0].get<std::string>();

    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void initialize_globals(Interpreter& interpreter) {
    auto math_env = std::make_shared<Environment>();
    math_env->define("sqrt", Value(NativeFunction{stdlib_sqrt, 1}));
    math_env->define("pow", Value(NativeFunction{stdlib_math_pow, 2}));
    math_env->define("abs", Value(NativeFunction{stdlib_math_abs, 1}));
    math_env->define("floor", Value(NativeFunction{stdlib_math_floor, 1}));
    math_env->define("ceil", Value(NativeFunction{stdlib_math_ceil, 1}));
    math_env->define("round", Value(NativeFunction{stdlib_math_round, 1}));
    math_env->define("min", Value(NativeFunction{stdlib_math_min, -1})); // Variable arity
    math_env->define("max", Value(NativeFunction{stdlib_math_max, -1})); // Variable arity
    math_env->define("sin", Value(NativeFunction{stdlib_math_sin, 1}));
    math_env->define("cos", Value(NativeFunction{stdlib_math_cos, 1}));
    math_env->define("tan", Value(NativeFunction{stdlib_math_tan, 1}));

    auto sys_env = std::make_shared<Environment>();
    sys_env->define("time", Value(NativeFunction{stdlib_time, 0}));
    sys_env->define("exit", Value(NativeFunction{stdlib_exit, 1}));

    auto file_env = std::make_shared<Environment>();
    file_env->define("readFile", Value(NativeFunction{stdlib_file_readFile, 1}));
    file_env->define("writeFile", Value(NativeFunction{stdlib_file_writeFile, 2}));
    file_env->define("appendFile", Value(NativeFunction{stdlib_file_appendFile, 2}));
    file_env->define("fileExists", Value(NativeFunction{stdlib_file_fileExists, 1}));

    // Register modules
    interpreter.register_module("math", math_env);
    interpreter.register_module("system", sys_env);
    interpreter.register_module("file", file_env);

    // Register global functions (not in modules)
    auto globalEnv = std::make_shared<Environment>();
    globalEnv->define("str", Value(NativeFunction{stdlib_str, 1}));
    globalEnv->define("int", Value(NativeFunction{stdlib_int, 1}));
    globalEnv->define("float", Value(NativeFunction{stdlib_float, 1}));
    globalEnv->define("type", Value(NativeFunction{stdlib_type, 1}));
    globalEnv->define("len", Value(NativeFunction{stdlib_len, 1}));
    globalEnv->define("range", Value(NativeFunction{stdlib_range, -1})); // -1 = variable arity

    // List methods
    globalEnv->define("push", Value(NativeFunction{stdlib_list_push, 2}));
    globalEnv->define("pop", Value(NativeFunction{stdlib_list_pop, 1}));
    globalEnv->define("insert", Value(NativeFunction{stdlib_list_insert, 3}));
    globalEnv->define("remove", Value(NativeFunction{stdlib_list_remove, 2}));
    globalEnv->define("contains", Value(NativeFunction{stdlib_list_contains, 2}));
    globalEnv->define("clear", Value(NativeFunction{stdlib_list_clear, 1}));
    globalEnv->define("sort", Value(NativeFunction{stdlib_list_sort, 1}));

    // String functions
    globalEnv->define("split", Value(NativeFunction{stdlib_string_split, 2}));
    globalEnv->define("join", Value(NativeFunction{stdlib_string_join, 2}));
    globalEnv->define("substring", Value(NativeFunction{stdlib_string_substring, -1})); // Variable arity
    globalEnv->define("indexOf", Value(NativeFunction{stdlib_string_indexOf, 2}));
    globalEnv->define("toUpper", Value(NativeFunction{stdlib_string_toUpper, 1}));
    globalEnv->define("toLower", Value(NativeFunction{stdlib_string_toLower, 1}));
    globalEnv->define("trim", Value(NativeFunction{stdlib_string_trim, 1}));

    interpreter.register_module("", globalEnv); // Empty string = global namespace
}
