#include "yen/native_libs.h"
#include "yen/value.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <functional>
#include <regex>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#endif

namespace YenNative {

// Helper: extract double from Value
static double toDouble(const Value& val) {
    if (val.holds_alternative<double>()) return val.get<double>();
    if (val.holds_alternative<float>()) return static_cast<double>(val.get<float>());
    if (val.holds_alternative<int>()) return static_cast<double>(val.get<int>());
    throw std::runtime_error("Expected numeric value.");
}

// Helper: extract int from Value
static int toInt(const Value& val) {
    if (val.holds_alternative<int>()) return val.get<int>();
    if (val.holds_alternative<double>()) return static_cast<int>(val.get<double>());
    if (val.holds_alternative<float>()) return static_cast<int>(val.get<float>());
    throw std::runtime_error("Expected integer value.");
}

// ============ CORE LIBRARY ============
namespace Core {
    Value isInt(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<int>();
    }

    Value isFloat(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<float>() || args[0].holds_alternative<double>();
    }

    Value isBool(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<bool>();
    }

    Value isString(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<std::string>();
    }

    Value isList(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<std::vector<Value>>();
    }

    Value isMap(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<std::unordered_map<std::string, Value>>();
    }

    Value isFunc(std::vector<Value>& args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<const FunctionStmt*>() ||
               args[0].holds_alternative<NativeFunction>() ||
               args[0].holds_alternative<LambdaValue>();
    }

    Value toIntFn(std::vector<Value>& args) {
        if (args.empty()) return 0;
        const auto& val = args[0];
        if (val.holds_alternative<int>()) return val.get<int>();
        if (val.holds_alternative<double>()) return static_cast<int>(val.get<double>());
        if (val.holds_alternative<float>()) return static_cast<int>(val.get<float>());
        if (val.holds_alternative<bool>()) return val.get<bool>() ? 1 : 0;
        if (val.holds_alternative<std::string>()) {
            const auto& s = val.get<std::string>();
            try {
                // Handle hex
                if (s.size() > 2 && s[0] == '0' && s[1] == 'x')
                    return static_cast<int>(std::stol(s, nullptr, 16));
                // Handle binary
                if (s.size() > 2 && s[0] == '0' && s[1] == 'b')
                    return static_cast<int>(std::stol(s.substr(2), nullptr, 2));
                return std::stoi(s);
            } catch (...) {
                return 0;
            }
        }
        return 0;
    }

    Value toFloatFn(std::vector<Value>& args) {
        if (args.empty()) return 0.0;
        const auto& val = args[0];
        if (val.holds_alternative<double>()) return val.get<double>();
        if (val.holds_alternative<float>()) return static_cast<double>(val.get<float>());
        if (val.holds_alternative<int>()) return static_cast<double>(val.get<int>());
        if (val.holds_alternative<std::string>()) {
            try { return std::stod(val.get<std::string>()); } catch (...) { return 0.0; }
        }
        return 0.0;
    }

    Value toStringFn(std::vector<Value>& args) {
        if (args.empty()) return std::string("");
        const auto& val = args[0];
        if (val.holds_alternative<std::string>()) return val.get<std::string>();
        if (val.holds_alternative<int>()) return std::to_string(val.get<int>());
        if (val.holds_alternative<double>()) return std::to_string(val.get<double>());
        if (val.holds_alternative<float>()) return std::to_string(val.get<float>());
        if (val.holds_alternative<bool>()) return val.get<bool>() ? std::string("true") : std::string("false");
        if (val.holds_alternative<std::monostate>()) return std::string("null");
        return std::string("<value>");
    }

    Value typeofFn(std::vector<Value>& args) {
        if (args.empty()) return std::string("null");
        const auto& val = args[0];
        if (val.holds_alternative<std::monostate>()) return std::string("null");
        if (val.holds_alternative<int>()) return std::string("int");
        if (val.holds_alternative<double>()) return std::string("float");
        if (val.holds_alternative<float>()) return std::string("float");
        if (val.holds_alternative<bool>()) return std::string("bool");
        if (val.holds_alternative<std::string>()) return std::string("string");
        if (val.holds_alternative<std::vector<Value>>()) return std::string("list");
        if (val.holds_alternative<std::unordered_map<std::string, Value>>()) return std::string("map");
        if (val.holds_alternative<std::shared_ptr<ClassInstance>>()) {
            return std::string("class:" + val.get<std::shared_ptr<ClassInstance>>()->className);
        }
        if (val.holds_alternative<std::shared_ptr<ObjectInstance>>()) return std::string("struct");
        if (val.holds_alternative<const FunctionStmt*>()) return std::string("function");
        if (val.holds_alternative<NativeFunction>()) return std::string("native_function");
        if (val.holds_alternative<LambdaValue>()) return std::string("lambda");
        return std::string("unknown");
    }

    Value printlnFn(std::vector<Value>& args) {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << " ";
            const auto& val = args[i];
            if (val.holds_alternative<std::string>()) std::cout << val.get<std::string>();
            else if (val.holds_alternative<int>()) std::cout << val.get<int>();
            else if (val.holds_alternative<double>()) std::cout << val.get<double>();
            else if (val.holds_alternative<bool>()) std::cout << (val.get<bool>() ? "true" : "false");
            else if (val.holds_alternative<std::monostate>()) std::cout << "null";
            else std::cout << "<value>";
        }
        std::cout << std::endl;
        return Value();
    }

    Value panicFn(std::vector<Value>& args) {
        std::string msg = "panic!";
        if (!args.empty() && args[0].holds_alternative<std::string>()) {
            msg = args[0].get<std::string>();
        }
        throw std::runtime_error("PANIC: " + msg);
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["core_is_int"] = NativeFunction{isInt, 1};
        globals["core_is_float"] = NativeFunction{isFloat, 1};
        globals["core_is_bool"] = NativeFunction{isBool, 1};
        globals["core_is_string"] = NativeFunction{isString, 1};
        globals["core_is_list"] = NativeFunction{isList, 1};
        globals["core_is_map"] = NativeFunction{isMap, 1};
        globals["core_is_func"] = NativeFunction{isFunc, 1};
        globals["core_to_int"] = NativeFunction{toIntFn, 1};
        globals["core_to_float"] = NativeFunction{toFloatFn, 1};
        globals["core_to_string"] = NativeFunction{toStringFn, 1};
        globals["typeof"] = NativeFunction{typeofFn, 1};
        globals["println"] = NativeFunction{printlnFn, -1};
        globals["panic"] = NativeFunction{panicFn, 1};
    }
}

// ============ MATH LIBRARY ============
namespace Math {
    Value abs_fn(std::vector<Value>& args) {
        if (args.empty()) return 0;
        if (args[0].holds_alternative<int>()) return std::abs(args[0].get<int>());
        return std::abs(toDouble(args[0]));
    }

    Value sqrt_fn(std::vector<Value>& args) {
        if (args.empty()) return 0.0;
        return std::sqrt(toDouble(args[0]));
    }

    Value pow_fn(std::vector<Value>& args) {
        if (args.size() < 2) return 0.0;
        return std::pow(toDouble(args[0]), toDouble(args[1]));
    }

    Value sin_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::sin(toDouble(args[0]))); }
    Value cos_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::cos(toDouble(args[0]))); }
    Value tan_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::tan(toDouble(args[0]))); }
    Value asin_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::asin(toDouble(args[0]))); }
    Value acos_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::acos(toDouble(args[0]))); }
    Value atan_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::atan(toDouble(args[0]))); }
    Value atan2_fn(std::vector<Value>& args) { return args.size() < 2 ? Value(0.0) : Value(std::atan2(toDouble(args[0]), toDouble(args[1]))); }
    Value log_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::log(toDouble(args[0]))); }
    Value log10_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::log10(toDouble(args[0]))); }
    Value exp_fn(std::vector<Value>& args) { return args.empty() ? Value(0.0) : Value(std::exp(toDouble(args[0]))); }

    Value floor_fn(std::vector<Value>& args) {
        if (args.empty()) return 0;
        if (args[0].holds_alternative<int>()) return args[0].get<int>();
        return static_cast<int>(std::floor(toDouble(args[0])));
    }

    Value ceil_fn(std::vector<Value>& args) {
        if (args.empty()) return 0;
        if (args[0].holds_alternative<int>()) return args[0].get<int>();
        return static_cast<int>(std::ceil(toDouble(args[0])));
    }

    Value round_fn(std::vector<Value>& args) {
        if (args.empty()) return 0;
        if (args[0].holds_alternative<int>()) return args[0].get<int>();
        return static_cast<int>(std::round(toDouble(args[0])));
    }

    Value min_fn(std::vector<Value>& args) {
        if (args.size() < 2) return 0;
        if (args[0].holds_alternative<int>() && args[1].holds_alternative<int>())
            return std::min(args[0].get<int>(), args[1].get<int>());
        return std::min(toDouble(args[0]), toDouble(args[1]));
    }

    Value max_fn(std::vector<Value>& args) {
        if (args.size() < 2) return 0;
        if (args[0].holds_alternative<int>() && args[1].holds_alternative<int>())
            return std::max(args[0].get<int>(), args[1].get<int>());
        return std::max(toDouble(args[0]), toDouble(args[1]));
    }

    Value clamp_fn(std::vector<Value>& args) {
        if (args.size() < 3) return 0;
        double val = toDouble(args[0]);
        double lo = toDouble(args[1]);
        double hi = toDouble(args[2]);
        if (val < lo) return lo;
        if (val > hi) return hi;
        return val;
    }

    Value random_fn(std::vector<Value>& args) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }

    Value random_int_fn(std::vector<Value>& args) {
        if (args.size() < 2) return 0;
        int min_v = toInt(args[0]);
        int max_v = toInt(args[1]);
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_v, max_v);
        return dis(gen);
    }

    Value sign_fn(std::vector<Value>& args) {
        if (args.empty()) return 0;
        double val = toDouble(args[0]);
        if (val > 0) return 1;
        if (val < 0) return -1;
        return 0;
    }

    Value lerp_fn(std::vector<Value>& args) {
        if (args.size() < 3) return 0.0;
        double a = toDouble(args[0]);
        double b = toDouble(args[1]);
        double t = toDouble(args[2]);
        return a + (b - a) * t;
    }

    Value map_range_fn(std::vector<Value>& args) {
        if (args.size() < 5) return 0.0;
        double val = toDouble(args[0]);
        double inMin = toDouble(args[1]);
        double inMax = toDouble(args[2]);
        double outMin = toDouble(args[3]);
        double outMax = toDouble(args[4]);
        return outMin + (val - inMin) * (outMax - outMin) / (inMax - inMin);
    }

    Value is_nan_fn(std::vector<Value>& args) {
        if (args.empty()) return false;
        return std::isnan(toDouble(args[0]));
    }

    Value is_inf_fn(std::vector<Value>& args) {
        if (args.empty()) return false;
        return std::isinf(toDouble(args[0]));
    }

    Value gcd_fn(std::vector<Value>& args) {
        if (args.size() < 2) return 0;
        int a = std::abs(toInt(args[0]));
        int b = std::abs(toInt(args[1]));
        while (b != 0) {
            int t = b;
            b = a % b;
            a = t;
        }
        return a;
    }

    Value lcm_fn(std::vector<Value>& args) {
        if (args.size() < 2) return 0;
        int a = std::abs(toInt(args[0]));
        int b = std::abs(toInt(args[1]));
        if (a == 0 || b == 0) return 0;
        // Compute GCD first
        int ga = a, gb = b;
        while (gb != 0) {
            int t = gb;
            gb = ga % gb;
            ga = t;
        }
        return (a / ga) * b;
    }

    Value factorial_fn(std::vector<Value>& args) {
        if (args.empty()) return 1;
        int n = toInt(args[0]);
        if (n < 0) throw std::runtime_error("factorial: negative argument.");
        if (n <= 1) return 1;
        int result = 1;
        for (int i = 2; i <= n; ++i) result *= i;
        return result;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["math_abs"] = NativeFunction{abs_fn, 1};
        globals["math_sqrt"] = NativeFunction{sqrt_fn, 1};
        globals["math_pow"] = NativeFunction{pow_fn, 2};
        globals["math_sin"] = NativeFunction{sin_fn, 1};
        globals["math_cos"] = NativeFunction{cos_fn, 1};
        globals["math_tan"] = NativeFunction{tan_fn, 1};
        globals["math_asin"] = NativeFunction{asin_fn, 1};
        globals["math_acos"] = NativeFunction{acos_fn, 1};
        globals["math_atan"] = NativeFunction{atan_fn, 1};
        globals["math_atan2"] = NativeFunction{atan2_fn, 2};
        globals["math_log"] = NativeFunction{log_fn, 1};
        globals["math_log10"] = NativeFunction{log10_fn, 1};
        globals["math_exp"] = NativeFunction{exp_fn, 1};
        globals["math_floor"] = NativeFunction{floor_fn, 1};
        globals["math_ceil"] = NativeFunction{ceil_fn, 1};
        globals["math_round"] = NativeFunction{round_fn, 1};
        globals["math_min"] = NativeFunction{min_fn, 2};
        globals["math_max"] = NativeFunction{max_fn, 2};
        globals["math_clamp"] = NativeFunction{clamp_fn, 3};
        globals["math_random"] = NativeFunction{random_fn, 0};
        globals["math_random_int"] = NativeFunction{random_int_fn, 2};
        globals["math_sign"] = NativeFunction{sign_fn, 1};
        globals["math_lerp"] = NativeFunction{lerp_fn, 3};
        globals["math_map_range"] = NativeFunction{map_range_fn, 5};
        globals["math_is_nan"] = NativeFunction{is_nan_fn, 1};
        globals["math_is_inf"] = NativeFunction{is_inf_fn, 1};
        globals["math_gcd"] = NativeFunction{gcd_fn, 2};
        globals["math_lcm"] = NativeFunction{lcm_fn, 2};
        globals["math_factorial"] = NativeFunction{factorial_fn, 1};
        globals["math_PI"] = 3.14159265358979323846;
        globals["math_E"] = 2.71828182845904523536;
        globals["math_INF"] = std::numeric_limits<double>::infinity();
        globals["math_NAN"] = std::numeric_limits<double>::quiet_NaN();
    }
}

// ============ STRING LIBRARY ============
namespace String {
    Value length(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return 0;
        return static_cast<int>(args[0].get<std::string>().length());
    }

    Value upper(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    Value lower(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    Value trim(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), str.end());
        return str;
    }

    Value split(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return std::vector<Value>();
        std::string str = args[0].get<std::string>();
        std::string delimiter = args[1].get<std::string>();
        std::vector<Value> result;
        if (delimiter.empty()) {
            for (char c : str) result.push_back(Value(std::string(1, c)));
            return result;
        }
        size_t pos = 0;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            result.push_back(Value(str.substr(0, pos)));
            str.erase(0, pos + delimiter.length());
        }
        result.push_back(Value(str));
        return result;
    }

    Value join(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>() || !args[1].holds_alternative<std::string>())
            return std::string("");
        const auto& vec = args[0].get<std::vector<Value>>();
        const std::string& separator = args[1].get<std::string>();
        std::string result;
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) result += separator;
            if (vec[i].holds_alternative<std::string>()) result += vec[i].get<std::string>();
            else if (vec[i].holds_alternative<int>()) result += std::to_string(vec[i].get<int>());
            else if (vec[i].holds_alternative<double>()) result += std::to_string(vec[i].get<double>());
            else if (vec[i].holds_alternative<bool>()) result += vec[i].get<bool>() ? "true" : "false";
        }
        return result;
    }

    Value substring(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        int start = toInt(args[1]);
        int len = toInt(args[2]);
        if (start < 0 || start >= static_cast<int>(str.length())) return std::string("");
        return str.substr(start, len);
    }

    Value contains(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        return args[0].get<std::string>().find(args[1].get<std::string>()) != std::string::npos;
    }

    Value replace(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>() ||
            !args[1].holds_alternative<std::string>() || !args[2].holds_alternative<std::string>())
            return std::string("");
        std::string str = args[0].get<std::string>();
        const std::string& from = args[1].get<std::string>();
        const std::string& to = args[2].get<std::string>();
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
        return str;
    }

    Value starts_with(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        const auto& str = args[0].get<std::string>();
        const auto& prefix = args[1].get<std::string>();
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    Value ends_with(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        const auto& str = args[0].get<std::string>();
        const auto& suffix = args[1].get<std::string>();
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    Value index_of(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return -1;
        size_t pos = args[0].get<std::string>().find(args[1].get<std::string>());
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }

    Value repeat(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        int count = toInt(args[1]);
        std::string result;
        for (int i = 0; i < count; ++i) result += str;
        return result;
    }

    Value char_at(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>()) return std::string("");
        const auto& str = args[0].get<std::string>();
        int idx = toInt(args[1]);
        if (idx < 0 || idx >= static_cast<int>(str.size())) return std::string("");
        return std::string(1, str[idx]);
    }

    Value to_chars(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::vector<Value>();
        const auto& str = args[0].get<std::string>();
        std::vector<Value> result;
        for (char c : str) result.push_back(Value(std::string(1, c)));
        return result;
    }

    Value pad_left(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>() || !args[2].holds_alternative<std::string>())
            return std::string("");
        std::string str = args[0].get<std::string>();
        int len = toInt(args[1]);
        std::string pad = args[2].get<std::string>();
        if (pad.empty()) return str;
        while (static_cast<int>(str.length()) < len) {
            str = pad + str;
        }
        if (static_cast<int>(str.length()) > len) {
            str = str.substr(str.length() - len);
        }
        return str;
    }

    Value pad_right(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>() || !args[2].holds_alternative<std::string>())
            return std::string("");
        std::string str = args[0].get<std::string>();
        int len = toInt(args[1]);
        std::string pad = args[2].get<std::string>();
        if (pad.empty()) return str;
        while (static_cast<int>(str.length()) < len) {
            str = str + pad;
        }
        if (static_cast<int>(str.length()) > len) {
            str = str.substr(0, len);
        }
        return str;
    }

    Value reverse_str(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        std::reverse(str.begin(), str.end());
        return str;
    }

    Value count(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return 0;
        const std::string& str = args[0].get<std::string>();
        const std::string& substr = args[1].get<std::string>();
        if (substr.empty()) return 0;
        int count = 0;
        size_t pos = 0;
        while ((pos = str.find(substr, pos)) != std::string::npos) {
            ++count;
            pos += substr.length();
        }
        return count;
    }

    Value is_empty(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return true;
        return args[0].get<std::string>().empty();
    }

    Value is_numeric(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        const std::string& str = args[0].get<std::string>();
        if (str.empty()) return false;
        bool has_dot = false;
        for (size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (c == '.') {
                if (has_dot) return false;
                has_dot = true;
            } else if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        return true;
    }

    Value to_bytes(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::vector<Value>();
        const std::string& str = args[0].get<std::string>();
        std::vector<Value> result;
        for (unsigned char c : str) result.push_back(Value(static_cast<int>(c)));
        return result;
    }

    Value from_bytes(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return std::string("");
        const auto& vec = args[0].get<std::vector<Value>>();
        std::string result;
        for (const auto& v : vec) {
            result += static_cast<char>(toInt(v));
        }
        return result;
    }

    Value char_code(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return 0;
        const std::string& str = args[0].get<std::string>();
        if (str.empty()) return 0;
        return static_cast<int>(static_cast<unsigned char>(str[0]));
    }

    Value from_char_code(std::vector<Value>& args) {
        if (args.empty()) return std::string("");
        int code = toInt(args[0]);
        return std::string(1, static_cast<char>(code));
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["str_length"] = NativeFunction{length, 1};
        globals["str_upper"] = NativeFunction{upper, 1};
        globals["str_lower"] = NativeFunction{lower, 1};
        globals["str_trim"] = NativeFunction{trim, 1};
        globals["str_split"] = NativeFunction{split, 2};
        globals["str_join"] = NativeFunction{join, 2};
        globals["str_substring"] = NativeFunction{substring, 3};
        globals["str_contains"] = NativeFunction{contains, 2};
        globals["str_replace"] = NativeFunction{replace, 3};
        globals["str_starts_with"] = NativeFunction{starts_with, 2};
        globals["str_ends_with"] = NativeFunction{ends_with, 2};
        globals["str_index_of"] = NativeFunction{index_of, 2};
        globals["str_repeat"] = NativeFunction{repeat, 2};
        globals["str_char_at"] = NativeFunction{char_at, 2};
        globals["str_to_chars"] = NativeFunction{to_chars, 1};
        globals["str_pad_left"] = NativeFunction{pad_left, 3};
        globals["str_pad_right"] = NativeFunction{pad_right, 3};
        globals["str_reverse"] = NativeFunction{reverse_str, 1};
        globals["str_count"] = NativeFunction{count, 2};
        globals["str_is_empty"] = NativeFunction{is_empty, 1};
        globals["str_is_numeric"] = NativeFunction{is_numeric, 1};
        globals["str_to_bytes"] = NativeFunction{to_bytes, 1};
        globals["str_from_bytes"] = NativeFunction{from_bytes, 1};
        globals["str_char_code"] = NativeFunction{char_code, 1};
        globals["str_from_char_code"] = NativeFunction{from_char_code, 1};
    }
}

// ============ COLLECTIONS LIBRARY ============
namespace Collections {
    Value push(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        auto vec = args[0].get<std::vector<Value>>();
        vec.push_back(args[1]);
        return vec;
    }

    Value pop(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        auto vec = args[0].get<std::vector<Value>>();
        if (!vec.empty()) vec.pop_back();
        return vec;
    }

    Value length(std::vector<Value>& args) {
        if (args.empty()) return 0;
        if (args[0].holds_alternative<std::vector<Value>>()) return static_cast<int>(args[0].get<std::vector<Value>>().size());
        if (args[0].holds_alternative<std::string>()) return static_cast<int>(args[0].get<std::string>().size());
        if (args[0].holds_alternative<std::unordered_map<std::string, Value>>()) return static_cast<int>(args[0].get<std::unordered_map<std::string, Value>>().size());
        return 0;
    }

    Value reverse(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        auto vec = args[0].get<std::vector<Value>>();
        std::reverse(vec.begin(), vec.end());
        return vec;
    }

    Value sort(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        auto vec = args[0].get<std::vector<Value>>();
        std::sort(vec.begin(), vec.end(), [](const Value& a, const Value& b) { return a < b; });
        return vec;
    }

    Value slice(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        const auto& vec = args[0].get<std::vector<Value>>();
        int start = toInt(args[1]);
        int end = toInt(args[2]);
        if (start < 0) start = 0;
        if (end > static_cast<int>(vec.size())) end = static_cast<int>(vec.size());
        if (start >= end) return std::vector<Value>();
        return std::vector<Value>(vec.begin() + start, vec.begin() + end);
    }

    Value index_of(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>()) return -1;
        const auto& vec = args[0].get<std::vector<Value>>();
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] == args[1]) return static_cast<int>(i);
        }
        return -1;
    }

    Value contains_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>()) return false;
        const auto& vec = args[0].get<std::vector<Value>>();
        for (const auto& item : vec) {
            if (item == args[1]) return true;
        }
        return false;
    }

    Value flatten(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        const auto& vec = args[0].get<std::vector<Value>>();
        std::vector<Value> result;
        for (const auto& item : vec) {
            if (item.holds_alternative<std::vector<Value>>()) {
                const auto& inner = item.get<std::vector<Value>>();
                result.insert(result.end(), inner.begin(), inner.end());
            } else {
                result.push_back(item);
            }
        }
        return result;
    }

    Value unique(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        const auto& vec = args[0].get<std::vector<Value>>();
        std::vector<Value> result;
        for (const auto& item : vec) {
            bool found = false;
            for (const auto& r : result) {
                if (r == item) { found = true; break; }
            }
            if (!found) result.push_back(item);
        }
        return result;
    }

    Value sum(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return 0;
        const auto& vec = args[0].get<std::vector<Value>>();
        double total = 0.0;
        bool all_int = true;
        for (const auto& item : vec) {
            total += toDouble(item);
            if (!item.holds_alternative<int>()) all_int = false;
        }
        if (all_int) return static_cast<int>(total);
        return total;
    }

    Value min_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return Value();
        const auto& vec = args[0].get<std::vector<Value>>();
        if (vec.empty()) return Value();
        double minVal = toDouble(vec[0]);
        size_t minIdx = 0;
        for (size_t i = 1; i < vec.size(); ++i) {
            double val = toDouble(vec[i]);
            if (val < minVal) {
                minVal = val;
                minIdx = i;
            }
        }
        return vec[minIdx];
    }

    Value max_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return Value();
        const auto& vec = args[0].get<std::vector<Value>>();
        if (vec.empty()) return Value();
        double maxVal = toDouble(vec[0]);
        size_t maxIdx = 0;
        for (size_t i = 1; i < vec.size(); ++i) {
            double val = toDouble(vec[i]);
            if (val > maxVal) {
                maxVal = val;
                maxIdx = i;
            }
        }
        return vec[maxIdx];
    }

    Value fill(std::vector<Value>& args) {
        if (args.size() < 2) return std::vector<Value>();
        int count = toInt(args[0]);
        std::vector<Value> result;
        for (int i = 0; i < count; ++i) result.push_back(args[1]);
        return result;
    }

    Value range(std::vector<Value>& args) {
        if (args.size() < 2) return std::vector<Value>();
        int start = toInt(args[0]);
        int end = toInt(args[1]);
        int step = (args.size() >= 3) ? toInt(args[2]) : 1;
        if (step == 0) throw std::runtime_error("list_range: step cannot be zero.");
        std::vector<Value> result;
        if (step > 0) {
            for (int i = start; i < end; i += step) result.push_back(Value(i));
        } else {
            for (int i = start; i > end; i += step) result.push_back(Value(i));
        }
        return result;
    }

    Value chunk(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>()) return std::vector<Value>();
        const auto& vec = args[0].get<std::vector<Value>>();
        int size = toInt(args[1]);
        if (size <= 0) throw std::runtime_error("list_chunk: size must be positive.");
        std::vector<Value> result;
        for (size_t i = 0; i < vec.size(); i += size) {
            size_t end = std::min(i + static_cast<size_t>(size), vec.size());
            std::vector<Value> chunkVec(vec.begin() + i, vec.begin() + end);
            result.push_back(Value(chunkVec));
        }
        return result;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["list_push"] = NativeFunction{push, 2};
        globals["list_pop"] = NativeFunction{pop, 1};
        globals["list_length"] = NativeFunction{length, 1};
        globals["list_reverse"] = NativeFunction{reverse, 1};
        globals["list_sort"] = NativeFunction{sort, 1};
        globals["list_slice"] = NativeFunction{slice, 3};
        globals["list_index_of"] = NativeFunction{index_of, 2};
        globals["list_contains"] = NativeFunction{contains_fn, 2};
        globals["list_flatten"] = NativeFunction{flatten, 1};
        globals["list_unique"] = NativeFunction{unique, 1};
        globals["list_sum"] = NativeFunction{sum, 1};
        globals["list_min"] = NativeFunction{min_fn, 1};
        globals["list_max"] = NativeFunction{max_fn, 1};
        globals["list_fill"] = NativeFunction{fill, 2};
        globals["list_range"] = NativeFunction{range, -1};
        globals["list_chunk"] = NativeFunction{chunk, 2};
    }
}

// ============ MAP LIBRARY ============
namespace Map {
    Value keys(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::unordered_map<std::string, Value>>())
            return std::vector<Value>();
        const auto& map = args[0].get<std::unordered_map<std::string, Value>>();
        std::vector<Value> result;
        for (const auto& [key, _] : map) result.push_back(Value(key));
        return result;
    }

    Value values(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::unordered_map<std::string, Value>>())
            return std::vector<Value>();
        const auto& map = args[0].get<std::unordered_map<std::string, Value>>();
        std::vector<Value> result;
        for (const auto& [_, val] : map) result.push_back(val);
        return result;
    }

    Value has(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>()) return false;
        const auto& map = args[0].get<std::unordered_map<std::string, Value>>();
        return map.count(args[1].get<std::string>()) > 0;
    }

    Value get_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>()) return Value();
        const auto& map = args[0].get<std::unordered_map<std::string, Value>>();
        const auto& key = args[1].get<std::string>();
        auto it = map.find(key);
        if (it != map.end()) return it->second;
        // Return default value if provided
        if (args.size() >= 3) return args[2];
        return Value();
    }

    Value set_fn(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>()) return args.empty() ? Value() : args[0];
        auto& map = const_cast<std::unordered_map<std::string, Value>&>(args[0].get<std::unordered_map<std::string, Value>>());
        map[args[1].get<std::string>()] = args[2];
        return Value(); // Modified in-place via write-back
    }

    Value remove_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>()) return args.empty() ? Value() : args[0];
        auto& map = const_cast<std::unordered_map<std::string, Value>&>(args[0].get<std::unordered_map<std::string, Value>>());
        map.erase(args[1].get<std::string>());
        return Value(); // Modified in-place via write-back
    }

    Value size(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::unordered_map<std::string, Value>>()) return 0;
        return static_cast<int>(args[0].get<std::unordered_map<std::string, Value>>().size());
    }

    Value merge(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::unordered_map<std::string, Value>>()) return args.empty() ? Value() : args[0];
        auto result = args[0].get<std::unordered_map<std::string, Value>>();
        const auto& other = args[1].get<std::unordered_map<std::string, Value>>();
        for (const auto& [key, val] : other) result[key] = val;
        return result;
    }

    Value entries(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::unordered_map<std::string, Value>>())
            return std::vector<Value>();
        const auto& map = args[0].get<std::unordered_map<std::string, Value>>();
        std::vector<Value> result;
        for (const auto& [key, val] : map) {
            std::vector<Value> pair;
            pair.push_back(Value(key));
            pair.push_back(val);
            result.push_back(Value(pair));
        }
        return result;
    }

    Value from_entries(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>())
            return std::unordered_map<std::string, Value>();
        const auto& list = args[0].get<std::vector<Value>>();
        std::unordered_map<std::string, Value> result;
        for (const auto& item : list) {
            if (!item.holds_alternative<std::vector<Value>>()) continue;
            const auto& pair = item.get<std::vector<Value>>();
            if (pair.size() < 2 || !pair[0].holds_alternative<std::string>()) continue;
            result[pair[0].get<std::string>()] = pair[1];
        }
        return result;
    }

    Value invert(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::unordered_map<std::string, Value>>())
            return std::unordered_map<std::string, Value>();
        const auto& map = args[0].get<std::unordered_map<std::string, Value>>();
        std::unordered_map<std::string, Value> result;
        for (const auto& [key, val] : map) {
            if (!val.holds_alternative<std::string>())
                throw std::runtime_error("map_invert: all values must be strings.");
            result[val.get<std::string>()] = Value(key);
        }
        return result;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["map_keys"] = NativeFunction{keys, 1};
        globals["map_values"] = NativeFunction{values, 1};
        globals["map_has"] = NativeFunction{has, 2};
        globals["map_get"] = NativeFunction{get_fn, -1};
        globals["map_set"] = NativeFunction{set_fn, 3};
        globals["map_remove"] = NativeFunction{remove_fn, 2};
        globals["map_size"] = NativeFunction{size, 1};
        globals["map_merge"] = NativeFunction{merge, 2};
        globals["map_entries"] = NativeFunction{entries, 1};
        globals["map_from_entries"] = NativeFunction{from_entries, 1};
        globals["map_invert"] = NativeFunction{invert, 1};
    }
}

// ============ JSON LIBRARY ============
namespace Json {
    // Forward declaration for recursive use
    static std::string valueToJson(const Value& val);

    static std::string escapeJsonString(const std::string& str) {
        std::string result;
        result.reserve(str.size() + 2);
        result += '"';
        for (char c : str) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        result += '"';
        return result;
    }

    static std::string valueToJson(const Value& val) {
        if (val.holds_alternative<std::monostate>()) return "null";
        if (val.holds_alternative<int>()) return std::to_string(val.get<int>());
        if (val.holds_alternative<double>()) {
            std::ostringstream oss;
            oss << val.get<double>();
            return oss.str();
        }
        if (val.holds_alternative<float>()) {
            std::ostringstream oss;
            oss << static_cast<double>(val.get<float>());
            return oss.str();
        }
        if (val.holds_alternative<bool>()) return val.get<bool>() ? "true" : "false";
        if (val.holds_alternative<std::string>()) return escapeJsonString(val.get<std::string>());
        if (val.holds_alternative<std::vector<Value>>()) {
            const auto& vec = val.get<std::vector<Value>>();
            std::string result = "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i > 0) result += ",";
                result += valueToJson(vec[i]);
            }
            result += "]";
            return result;
        }
        if (val.holds_alternative<std::unordered_map<std::string, Value>>()) {
            const auto& map = val.get<std::unordered_map<std::string, Value>>();
            std::string result = "{";
            bool first = true;
            for (const auto& [key, v] : map) {
                if (!first) result += ",";
                first = false;
                result += escapeJsonString(key) + ":" + valueToJson(v);
            }
            result += "}";
            return result;
        }
        return "null";
    }

    Value to_json(std::vector<Value>& args) {
        if (args.empty()) return std::string("null");
        return valueToJson(args[0]);
    }

    // Simple recursive descent JSON parser
    struct JsonParser {
        const std::string& input;
        size_t pos;

        JsonParser(const std::string& s) : input(s), pos(0) {}

        void skipWhitespace() {
            while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) ++pos;
        }

        char peek() {
            skipWhitespace();
            if (pos >= input.size()) throw std::runtime_error("from_json: unexpected end of input.");
            return input[pos];
        }

        char advance() {
            skipWhitespace();
            if (pos >= input.size()) throw std::runtime_error("from_json: unexpected end of input.");
            return input[pos++];
        }

        bool match(const std::string& expected) {
            skipWhitespace();
            if (input.compare(pos, expected.size(), expected) == 0) {
                pos += expected.size();
                return true;
            }
            return false;
        }

        Value parseValue() {
            skipWhitespace();
            if (pos >= input.size()) throw std::runtime_error("from_json: unexpected end of input.");
            char c = input[pos];
            if (c == '"') return parseString();
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') return parseNull();
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();
            throw std::runtime_error("from_json: unexpected character '" + std::string(1, c) + "'.");
        }

        Value parseNull() {
            if (!match("null")) throw std::runtime_error("from_json: expected 'null'.");
            return Value();
        }

        Value parseBool() {
            if (match("true")) return Value(true);
            if (match("false")) return Value(false);
            throw std::runtime_error("from_json: expected boolean.");
        }

        Value parseNumber() {
            skipWhitespace();
            size_t start = pos;
            bool is_float = false;
            if (pos < input.size() && input[pos] == '-') ++pos;
            while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) ++pos;
            if (pos < input.size() && input[pos] == '.') {
                is_float = true;
                ++pos;
                while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) ++pos;
            }
            if (pos < input.size() && (input[pos] == 'e' || input[pos] == 'E')) {
                is_float = true;
                ++pos;
                if (pos < input.size() && (input[pos] == '+' || input[pos] == '-')) ++pos;
                while (pos < input.size() && std::isdigit(static_cast<unsigned char>(input[pos]))) ++pos;
            }
            std::string numStr = input.substr(start, pos - start);
            if (is_float) return Value(std::stod(numStr));
            return Value(std::stoi(numStr));
        }

        std::string parseRawString() {
            if (advance() != '"') throw std::runtime_error("from_json: expected '\"'.");
            std::string result;
            while (pos < input.size() && input[pos] != '"') {
                if (input[pos] == '\\') {
                    ++pos;
                    if (pos >= input.size()) throw std::runtime_error("from_json: unexpected end in string escape.");
                    switch (input[pos]) {
                        case '"':  result += '"'; break;
                        case '\\': result += '\\'; break;
                        case '/':  result += '/'; break;
                        case 'b':  result += '\b'; break;
                        case 'f':  result += '\f'; break;
                        case 'n':  result += '\n'; break;
                        case 'r':  result += '\r'; break;
                        case 't':  result += '\t'; break;
                        default:   result += input[pos]; break;
                    }
                } else {
                    result += input[pos];
                }
                ++pos;
            }
            if (pos >= input.size()) throw std::runtime_error("from_json: unterminated string.");
            ++pos; // skip closing '"'
            return result;
        }

        Value parseString() {
            return Value(parseRawString());
        }

        Value parseArray() {
            advance(); // skip '['
            std::vector<Value> result;
            skipWhitespace();
            if (pos < input.size() && input[pos] == ']') { ++pos; return Value(result); }
            result.push_back(parseValue());
            while (pos < input.size()) {
                skipWhitespace();
                if (pos < input.size() && input[pos] == ']') { ++pos; return Value(result); }
                if (advance() != ',') throw std::runtime_error("from_json: expected ',' or ']' in array.");
                result.push_back(parseValue());
            }
            throw std::runtime_error("from_json: unterminated array.");
        }

        Value parseObject() {
            advance(); // skip '{'
            std::unordered_map<std::string, Value> result;
            skipWhitespace();
            if (pos < input.size() && input[pos] == '}') { ++pos; return Value(result); }
            std::string key = parseRawString();
            skipWhitespace();
            if (advance() != ':') throw std::runtime_error("from_json: expected ':' in object.");
            result[key] = parseValue();
            while (pos < input.size()) {
                skipWhitespace();
                if (pos < input.size() && input[pos] == '}') { ++pos; return Value(result); }
                if (advance() != ',') throw std::runtime_error("from_json: expected ',' or '}' in object.");
                key = parseRawString();
                skipWhitespace();
                if (advance() != ':') throw std::runtime_error("from_json: expected ':' in object.");
                result[key] = parseValue();
            }
            throw std::runtime_error("from_json: unterminated object.");
        }
    };

    Value from_json(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("from_json: expected string argument.");
        JsonParser parser(args[0].get<std::string>());
        return parser.parseValue();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["json_to_string"] = NativeFunction{to_json, 1};
        globals["json_from_string"] = NativeFunction{from_json, 1};
    }
}

// ============ UTILITY LIBRARY ============
namespace Utility {
    // Helper to produce a debug string representation of a value
    static std::string debugRepr(const Value& val) {
        if (val.holds_alternative<std::monostate>()) return "null";
        if (val.holds_alternative<int>()) return std::to_string(val.get<int>());
        if (val.holds_alternative<double>()) return std::to_string(val.get<double>());
        if (val.holds_alternative<float>()) return std::to_string(val.get<float>());
        if (val.holds_alternative<bool>()) return val.get<bool>() ? "true" : "false";
        if (val.holds_alternative<std::string>()) return "\"" + val.get<std::string>() + "\"";
        if (val.holds_alternative<std::vector<Value>>()) {
            const auto& vec = val.get<std::vector<Value>>();
            std::string result = "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i > 0) result += ", ";
                result += debugRepr(vec[i]);
            }
            result += "]";
            return result;
        }
        if (val.holds_alternative<std::unordered_map<std::string, Value>>()) {
            const auto& map = val.get<std::unordered_map<std::string, Value>>();
            std::string result = "{";
            bool first = true;
            for (const auto& [key, v] : map) {
                if (!first) result += ", ";
                first = false;
                result += "\"" + key + "\": " + debugRepr(v);
            }
            result += "}";
            return result;
        }
        return "<value>";
    }

    Value assert_eq(std::vector<Value>& args) {
        if (args.size() < 2)
            throw std::runtime_error("assert_eq: requires two arguments.");
        if (!(args[0] == args[1])) {
            throw std::runtime_error("assert_eq failed: " + debugRepr(args[0]) + " != " + debugRepr(args[1]));
        }
        return Value();
    }

    Value assert_ne(std::vector<Value>& args) {
        if (args.size() < 2)
            throw std::runtime_error("assert_ne: requires two arguments.");
        if (args[0] == args[1]) {
            throw std::runtime_error("assert_ne failed: " + debugRepr(args[0]) + " == " + debugRepr(args[1]));
        }
        return Value();
    }

    Value todo_fn(std::vector<Value>& args) {
        throw std::runtime_error("not yet implemented");
    }

    Value dbg(std::vector<Value>& args) {
        if (args.empty()) {
            std::cerr << "[dbg] null" << std::endl;
            return Value();
        }
        std::cerr << "[dbg] " << debugRepr(args[0]) << std::endl;
        return args[0];
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["assert_eq"] = NativeFunction{assert_eq, 2};
        globals["assert_ne"] = NativeFunction{assert_ne, 2};
        globals["todo"] = NativeFunction{todo_fn, 0};
        globals["dbg"] = NativeFunction{dbg, 1};
    }
}

// ============ IO LIBRARY ============
namespace IO {
    Value readFile(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::ifstream file(args[0].get<std::string>());
        if (!file.is_open()) return std::string("");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    Value writeFile(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        std::ofstream file(args[0].get<std::string>());
        if (!file.is_open()) return false;
        file << args[1].get<std::string>();
        return true;
    }

    Value appendFile(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        std::ofstream file(args[0].get<std::string>(), std::ios::app);
        if (!file.is_open()) return false;
        file << args[1].get<std::string>();
        return true;
    }

    Value readLines(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::vector<Value>();
        std::ifstream file(args[0].get<std::string>());
        if (!file.is_open()) return std::vector<Value>();
        std::vector<Value> lines;
        std::string line;
        while (std::getline(file, line)) lines.push_back(Value(line));
        return lines;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["io_read_file"] = NativeFunction{readFile, 1};
        globals["io_write_file"] = NativeFunction{writeFile, 2};
        globals["io_append_file"] = NativeFunction{appendFile, 2};
        globals["io_read_lines"] = NativeFunction{readLines, 1};
    }
}

// ============ FS LIBRARY ============
namespace FS {
    Value fileExists(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        return std::filesystem::exists(args[0].get<std::string>());
    }

    Value isDirectory(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        return std::filesystem::is_directory(args[0].get<std::string>());
    }

    Value isFile(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        return std::filesystem::is_regular_file(args[0].get<std::string>());
    }

    Value createDirectory(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        try { return std::filesystem::create_directories(args[0].get<std::string>()); }
        catch (...) { return false; }
    }

    Value removeFile(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        try { return std::filesystem::remove(args[0].get<std::string>()); }
        catch (...) { return false; }
    }

    Value fileSize(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return 0;
        try { return static_cast<int>(std::filesystem::file_size(args[0].get<std::string>())); }
        catch (...) { return 0; }
    }

    Value listDir(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::vector<Value>();
        std::vector<Value> result;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(args[0].get<std::string>())) {
                result.push_back(Value(entry.path().string()));
            }
        } catch (...) {}
        return result;
    }

    Value absPath(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        try { return std::filesystem::absolute(args[0].get<std::string>()).string(); }
        catch (...) { return std::string(""); }
    }

    Value copyFile(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        try {
            std::filesystem::copy(args[0].get<std::string>(), args[1].get<std::string>(),
                                  std::filesystem::copy_options::overwrite_existing);
            return true;
        } catch (...) { return false; }
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["fs_exists"] = NativeFunction{fileExists, 1};
        globals["fs_is_directory"] = NativeFunction{isDirectory, 1};
        globals["fs_is_file"] = NativeFunction{isFile, 1};
        globals["fs_create_dir"] = NativeFunction{createDirectory, 1};
        globals["fs_remove"] = NativeFunction{removeFile, 1};
        globals["fs_file_size"] = NativeFunction{fileSize, 1};
        globals["fs_list_dir"] = NativeFunction{listDir, 1};
        globals["fs_abs_path"] = NativeFunction{absPath, 1};
        globals["fs_copy"] = NativeFunction{copyFile, 2};
    }
}

// ============ TIME LIBRARY ============
namespace Time {
    Value now(std::vector<Value>& args) {
        auto now_tp = std::chrono::system_clock::now();
        auto duration = now_tp.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return static_cast<double>(millis);
    }

    Value sleep_fn(std::vector<Value>& args) {
        if (args.empty()) return Value();
        int ms = toInt(args[0]);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return Value();
    }

    Value timestamp(std::vector<Value>& args) {
        auto now_tp = std::chrono::system_clock::now();
        auto duration = now_tp.time_since_epoch();
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        return static_cast<int>(secs);
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["time_now"] = NativeFunction{now, 0};
        globals["time_sleep"] = NativeFunction{sleep_fn, 1};
        globals["time_timestamp"] = NativeFunction{timestamp, 0};
    }
}

// ============ CRYPTO LIBRARY ============
namespace Crypto {
    Value xor_cipher(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return std::string("");
        std::string data = args[0].get<std::string>();
        std::string key = args[1].get<std::string>();
        if (key.empty()) return data;
        std::string result = data;
        for (size_t i = 0; i < data.size(); ++i) result[i] = data[i] ^ key[i % key.size()];
        return result;
    }

    Value simple_hash(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return 0;
        std::string str = args[0].get<std::string>();
        size_t hash = 0;
        for (char c : str) hash = hash * 31 + static_cast<size_t>(c);
        return static_cast<int>(hash % 1000000007);
    }

    Value random_bytes(std::vector<Value>& args) {
        int count = 16;
        if (!args.empty() && args[0].holds_alternative<int>()) count = args[0].get<int>();
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 255);
        std::string result;
        for (int i = 0; i < count; ++i) result += static_cast<char>(dis(gen));
        return result;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["crypto_xor"] = NativeFunction{xor_cipher, 2};
        globals["crypto_hash"] = NativeFunction{simple_hash, 1};
        globals["crypto_random_bytes"] = NativeFunction{random_bytes, 1};
    }
}

// ============ ENCODING LIBRARY ============
namespace Encoding {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    Value base64_encode(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const std::string& input = args[0].get<std::string>();
        std::string ret;
        int i = 0;
        unsigned char char_array_3[3], char_array_4[4];
        for (char c : input) {
            char_array_3[i++] = c;
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                for (i = 0; i < 4; i++) ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        if (i) {
            for (int j = i; j < 3; j++) char_array_3[j] = '\0';
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            for (int j = 0; j < i + 1; j++) ret += base64_chars[char_array_4[j]];
            while (i++ < 3) ret += '=';
        }
        return ret;
    }

    Value base64_decode(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const std::string& input = args[0].get<std::string>();
        std::string ret;
        int i = 0;
        unsigned char char_array_4[4], char_array_3[3];
        for (char c : input) {
            if (c == '=') break;
            size_t pos = base64_chars.find(c);
            if (pos == std::string::npos) continue;
            char_array_4[i++] = static_cast<unsigned char>(pos);
            if (i == 4) {
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                for (i = 0; i < 3; i++) ret += static_cast<char>(char_array_3[i]);
                i = 0;
            }
        }
        if (i) {
            for (int j = i; j < 4; j++) char_array_4[j] = 0;
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            for (int j = 0; j < i - 1; j++) ret += static_cast<char>(char_array_3[j]);
        }
        return ret;
    }

    Value hex_encode(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const std::string& input = args[0].get<std::string>();
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (unsigned char c : input) ss << std::setw(2) << static_cast<int>(c);
        return ss.str();
    }

    Value hex_decode(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const std::string& input = args[0].get<std::string>();
        std::string result;
        for (size_t i = 0; i + 1 < input.size(); i += 2) {
            std::string byte = input.substr(i, 2);
            result += static_cast<char>(std::stoi(byte, nullptr, 16));
        }
        return result;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["encoding_base64_encode"] = NativeFunction{base64_encode, 1};
        globals["encoding_base64_decode"] = NativeFunction{base64_decode, 1};
        globals["encoding_hex_encode"] = NativeFunction{hex_encode, 1};
        globals["encoding_hex_decode"] = NativeFunction{hex_decode, 1};
    }
}

// ============ LOG LIBRARY ============
namespace Log {
    Value info(std::vector<Value>& args) {
        std::cout << "[INFO] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) std::cout << arg.get<std::string>();
            else if (arg.holds_alternative<int>()) std::cout << arg.get<int>();
            else if (arg.holds_alternative<double>()) std::cout << arg.get<double>();
            else if (arg.holds_alternative<bool>()) std::cout << (arg.get<bool>() ? "true" : "false");
        }
        std::cout << std::endl;
        return Value();
    }

    Value warn(std::vector<Value>& args) {
        std::cout << "[WARN] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) std::cout << arg.get<std::string>();
            else if (arg.holds_alternative<int>()) std::cout << arg.get<int>();
            else if (arg.holds_alternative<double>()) std::cout << arg.get<double>();
            else if (arg.holds_alternative<bool>()) std::cout << (arg.get<bool>() ? "true" : "false");
        }
        std::cout << std::endl;
        return Value();
    }

    Value error(std::vector<Value>& args) {
        std::cerr << "[ERROR] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) std::cerr << arg.get<std::string>();
            else if (arg.holds_alternative<int>()) std::cerr << arg.get<int>();
            else if (arg.holds_alternative<double>()) std::cerr << arg.get<double>();
            else if (arg.holds_alternative<bool>()) std::cerr << (arg.get<bool>() ? "true" : "false");
        }
        std::cerr << std::endl;
        return Value();
    }

    Value debug(std::vector<Value>& args) {
        std::cout << "[DEBUG] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) std::cout << arg.get<std::string>();
            else if (arg.holds_alternative<int>()) std::cout << arg.get<int>();
            else if (arg.holds_alternative<double>()) std::cout << arg.get<double>();
            else if (arg.holds_alternative<bool>()) std::cout << (arg.get<bool>() ? "true" : "false");
        }
        std::cout << std::endl;
        return Value();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["log_info"] = NativeFunction{info, 1};
        globals["log_warn"] = NativeFunction{warn, 1};
        globals["log_error"] = NativeFunction{error, 1};
        globals["log_debug"] = NativeFunction{debug, 1};
    }
}

// ============ ENV LIBRARY ============
namespace Env {
    Value getEnv(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const char* value = std::getenv(args[0].get<std::string>().c_str());
        return value ? std::string(value) : std::string("");
    }

    Value setEnv(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        #ifdef _WIN32
        return _putenv_s(args[0].get<std::string>().c_str(), args[1].get<std::string>().c_str()) == 0;
        #else
        return setenv(args[0].get<std::string>().c_str(), args[1].get<std::string>().c_str(), 1) == 0;
        #endif
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["env_get"] = NativeFunction{getEnv, 1};
        globals["env_set"] = NativeFunction{setEnv, 2};
    }
}

// ============ PROCESS LIBRARY ============
namespace Process {
    Value exec(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return -1;
        return system(args[0].get<std::string>().c_str());
    }

    Value shell(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string command = args[0].get<std::string>();
        std::string result;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return std::string("");
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
        pclose(pipe);
        return result;
    }

    Value spawn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return -1;
        std::string command = args[0].get<std::string>();
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].holds_alternative<std::string>()) command += " " + args[i].get<std::string>();
        }
        return system(command.c_str());
    }

    Value cwd(std::vector<Value>& args) {
        char buffer[4096];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) return std::string(buffer);
        return std::string("");
    }

    Value chdir_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return -1;
        return chdir(args[0].get<std::string>().c_str());
    }

    Value pid_fn(std::vector<Value>& args) {
        #ifdef _WIN32
        return static_cast<int>(GetCurrentProcessId());
        #else
        return static_cast<int>(getpid());
        #endif
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["process_exec"] = NativeFunction{exec, 1};
        globals["process_shell"] = NativeFunction{shell, 1};
        globals["process_spawn"] = NativeFunction{spawn, -1};
        globals["process_cwd"] = NativeFunction{cwd, 0};
        globals["process_chdir"] = NativeFunction{chdir_fn, 1};
        globals["process_pid"] = NativeFunction{pid_fn, 0};
    }
}

// ============ PLATFORM LIBRARY ============
namespace Platform {
    Value os_name(std::vector<Value>& args) {
        #ifdef _WIN32
        return std::string("windows");
        #elif __APPLE__
        return std::string("macos");
        #elif __linux__
        return std::string("linux");
        #elif __FreeBSD__
        return std::string("freebsd");
        #else
        return std::string("unknown");
        #endif
    }

    Value arch(std::vector<Value>& args) {
        #if defined(__x86_64__) || defined(_M_X64)
        return std::string("x86_64");
        #elif defined(__aarch64__) || defined(_M_ARM64)
        return std::string("aarch64");
        #elif defined(__i386__) || defined(_M_IX86)
        return std::string("x86");
        #elif defined(__arm__) || defined(_M_ARM)
        return std::string("arm");
        #else
        return std::string("unknown");
        #endif
    }

    Value is_windows(std::vector<Value>& args) {
        #ifdef _WIN32
        return true;
        #else
        return false;
        #endif
    }

    Value is_linux(std::vector<Value>& args) {
        #ifdef __linux__
        return true;
        #else
        return false;
        #endif
    }

    Value is_macos(std::vector<Value>& args) {
        #ifdef __APPLE__
        return true;
        #else
        return false;
        #endif
    }

    Value pointer_size(std::vector<Value>& args) {
        return static_cast<int>(sizeof(void*));
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["platform_os"] = NativeFunction{os_name, 0};
        globals["platform_arch"] = NativeFunction{arch, 0};
        globals["platform_is_windows"] = NativeFunction{is_windows, 0};
        globals["platform_is_linux"] = NativeFunction{is_linux, 0};
        globals["platform_is_macos"] = NativeFunction{is_macos, 0};
        globals["platform_pointer_size"] = NativeFunction{pointer_size, 0};
    }
}

// ============ REGEX LIBRARY ============
namespace Regex {
    Value match_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        try {
            std::regex re(args[1].get<std::string>());
            return std::regex_match(args[0].get<std::string>(), re);
        } catch (...) { return false; }
    }

    Value search_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return Value();
        try {
            std::regex re(args[1].get<std::string>());
            std::smatch match;
            std::string str = args[0].get<std::string>();
            if (std::regex_search(str, match, re))
                return match[0].str();
            return Value();
        } catch (...) { return Value(); }
    }

    Value find_all_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return std::vector<Value>();
        try {
            std::regex re(args[1].get<std::string>());
            std::string str = args[0].get<std::string>();
            std::vector<Value> results;
            std::sregex_iterator it(str.begin(), str.end(), re);
            std::sregex_iterator end;
            for (; it != end; ++it)
                results.push_back(Value((*it)[0].str()));
            return results;
        } catch (...) { return std::vector<Value>(); }
    }

    Value replace_fn(std::vector<Value>& args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>() ||
            !args[1].holds_alternative<std::string>() || !args[2].holds_alternative<std::string>())
            return std::string("");
        try {
            std::regex re(args[1].get<std::string>());
            return std::regex_replace(args[0].get<std::string>(), re, args[2].get<std::string>());
        } catch (...) { return args[0]; }
    }

    Value split_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return std::vector<Value>();
        try {
            std::regex re(args[1].get<std::string>());
            std::string str = args[0].get<std::string>();
            std::vector<Value> results;
            std::sregex_token_iterator it(str.begin(), str.end(), re, -1);
            std::sregex_token_iterator end;
            for (; it != end; ++it)
                results.push_back(Value(it->str()));
            return results;
        } catch (...) { return std::vector<Value>(); }
    }

    Value captures_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return std::vector<Value>();
        try {
            std::regex re(args[1].get<std::string>());
            std::smatch match;
            std::vector<Value> results;
            std::string str = args[0].get<std::string>();
            if (std::regex_search(str, match, re)) {
                for (size_t i = 0; i < match.size(); ++i)
                    results.push_back(Value(match[i].str()));
            }
            return results;
        } catch (...) { return std::vector<Value>(); }
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["regex_match"] = NativeFunction{match_fn, 2};
        globals["regex_search"] = NativeFunction{search_fn, 2};
        globals["regex_find_all"] = NativeFunction{find_all_fn, 2};
        globals["regex_replace"] = NativeFunction{replace_fn, 3};
        globals["regex_split"] = NativeFunction{split_fn, 2};
        globals["regex_captures"] = NativeFunction{captures_fn, 2};
    }
}

// ============ NET SOCKET LIBRARY ============
namespace NetSocket {
    Value tcp_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw std::runtime_error("socket_tcp: failed to create socket.");
        return fd;
        #else
        throw std::runtime_error("socket_tcp: not supported on Windows.");
        #endif
    }

    Value udp_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) throw std::runtime_error("socket_udp: failed to create socket.");
        return fd;
        #else
        throw std::runtime_error("socket_udp: not supported on Windows.");
        #endif
    }

    Value bind_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.size() < 3) throw std::runtime_error("socket_bind: requires handle, host, port.");
        int fd = toInt(args[0]);
        std::string host = args[1].holds_alternative<std::string>() ? args[1].get<std::string>() : "0.0.0.0";
        int port = toInt(args[2]);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = host == "0.0.0.0" ? INADDR_ANY : inet_addr(host.c_str());
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("socket_bind: bind failed.");
        return true;
        #else
        throw std::runtime_error("socket_bind: not supported on Windows.");
        #endif
    }

    Value listen_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("socket_listen: requires handle.");
        int fd = toInt(args[0]);
        int backlog = args.size() >= 2 ? toInt(args[1]) : 128;
        if (::listen(fd, backlog) < 0)
            throw std::runtime_error("socket_listen: listen failed.");
        return true;
        #else
        throw std::runtime_error("socket_listen: not supported on Windows.");
        #endif
    }

    Value accept_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("socket_accept: requires handle.");
        int fd = toInt(args[0]);
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0)
            throw std::runtime_error("socket_accept: accept failed.");
        std::string addr_str = std::string(inet_ntoa(client_addr.sin_addr)) + ":" + std::to_string(ntohs(client_addr.sin_port));
        std::vector<Value> result;
        result.push_back(Value(client_fd));
        result.push_back(Value(addr_str));
        return result;
        #else
        throw std::runtime_error("socket_accept: not supported on Windows.");
        #endif
    }

    Value connect_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.size() < 3) throw std::runtime_error("socket_connect: requires handle, host, port.");
        int fd = toInt(args[0]);
        std::string host = args[1].get<std::string>();
        int port = toInt(args[2]);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        struct hostent* he = gethostbyname(host.c_str());
        if (he) {
            memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
        } else {
            addr.sin_addr.s_addr = inet_addr(host.c_str());
        }

        if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("socket_connect: connect failed.");
        return true;
        #else
        throw std::runtime_error("socket_connect: not supported on Windows.");
        #endif
    }

    Value send_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.size() < 2) throw std::runtime_error("socket_send: requires handle and data.");
        int fd = toInt(args[0]);
        std::string data = args[1].get<std::string>();
        ssize_t sent = ::send(fd, data.c_str(), data.size(), 0);
        if (sent < 0) throw std::runtime_error("socket_send: send failed.");
        return static_cast<int>(sent);
        #else
        throw std::runtime_error("socket_send: not supported on Windows.");
        #endif
    }

    Value recv_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("socket_recv: requires handle.");
        int fd = toInt(args[0]);
        int maxlen = args.size() >= 2 ? toInt(args[1]) : 4096;
        std::vector<char> buffer(maxlen);
        ssize_t received = ::recv(fd, buffer.data(), maxlen, 0);
        if (received < 0) throw std::runtime_error("socket_recv: recv failed.");
        if (received == 0) return std::string("");
        return std::string(buffer.data(), received);
        #else
        throw std::runtime_error("socket_recv: not supported on Windows.");
        #endif
    }

    Value close_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("socket_close: requires handle.");
        ::close(toInt(args[0]));
        return true;
        #else
        throw std::runtime_error("socket_close: not supported on Windows.");
        #endif
    }

    Value set_option_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.size() < 3) throw std::runtime_error("socket_set_option: requires handle, option, value.");
        int fd = toInt(args[0]);
        std::string option = args[1].get<std::string>();
        int val = toInt(args[2]);
        if (option == "reuse_addr") {
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
        } else if (option == "keep_alive") {
            setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));
        }
        return true;
        #else
        throw std::runtime_error("socket_set_option: not supported on Windows.");
        #endif
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["socket_tcp"] = NativeFunction{tcp_fn, 0};
        globals["socket_udp"] = NativeFunction{udp_fn, 0};
        globals["socket_bind"] = NativeFunction{bind_fn, 3};
        globals["socket_listen"] = NativeFunction{listen_fn, -1};
        globals["socket_accept"] = NativeFunction{accept_fn, 1};
        globals["socket_connect"] = NativeFunction{connect_fn, 3};
        globals["socket_send"] = NativeFunction{send_fn, 2};
        globals["socket_recv"] = NativeFunction{recv_fn, -1};
        globals["socket_close"] = NativeFunction{close_fn, 1};
        globals["socket_set_option"] = NativeFunction{set_option_fn, 3};
    }
}

// ============ NET HTTP LIBRARY ============
namespace NetHTTP {

    // ========== libcurl-powered HTTP client ==========
#ifdef HAVE_LIBCURL

    // Callback: append data to std::string
    static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        size_t total = size * nmemb;
        static_cast<std::string*>(userdata)->append(ptr, total);
        return total;
    }

    // Callback: collect response headers
    static size_t curlHeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        size_t total = size * nmemb;
        auto* headers = static_cast<std::unordered_map<std::string, Value>*>(userdata);
        std::string line(ptr, total);

        // Trim \r\n
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.empty()) return total;

        // Skip status line (starts with "HTTP/")
        if (line.find("HTTP/") == 0) return total;

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            size_t val_start = val.find_first_not_of(' ');
            if (val_start != std::string::npos) val = val.substr(val_start);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            (*headers)[key] = Value(val);
        }
        return total;
    }

    // Callback: write to FILE* for download
    static size_t curlFileWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        return fwrite(ptr, size, nmemb, static_cast<FILE*>(userdata));
    }

    static std::unordered_map<std::string, Value> doHttpRequest(
        const std::string& method, const std::string& url,
        const std::unordered_map<std::string, Value>& extra_headers,
        const std::string& body)
    {
        std::unordered_map<std::string, Value> result;
        result["status"] = Value(0);
        result["body"] = Value(std::string(""));
        result["headers"] = Value(std::unordered_map<std::string, Value>());

        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("http: failed to initialize libcurl.");

        std::string response_body;
        std::unordered_map<std::string, Value> response_headers;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlHeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Yen/1.0");
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  // Accept all encodings (gzip, deflate, etc.)

        // Set request body
        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        }

        // Set custom headers
        struct curl_slist* header_list = nullptr;
        for (const auto& [key, val] : extra_headers) {
            if (val.holds_alternative<std::string>()) {
                std::string header_line = key + ": " + val.get<std::string>();
                header_list = curl_slist_append(header_list, header_line.c_str());
            }
        }
        if (header_list) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        }

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::string err = curl_easy_strerror(res);
            if (header_list) curl_slist_free_all(header_list);
            curl_easy_cleanup(curl);
            throw std::runtime_error("http: request failed - " + err);
        }

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        result["status"] = Value(static_cast<int>(status_code));
        result["body"] = Value(response_body);
        result["headers"] = Value(response_headers);

        if (header_list) curl_slist_free_all(header_list);
        curl_easy_cleanup(curl);

        return result;
    }

    // Download URL to file (streaming, memory-efficient)
    static Value download_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            throw std::runtime_error("http_download: requires URL and filepath.");

        std::string url = args[0].get<std::string>();
        std::string filepath = args[1].get<std::string>();

        FILE* fp = fopen(filepath.c_str(), "wb");
        if (!fp) throw std::runtime_error("http_download: cannot open file '" + filepath + "' for writing.");

        CURL* curl = curl_easy_init();
        if (!curl) { fclose(fp); throw std::runtime_error("http_download: failed to initialize libcurl."); }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlFileWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Yen/1.0");

        CURLcode res = curl_easy_perform(curl);

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

        curl_easy_cleanup(curl);
        fclose(fp);

        if (res != CURLE_OK) {
            throw std::runtime_error("http_download: " + std::string(curl_easy_strerror(res)));
        }

        return static_cast<int>(status_code);
    }

    // HEAD request - returns headers map
    static Value headers_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("http_headers: requires URL string.");

        std::string url = args[0].get<std::string>();

        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("http_headers: failed to initialize libcurl.");

        std::string discard_body;
        std::unordered_map<std::string, Value> response_headers;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);  // HEAD request
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &discard_body);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curlHeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Yen/1.0");

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::string err = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            throw std::runtime_error("http_headers: " + err);
        }

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        curl_easy_cleanup(curl);

        // Add status to headers map for convenience
        response_headers["_status"] = Value(static_cast<int>(status_code));
        return response_headers;
    }

#else
    // ========== Fallback: raw socket HTTP client (no HTTPS) ==========

    struct UrlParts {
        std::string host;
        int port;
        std::string path;
    };

    static UrlParts parseUrl(const std::string& url) {
        UrlParts parts;
        parts.port = 80;
        parts.path = "/";
        std::string work = url;
        if (work.find("http://") == 0) { work = work.substr(7); }
        else if (work.find("https://") == 0) { work = work.substr(8); parts.port = 443; }
        size_t slash_pos = work.find('/');
        if (slash_pos != std::string::npos) { parts.path = work.substr(slash_pos); work = work.substr(0, slash_pos); }
        size_t colon_pos = work.find(':');
        if (colon_pos != std::string::npos) { parts.host = work.substr(0, colon_pos); parts.port = std::stoi(work.substr(colon_pos + 1)); }
        else { parts.host = work; }
        return parts;
    }

    static std::string readAllFromSocket(int fd) {
        #ifndef _WIN32
        std::string response;
        char buffer[8192];
        while (true) { ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0); if (n <= 0) break; response.append(buffer, n); }
        return response;
        #else
        return "";
        #endif
    }

    static std::unordered_map<std::string, Value> parseHttpResponse(const std::string& response) {
        std::unordered_map<std::string, Value> result;
        result["status"] = Value(0); result["body"] = Value(std::string("")); result["headers"] = Value(std::unordered_map<std::string, Value>());
        size_t status_end = response.find("\r\n");
        if (status_end == std::string::npos) { result["body"] = Value(response); return result; }
        std::string status_line = response.substr(0, status_end);
        size_t space1 = status_line.find(' ');
        if (space1 != std::string::npos) {
            size_t space2 = status_line.find(' ', space1 + 1);
            std::string status_str = space2 != std::string::npos ? status_line.substr(space1 + 1, space2 - space1 - 1) : status_line.substr(space1 + 1);
            try { result["status"] = Value(std::stoi(status_str)); } catch (...) {}
        }
        size_t headers_end = response.find("\r\n\r\n");
        if (headers_end == std::string::npos) return result;
        std::unordered_map<std::string, Value> headers;
        size_t pos = status_end + 2;
        while (pos < headers_end) {
            size_t line_end = response.find("\r\n", pos);
            if (line_end == std::string::npos || line_end > headers_end) break;
            std::string line = response.substr(pos, line_end - pos);
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon); std::string val = line.substr(colon + 1);
                size_t val_start = val.find_first_not_of(' '); if (val_start != std::string::npos) val = val.substr(val_start);
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                headers[key] = Value(val);
            }
            pos = line_end + 2;
        }
        result["headers"] = Value(headers);
        result["body"] = Value(response.substr(headers_end + 4));
        return result;
    }

    static std::unordered_map<std::string, Value> doHttpRequest(
        const std::string& method, const std::string& url,
        const std::unordered_map<std::string, Value>& extra_headers,
        const std::string& body)
    {
        #ifndef _WIN32
        if (url.find("https://") == 0)
            throw std::runtime_error("http: HTTPS not supported (libcurl not available). Use http:// or install libcurl.");
        UrlParts parts = parseUrl(url);
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw std::runtime_error("http: failed to create socket.");
        struct sockaddr_in addr; addr.sin_family = AF_INET; addr.sin_port = htons(parts.port);
        struct hostent* he = gethostbyname(parts.host.c_str());
        if (he) { memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length); } else { addr.sin_addr.s_addr = inet_addr(parts.host.c_str()); }
        if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); throw std::runtime_error("http: connect failed to " + parts.host); }
        std::string request = method + " " + parts.path + " HTTP/1.1\r\n";
        request += "Host: " + parts.host + "\r\nConnection: close\r\n";
        for (const auto& [key, val] : extra_headers) { if (val.holds_alternative<std::string>()) request += key + ": " + val.get<std::string>() + "\r\n"; }
        if (!body.empty()) request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        request += "\r\n" + body;
        ::send(fd, request.c_str(), request.size(), 0);
        std::string response = readAllFromSocket(fd); ::close(fd);
        return parseHttpResponse(response);
        #else
        throw std::runtime_error("http: not supported on Windows.");
        #endif
    }

    // Stubs for libcurl-only functions
    static Value download_fn(std::vector<Value>& args) { throw std::runtime_error("http_download: requires libcurl (not available)."); }
    static Value headers_fn(std::vector<Value>& args) { throw std::runtime_error("http_headers: requires libcurl (not available)."); }

#endif // HAVE_LIBCURL

    Value get_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("http_get: requires URL string.");
        std::unordered_map<std::string, Value> empty_headers;
        auto result = doHttpRequest("GET", args[0].get<std::string>(), empty_headers, "");
        return result;
    }

    Value post_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("http_post: requires URL and body.");
        std::string url = args[0].get<std::string>();
        std::string body = args[1].holds_alternative<std::string>() ? args[1].get<std::string>() : "";
        std::string content_type = "application/x-www-form-urlencoded";
        if (args.size() >= 3 && args[2].holds_alternative<std::string>())
            content_type = args[2].get<std::string>();
        std::unordered_map<std::string, Value> headers;
        headers["Content-Type"] = Value(content_type);
        auto result = doHttpRequest("POST", url, headers, body);
        return result;
    }

    Value put_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("http_put: requires URL and body.");
        std::string url = args[0].get<std::string>();
        std::string body = args[1].holds_alternative<std::string>() ? args[1].get<std::string>() : "";
        std::string content_type = "application/json";
        if (args.size() >= 3 && args[2].holds_alternative<std::string>())
            content_type = args[2].get<std::string>();
        std::unordered_map<std::string, Value> headers;
        headers["Content-Type"] = Value(content_type);
        return doHttpRequest("PUT", url, headers, body);
    }

    Value patch_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("http_patch: requires URL and body.");
        std::string url = args[0].get<std::string>();
        std::string body = args[1].holds_alternative<std::string>() ? args[1].get<std::string>() : "";
        std::string content_type = "application/json";
        if (args.size() >= 3 && args[2].holds_alternative<std::string>())
            content_type = args[2].get<std::string>();
        std::unordered_map<std::string, Value> headers;
        headers["Content-Type"] = Value(content_type);
        return doHttpRequest("PATCH", url, headers, body);
    }

    Value delete_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            throw std::runtime_error("http_delete: requires URL string.");
        std::unordered_map<std::string, Value> empty_headers;
        return doHttpRequest("DELETE", args[0].get<std::string>(), empty_headers, "");
    }

    Value request_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            throw std::runtime_error("http_request: requires method and URL.");
        std::string method = args[0].get<std::string>();
        std::string url = args[1].get<std::string>();
        std::unordered_map<std::string, Value> headers;
        if (args.size() >= 3 && args[2].holds_alternative<std::unordered_map<std::string, Value>>())
            headers = args[2].get<std::unordered_map<std::string, Value>>();
        std::string body = "";
        if (args.size() >= 4 && args[3].holds_alternative<std::string>())
            body = args[3].get<std::string>();
        auto result = doHttpRequest(method, url, headers, body);
        return result;
    }

    Value url_encode_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const std::string& input = args[0].get<std::string>();
        std::ostringstream encoded;
        encoded << std::hex << std::uppercase;
        for (unsigned char c : input) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded << c;
            } else {
                encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
            }
        }
        return encoded.str();
    }

    Value url_decode_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const std::string& input = args[0].get<std::string>();
        std::string decoded;
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '%' && i + 2 < input.size()) {
                std::string hex = input.substr(i + 1, 2);
                try {
                    decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
                    i += 2;
                } catch (...) {
                    decoded += input[i];
                }
            } else if (input[i] == '+') {
                decoded += ' ';
            } else {
                decoded += input[i];
            }
        }
        return decoded;
    }

    Value server_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("http_server: requires port.");
        int port = toInt(args[0]);

        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw std::runtime_error("http_server: failed to create socket.");

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

        if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            ::close(fd);
            throw std::runtime_error("http_server: bind failed on port " + std::to_string(port));
        }

        if (::listen(fd, 128) < 0) {
            ::close(fd);
            throw std::runtime_error("http_server: listen failed.");
        }

        return fd;
        #else
        throw std::runtime_error("http_server: not supported on Windows.");
        #endif
    }

    Value server_next_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("http_server_next: requires server handle.");
        int server_fd = toInt(args[0]);

        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) throw std::runtime_error("http_server_next: accept failed.");

        // Read HTTP request headers (loop until we find \r\n\r\n or hit limit)
        std::string request;
        char buffer[8192];
        size_t headers_end_pos = std::string::npos;

        while (headers_end_pos == std::string::npos && request.size() < 1048576) {  // 1MB header limit
            ssize_t n = ::recv(client_fd, buffer, sizeof(buffer), 0);
            if (n <= 0) break;
            request.append(buffer, n);
            headers_end_pos = request.find("\r\n\r\n");
        }

        // Parse request line
        std::unordered_map<std::string, Value> result;
        result["client"] = Value(client_fd);
        result["method"] = Value(std::string(""));
        result["path"] = Value(std::string(""));
        result["headers"] = Value(std::unordered_map<std::string, Value>());
        result["body"] = Value(std::string(""));

        size_t first_line_end = request.find("\r\n");
        if (first_line_end != std::string::npos) {
            std::string request_line = request.substr(0, first_line_end);
            size_t sp1 = request_line.find(' ');
            if (sp1 != std::string::npos) {
                result["method"] = Value(request_line.substr(0, sp1));
                size_t sp2 = request_line.find(' ', sp1 + 1);
                if (sp2 != std::string::npos) {
                    result["path"] = Value(request_line.substr(sp1 + 1, sp2 - sp1 - 1));
                } else {
                    result["path"] = Value(request_line.substr(sp1 + 1));
                }
            }

            // Parse headers
            std::unordered_map<std::string, Value> headers;
            size_t pos = first_line_end + 2;
            size_t hdr_end = headers_end_pos != std::string::npos ? headers_end_pos : request.size();

            long content_length = -1;

            while (pos < hdr_end) {
                size_t line_end = request.find("\r\n", pos);
                if (line_end == std::string::npos || line_end > hdr_end) break;
                std::string line = request.substr(pos, line_end - pos);
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string val = line.substr(colon + 1);
                    size_t val_start = val.find_first_not_of(' ');
                    if (val_start != std::string::npos) val = val.substr(val_start);
                    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                    headers[key] = Value(val);
                    if (key == "content-length") {
                        try { content_length = std::stol(val); } catch (...) {}
                    }
                }
                pos = line_end + 2;
            }
            result["headers"] = Value(headers);

            // Read body based on Content-Length
            if (headers_end_pos != std::string::npos) {
                size_t body_start = headers_end_pos + 4;
                std::string body_data;
                if (body_start < request.size()) {
                    body_data = request.substr(body_start);
                }

                // If Content-Length specified, read remaining body bytes
                if (content_length > 0 && static_cast<long>(body_data.size()) < content_length) {
                    long remaining = content_length - static_cast<long>(body_data.size());
                    while (remaining > 0) {
                        ssize_t n = ::recv(client_fd, buffer, std::min(static_cast<long>(sizeof(buffer)), remaining), 0);
                        if (n <= 0) break;
                        body_data.append(buffer, n);
                        remaining -= n;
                    }
                }
                result["body"] = Value(body_data);
            }
        }

        return result;
        #else
        throw std::runtime_error("http_server_next: not supported on Windows.");
        #endif
    }

    Value server_respond_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.size() < 4) throw std::runtime_error("http_server_respond: requires client, status, headers, body.");
        int client_fd = toInt(args[0]);
        int status = toInt(args[1]);

        std::unordered_map<std::string, Value> headers;
        if (args[2].holds_alternative<std::unordered_map<std::string, Value>>())
            headers = args[2].get<std::unordered_map<std::string, Value>>();

        std::string body = "";
        if (args[3].holds_alternative<std::string>())
            body = args[3].get<std::string>();

        // Build status text
        std::string status_text = "OK";
        if (status == 200) status_text = "OK";
        else if (status == 201) status_text = "Created";
        else if (status == 204) status_text = "No Content";
        else if (status == 301) status_text = "Moved Permanently";
        else if (status == 302) status_text = "Found";
        else if (status == 400) status_text = "Bad Request";
        else if (status == 401) status_text = "Unauthorized";
        else if (status == 403) status_text = "Forbidden";
        else if (status == 404) status_text = "Not Found";
        else if (status == 405) status_text = "Method Not Allowed";
        else if (status == 409) status_text = "Conflict";
        else if (status == 500) status_text = "Internal Server Error";

        std::string response = "HTTP/1.1 " + std::to_string(status) + " " + status_text + "\r\n";

        // Add headers
        bool has_content_length = false;
        bool has_connection = false;
        for (const auto& [key, val] : headers) {
            if (val.holds_alternative<std::string>()) {
                response += key + ": " + val.get<std::string>() + "\r\n";
                std::string lower_key = key;
                std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
                if (lower_key == "content-length") has_content_length = true;
                if (lower_key == "connection") has_connection = true;
            }
        }
        if (!has_content_length)
            response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        if (!has_connection)
            response += "Connection: close\r\n";
        response += "\r\n";
        response += body;

        ::send(client_fd, response.c_str(), response.size(), 0);
        ::close(client_fd);
        return true;
        #else
        throw std::runtime_error("http_server_respond: not supported on Windows.");
        #endif
    }

    Value server_close_fn(std::vector<Value>& args) {
        #ifndef _WIN32
        if (args.empty()) throw std::runtime_error("http_server_close: requires handle.");
        ::close(toInt(args[0]));
        return true;
        #else
        throw std::runtime_error("http_server_close: not supported on Windows.");
        #endif
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["http_get"] = NativeFunction{get_fn, 1};
        globals["http_post"] = NativeFunction{post_fn, -1};
        globals["http_put"] = NativeFunction{put_fn, -1};
        globals["http_patch"] = NativeFunction{patch_fn, -1};
        globals["http_delete"] = NativeFunction{delete_fn, 1};
        globals["http_request"] = NativeFunction{request_fn, -1};
        globals["http_url_encode"] = NativeFunction{url_encode_fn, 1};
        globals["http_url_decode"] = NativeFunction{url_decode_fn, 1};
        globals["http_download"] = NativeFunction{download_fn, 2};
        globals["http_headers"] = NativeFunction{headers_fn, 1};
        globals["http_server"] = NativeFunction{server_fn, 1};
        globals["http_server_next"] = NativeFunction{server_next_fn, 1};
        globals["http_server_respond"] = NativeFunction{server_respond_fn, 4};
        globals["http_server_close"] = NativeFunction{server_close_fn, 1};
    }
}

// ============ OS LIBRARY ============
namespace OS {
    Value exec_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string command = args[0].get<std::string>();
        std::string result;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) return std::string("");
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;
        pclose(pipe);
        return result;
    }

    Value exec_status_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return -1;
        return system(args[0].get<std::string>().c_str());
    }

    Value env_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        const char* value = std::getenv(args[0].get<std::string>().c_str());
        return value ? std::string(value) : std::string("");
    }

    Value set_env_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        #ifdef _WIN32
        return _putenv_s(args[0].get<std::string>().c_str(), args[1].get<std::string>().c_str()) == 0;
        #else
        return setenv(args[0].get<std::string>().c_str(), args[1].get<std::string>().c_str(), 1) == 0;
        #endif
    }

    Value cwd_fn(std::vector<Value>& args) {
        char buffer[4096];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) return std::string(buffer);
        return std::string("");
    }

    Value chdir_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        return chdir(args[0].get<std::string>().c_str()) == 0;
    }

    Value pid_fn(std::vector<Value>& args) {
        #ifdef _WIN32
        return static_cast<int>(GetCurrentProcessId());
        #else
        return static_cast<int>(getpid());
        #endif
    }

    Value platform_fn(std::vector<Value>& args) {
        #ifdef _WIN32
        return std::string("windows");
        #elif __APPLE__
        return std::string("macos");
        #elif __linux__
        return std::string("linux");
        #else
        return std::string("unknown");
        #endif
    }

    Value arch_fn(std::vector<Value>& args) {
        #if defined(__x86_64__) || defined(_M_X64)
        return std::string("x86_64");
        #elif defined(__aarch64__) || defined(_M_ARM64)
        return std::string("aarch64");
        #elif defined(__i386__) || defined(_M_IX86)
        return std::string("x86");
        #elif defined(__arm__) || defined(_M_ARM)
        return std::string("arm");
        #else
        return std::string("unknown");
        #endif
    }

    Value args_fn(std::vector<Value>& args) {
        return std::vector<Value>();
    }

    Value exit_fn(std::vector<Value>& args) {
        int code = 0;
        if (!args.empty()) code = toInt(args[0]);
        std::exit(code);
        return Value();
    }

    Value hostname_fn(std::vector<Value>& args) {
        char buffer[256];
        #ifndef _WIN32
        if (gethostname(buffer, sizeof(buffer)) == 0) return std::string(buffer);
        #endif
        return std::string("unknown");
    }

    Value read_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::ifstream file(args[0].get<std::string>());
        if (!file.is_open()) return std::string("");
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    Value write_fn(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>())
            return false;
        std::ofstream file(args[0].get<std::string>());
        if (!file.is_open()) return false;
        file << args[1].get<std::string>();
        return true;
    }

    Value ls_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::vector<Value>();
        std::vector<Value> result;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(args[0].get<std::string>())) {
                result.push_back(Value(entry.path().filename().string()));
            }
        } catch (...) {}
        return result;
    }

    Value mkdir_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        try { return std::filesystem::create_directories(args[0].get<std::string>()); }
        catch (...) { return false; }
    }

    Value rm_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        try {
            std::filesystem::path p(args[0].get<std::string>());
            if (std::filesystem::is_directory(p))
                return static_cast<int>(std::filesystem::remove_all(p)) > 0;
            return std::filesystem::remove(p);
        } catch (...) { return false; }
    }

    Value exists_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        return std::filesystem::exists(args[0].get<std::string>());
    }

    Value is_dir_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        return std::filesystem::is_directory(args[0].get<std::string>());
    }

    Value stat_fn(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            return std::unordered_map<std::string, Value>();
        std::unordered_map<std::string, Value> result;
        std::string path = args[0].get<std::string>();
        try {
            result["is_dir"] = Value(std::filesystem::is_directory(path));
            result["is_file"] = Value(std::filesystem::is_regular_file(path));
            if (std::filesystem::is_regular_file(path))
                result["size"] = Value(static_cast<int>(std::filesystem::file_size(path)));
            else
                result["size"] = Value(0);
        } catch (...) {
            result["is_dir"] = Value(false);
            result["is_file"] = Value(false);
            result["size"] = Value(0);
        }
        return result;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["os_exec"] = NativeFunction{exec_fn, 1};
        globals["os_exec_status"] = NativeFunction{exec_status_fn, 1};
        globals["os_env"] = NativeFunction{env_fn, 1};
        globals["os_set_env"] = NativeFunction{set_env_fn, 2};
        globals["os_cwd"] = NativeFunction{cwd_fn, 0};
        globals["os_chdir"] = NativeFunction{chdir_fn, 1};
        globals["os_pid"] = NativeFunction{pid_fn, 0};
        globals["os_platform"] = NativeFunction{platform_fn, 0};
        globals["os_arch"] = NativeFunction{arch_fn, 0};
        globals["os_args"] = NativeFunction{args_fn, 0};
        globals["os_exit"] = NativeFunction{exit_fn, -1};
        globals["os_hostname"] = NativeFunction{hostname_fn, 0};
        globals["os_read"] = NativeFunction{read_fn, 1};
        globals["os_write"] = NativeFunction{write_fn, 2};
        globals["os_ls"] = NativeFunction{ls_fn, 1};
        globals["os_mkdir"] = NativeFunction{mkdir_fn, 1};
        globals["os_rm"] = NativeFunction{rm_fn, 1};
        globals["os_exists"] = NativeFunction{exists_fn, 1};
        globals["os_is_dir"] = NativeFunction{is_dir_fn, 1};
        globals["os_stat"] = NativeFunction{stat_fn, 1};
    }
}

// ============ ASYNC LIBRARY ============
namespace Async {
    struct Channel {
        std::queue<Value> buffer;
        std::mutex mtx;
        std::condition_variable cv_send;
        std::condition_variable cv_recv;
        size_t capacity;
        bool closed;

        Channel(size_t cap = 0) : capacity(cap), closed(false) {}
    };

    static std::mutex registryMutex;
    static std::unordered_map<int, std::shared_ptr<Channel>> channels;
    static std::atomic<int> nextChannelId{1};

    Value chan_fn(std::vector<Value>& args) {
        size_t cap = args.empty() ? 0 : static_cast<size_t>(toInt(args[0]));
        int id = nextChannelId++;
        auto ch = std::make_shared<Channel>(cap);
        std::lock_guard<std::mutex> lock(registryMutex);
        channels[id] = ch;
        return id;
    }

    Value send_fn(std::vector<Value>& args) {
        if (args.size() < 2) throw std::runtime_error("send: requires channel and value.");
        int id = toInt(args[0]);
        std::shared_ptr<Channel> ch;
        {
            std::lock_guard<std::mutex> lock(registryMutex);
            auto it = channels.find(id);
            if (it == channels.end()) throw std::runtime_error("send: invalid channel.");
            ch = it->second;
        }
        std::unique_lock<std::mutex> lock(ch->mtx);
        if (ch->closed) throw std::runtime_error("send: channel is closed.");
        if (ch->capacity > 0) {
            ch->cv_send.wait(lock, [&] { return ch->buffer.size() < ch->capacity || ch->closed; });
        }
        if (ch->closed) throw std::runtime_error("send: channel is closed.");
        ch->buffer.push(args[1]);
        ch->cv_recv.notify_one();
        return Value();
    }

    Value recv_fn(std::vector<Value>& args) {
        if (args.empty()) throw std::runtime_error("recv: requires channel.");
        int id = toInt(args[0]);
        std::shared_ptr<Channel> ch;
        {
            std::lock_guard<std::mutex> lock(registryMutex);
            auto it = channels.find(id);
            if (it == channels.end()) throw std::runtime_error("recv: invalid channel.");
            ch = it->second;
        }
        std::unique_lock<std::mutex> lock(ch->mtx);
        ch->cv_recv.wait(lock, [&] { return !ch->buffer.empty() || ch->closed; });
        if (ch->buffer.empty()) return Value();
        Value val = ch->buffer.front();
        ch->buffer.pop();
        ch->cv_send.notify_one();
        return val;
    }

    Value try_recv_fn(std::vector<Value>& args) {
        if (args.empty()) throw std::runtime_error("try_recv: requires channel.");
        int id = toInt(args[0]);
        std::shared_ptr<Channel> ch;
        {
            std::lock_guard<std::mutex> lock(registryMutex);
            auto it = channels.find(id);
            if (it == channels.end()) throw std::runtime_error("try_recv: invalid channel.");
            ch = it->second;
        }
        std::lock_guard<std::mutex> lock(ch->mtx);
        if (ch->buffer.empty()) return Value();
        Value val = ch->buffer.front();
        ch->buffer.pop();
        ch->cv_send.notify_one();
        return val;
    }

    Value close_fn(std::vector<Value>& args) {
        if (args.empty()) throw std::runtime_error("close_chan: requires channel.");
        int id = toInt(args[0]);
        std::shared_ptr<Channel> ch;
        {
            std::lock_guard<std::mutex> lock(registryMutex);
            auto it = channels.find(id);
            if (it == channels.end()) throw std::runtime_error("close_chan: invalid channel.");
            ch = it->second;
        }
        std::lock_guard<std::mutex> lock(ch->mtx);
        ch->closed = true;
        ch->cv_recv.notify_all();
        ch->cv_send.notify_all();
        return Value();
    }

    Value sleep_fn(std::vector<Value>& args) {
        if (args.empty()) return Value();
        int ms = toInt(args[0]);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return Value();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["chan"] = NativeFunction{chan_fn, -1};
        globals["send"] = NativeFunction{send_fn, 2};
        globals["recv"] = NativeFunction{recv_fn, 1};
        globals["try_recv"] = NativeFunction{try_recv_fn, 1};
        globals["close_chan"] = NativeFunction{close_fn, 1};
        globals["sleep"] = NativeFunction{sleep_fn, 1};
    }
}

// ============ DATETIME LIBRARY ============
namespace DateTime {
    static std::string valToStr(const Value& v) {
        if (v.holds_alternative<int>()) return std::to_string(v.get<int>());
        if (v.holds_alternative<double>()) return std::to_string(v.get<double>());
        if (v.holds_alternative<std::string>()) return v.get<std::string>();
        if (v.holds_alternative<bool>()) return v.get<bool>() ? "true" : "false";
        return "none";
    }

    Value datetime_now(std::vector<Value>& args) {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        double seconds = std::chrono::duration<double>(duration).count();
        return seconds;
    }

    Value datetime_format(std::vector<Value>& args) {
        if (args.size() < 2) return std::string("");
        double timestamp = toDouble(args[0]);
        const auto& fmt = args[1].get<std::string>();
        std::time_t t = static_cast<std::time_t>(timestamp);
        std::tm* tm_info = std::localtime(&t);
        if (!tm_info) return std::string("");
        char buffer[256];
        std::strftime(buffer, sizeof(buffer), fmt.c_str(), tm_info);
        return std::string(buffer);
    }

    Value datetime_parse(std::vector<Value>& args) {
        if (args.size() < 2) return 0.0;
        const auto& str = args[0].get<std::string>();
        const auto& fmt = args[1].get<std::string>();
        std::tm tm_info = {};
        tm_info.tm_isdst = -1;
#ifdef _WIN32
        // Windows: manual parsing using sscanf for common formats
        // Try ISO format: YYYY-MM-DD HH:MM:SS
        if (sscanf(str.c_str(), "%d-%d-%d %d:%d:%d",
                   &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
                   &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) >= 3) {
            tm_info.tm_year -= 1900;
            tm_info.tm_mon -= 1;
            std::time_t t = std::mktime(&tm_info);
            return static_cast<double>(t);
        }
        return 0.0;
#else
        char* result = strptime(str.c_str(), fmt.c_str(), &tm_info);
        if (!result) return 0.0;
        std::time_t t = std::mktime(&tm_info);
        return static_cast<double>(t);
#endif
    }

    Value datetime_add(std::vector<Value>& args) {
        if (args.size() < 2) return 0.0;
        double timestamp = toDouble(args[0]);
        double seconds = toDouble(args[1]);
        return timestamp + seconds;
    }

    Value datetime_diff(std::vector<Value>& args) {
        if (args.size() < 2) return 0.0;
        double t1 = toDouble(args[0]);
        double t2 = toDouble(args[1]);
        return t1 - t2;
    }

    static Value datetime_component(std::vector<Value>& args, int component) {
        if (args.empty()) return 0;
        double timestamp = toDouble(args[0]);
        std::time_t t = static_cast<std::time_t>(timestamp);
        std::tm* tm_info = std::localtime(&t);
        if (!tm_info) return 0;
        switch (component) {
            case 0: return tm_info->tm_year + 1900;  // year
            case 1: return tm_info->tm_mon + 1;       // month (1-12)
            case 2: return tm_info->tm_mday;           // day
            case 3: return tm_info->tm_hour;           // hour
            case 4: return tm_info->tm_min;            // minute
            case 5: return tm_info->tm_sec;            // second
            case 6: return tm_info->tm_wday;           // weekday (0=Sunday)
            default: return 0;
        }
    }

    Value datetime_year(std::vector<Value>& args) { return datetime_component(args, 0); }
    Value datetime_month(std::vector<Value>& args) { return datetime_component(args, 1); }
    Value datetime_day(std::vector<Value>& args) { return datetime_component(args, 2); }
    Value datetime_hour(std::vector<Value>& args) { return datetime_component(args, 3); }
    Value datetime_minute(std::vector<Value>& args) { return datetime_component(args, 4); }
    Value datetime_second(std::vector<Value>& args) { return datetime_component(args, 5); }
    Value datetime_weekday(std::vector<Value>& args) { return datetime_component(args, 6); }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["datetime_now"] = NativeFunction{datetime_now, 0};
        globals["datetime_format"] = NativeFunction{datetime_format, 2};
        globals["datetime_parse"] = NativeFunction{datetime_parse, 2};
        globals["datetime_add"] = NativeFunction{datetime_add, 2};
        globals["datetime_diff"] = NativeFunction{datetime_diff, 2};
        globals["datetime_year"] = NativeFunction{datetime_year, 1};
        globals["datetime_month"] = NativeFunction{datetime_month, 1};
        globals["datetime_day"] = NativeFunction{datetime_day, 1};
        globals["datetime_hour"] = NativeFunction{datetime_hour, 1};
        globals["datetime_minute"] = NativeFunction{datetime_minute, 1};
        globals["datetime_second"] = NativeFunction{datetime_second, 1};
        globals["datetime_weekday"] = NativeFunction{datetime_weekday, 1};
    }
}

// ============ TESTING LIBRARY ============
namespace Testing {
    static int test_pass_count = 0;
    static int test_fail_count = 0;

    Value test_describe(std::vector<Value>& args) {
        if (args.empty()) return Value();
        std::string name = "";
        if (args[0].holds_alternative<std::string>()) name = args[0].get<std::string>();
        std::cout << "\n=== " << name << " ===" << std::endl;
        return Value();
    }

    Value test_it(std::vector<Value>& args) {
        if (args.empty()) return Value();
        std::string name = "";
        if (args[0].holds_alternative<std::string>()) name = args[0].get<std::string>();
        std::cout << "  - " << name << " ";
        return Value();
    }

    Value test_assert_eq(std::vector<Value>& args) {
        if (args.size() < 2) {
            test_fail_count++;
            std::cout << "[FAIL] assert_eq: not enough arguments" << std::endl;
            throw std::runtime_error("assert_eq: expected 2 arguments");
        }
        if (args[0] == args[1]) {
            test_pass_count++;
            std::cout << "[PASS]" << std::endl;
            return true;
        }
        test_fail_count++;
        std::cout << "[FAIL] expected values to be equal" << std::endl;
        throw std::runtime_error("assert_eq failed: values are not equal");
    }

    Value test_assert_neq(std::vector<Value>& args) {
        if (args.size() < 2) {
            test_fail_count++;
            std::cout << "[FAIL] assert_neq: not enough arguments" << std::endl;
            throw std::runtime_error("assert_neq: expected 2 arguments");
        }
        if (!(args[0] == args[1])) {
            test_pass_count++;
            std::cout << "[PASS]" << std::endl;
            return true;
        }
        test_fail_count++;
        std::cout << "[FAIL] expected values to be not equal" << std::endl;
        throw std::runtime_error("assert_neq failed: values are equal");
    }

    Value test_assert_true(std::vector<Value>& args) {
        if (args.empty()) {
            test_fail_count++;
            std::cout << "[FAIL] assert_true: no argument" << std::endl;
            throw std::runtime_error("assert_true: expected 1 argument");
        }
        bool truthy = false;
        const auto& val = args[0];
        if (val.holds_alternative<bool>()) truthy = val.get<bool>();
        else if (val.holds_alternative<int>()) truthy = val.get<int>() != 0;
        else if (val.holds_alternative<double>()) truthy = val.get<double>() != 0.0;
        else if (val.holds_alternative<std::string>()) truthy = !val.get<std::string>().empty();
        else if (val.holds_alternative<std::monostate>()) truthy = false;
        else truthy = true; // non-null, non-empty values are truthy

        if (truthy) {
            test_pass_count++;
            std::cout << "[PASS]" << std::endl;
            return true;
        }
        test_fail_count++;
        std::cout << "[FAIL] expected truthy value" << std::endl;
        throw std::runtime_error("assert_true failed: value is falsy");
    }

    Value test_assert_false(std::vector<Value>& args) {
        if (args.empty()) {
            test_fail_count++;
            std::cout << "[FAIL] assert_false: no argument" << std::endl;
            throw std::runtime_error("assert_false: expected 1 argument");
        }
        bool truthy = false;
        const auto& val = args[0];
        if (val.holds_alternative<bool>()) truthy = val.get<bool>();
        else if (val.holds_alternative<int>()) truthy = val.get<int>() != 0;
        else if (val.holds_alternative<double>()) truthy = val.get<double>() != 0.0;
        else if (val.holds_alternative<std::string>()) truthy = !val.get<std::string>().empty();
        else if (val.holds_alternative<std::monostate>()) truthy = false;
        else truthy = true;

        if (!truthy) {
            test_pass_count++;
            std::cout << "[PASS]" << std::endl;
            return true;
        }
        test_fail_count++;
        std::cout << "[FAIL] expected falsy value" << std::endl;
        throw std::runtime_error("assert_false failed: value is truthy");
    }

    Value test_assert_throws(std::vector<Value>& args) {
        // Placeholder: cannot call Yen functions from native code
        // The user should use try/catch in Yen code to test for throws
        std::cout << "[SKIP] assert_throws (use try/catch in Yen code)" << std::endl;
        return Value();
    }

    Value test_pass(std::vector<Value>& args) {
        test_pass_count++;
        std::cout << "[PASS]" << std::endl;
        return true;
    }

    Value test_fail(std::vector<Value>& args) {
        test_fail_count++;
        std::string msg = "test failed";
        if (!args.empty() && args[0].holds_alternative<std::string>())
            msg = args[0].get<std::string>();
        std::cout << "[FAIL] " << msg << std::endl;
        throw std::runtime_error(msg);
    }

    Value test_summary(std::vector<Value>& args) {
        std::cout << "\n--- Test Summary ---" << std::endl;
        std::cout << "  Passed: " << test_pass_count << std::endl;
        std::cout << "  Failed: " << test_fail_count << std::endl;
        std::cout << "  Total:  " << (test_pass_count + test_fail_count) << std::endl;
        if (test_fail_count == 0) {
            std::cout << "  All tests passed!" << std::endl;
        }
        std::unordered_map<std::string, Value> result;
        result["passed"] = Value(test_pass_count);
        result["failed"] = Value(test_fail_count);
        result["total"] = Value(test_pass_count + test_fail_count);
        return result;
    }

    Value test_reset(std::vector<Value>& args) {
        test_pass_count = 0;
        test_fail_count = 0;
        return Value();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["test_describe"] = NativeFunction{test_describe, 1};
        globals["test_it"] = NativeFunction{test_it, 1};
        globals["test_assert_eq"] = NativeFunction{test_assert_eq, 2};
        globals["test_assert_neq"] = NativeFunction{test_assert_neq, 2};
        globals["test_assert_true"] = NativeFunction{test_assert_true, 1};
        globals["test_assert_false"] = NativeFunction{test_assert_false, 1};
        globals["test_assert_throws"] = NativeFunction{test_assert_throws, 1};
        globals["test_pass"] = NativeFunction{test_pass, 0};
        globals["test_fail"] = NativeFunction{test_fail, 1};
        globals["test_summary"] = NativeFunction{test_summary, 0};
        globals["test_reset"] = NativeFunction{test_reset, 0};
    }
}

// ============ COLOR LIBRARY ============
namespace Color {
    static std::string wrap(const std::string& code, const std::string& text) {
        return "\033[" + code + "m" + text + "\033[0m";
    }

    Value color_red(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("31", args[0].get<std::string>());
    }

    Value color_green(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("32", args[0].get<std::string>());
    }

    Value color_blue(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("34", args[0].get<std::string>());
    }

    Value color_yellow(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("33", args[0].get<std::string>());
    }

    Value color_cyan(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("36", args[0].get<std::string>());
    }

    Value color_magenta(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("35", args[0].get<std::string>());
    }

    Value color_bold(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("1", args[0].get<std::string>());
    }

    Value color_underline(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("4", args[0].get<std::string>());
    }

    Value color_dim(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("2", args[0].get<std::string>());
    }

    Value color_reset(std::vector<Value>& args) {
        return std::string("\033[0m");
    }

    Value color_rgb(std::vector<Value>& args) {
        if (args.size() < 4) return std::string("");
        int r = toInt(args[0]);
        int g = toInt(args[1]);
        int b = toInt(args[2]);
        if (!args[3].holds_alternative<std::string>()) return std::string("");
        const auto& text = args[3].get<std::string>();
        // 24-bit true color ANSI: \033[38;2;R;G;Bm
        std::string code = "38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b);
        return wrap(code, text);
    }

    Value color_bg_red(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("41", args[0].get<std::string>());
    }

    Value color_bg_green(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("42", args[0].get<std::string>());
    }

    Value color_bg_blue(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        return wrap("44", args[0].get<std::string>());
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["color_red"] = NativeFunction{color_red, 1};
        globals["color_green"] = NativeFunction{color_green, 1};
        globals["color_blue"] = NativeFunction{color_blue, 1};
        globals["color_yellow"] = NativeFunction{color_yellow, 1};
        globals["color_cyan"] = NativeFunction{color_cyan, 1};
        globals["color_magenta"] = NativeFunction{color_magenta, 1};
        globals["color_bold"] = NativeFunction{color_bold, 1};
        globals["color_underline"] = NativeFunction{color_underline, 1};
        globals["color_dim"] = NativeFunction{color_dim, 1};
        globals["color_reset"] = NativeFunction{color_reset, 0};
        globals["color_rgb"] = NativeFunction{color_rgb, 4};
        globals["color_bg_red"] = NativeFunction{color_bg_red, 1};
        globals["color_bg_green"] = NativeFunction{color_bg_green, 1};
        globals["color_bg_blue"] = NativeFunction{color_bg_blue, 1};
    }
}

// ============ SET LIBRARY ============
namespace Set {
    // Helper: check if a value exists in a list (using operator==)
    static bool listContains(const std::vector<Value>& vec, const Value& val) {
        for (const auto& item : vec) {
            if (item == val) return true;
        }
        return false;
    }

    Value set_new(std::vector<Value>& args) {
        return std::vector<Value>();
    }

    Value set_from_list(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>())
            return std::vector<Value>();
        const auto& input = args[0].get<std::vector<Value>>();
        std::vector<Value> result;
        for (const auto& item : input) {
            if (!listContains(result, item)) {
                result.push_back(item);
            }
        }
        return result;
    }

    Value set_add(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>())
            return args.empty() ? Value(std::vector<Value>()) : args[0];
        auto vec = args[0].get<std::vector<Value>>();
        if (!listContains(vec, args[1])) {
            vec.push_back(args[1]);
        }
        return vec;
    }

    Value set_remove(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>())
            return args.empty() ? Value(std::vector<Value>()) : args[0];
        auto vec = args[0].get<std::vector<Value>>();
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (*it == args[1]) {
                vec.erase(it);
                break;
            }
        }
        return vec;
    }

    Value set_contains(std::vector<Value>& args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>())
            return false;
        const auto& vec = args[0].get<std::vector<Value>>();
        return listContains(vec, args[1]);
    }

    Value set_size(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) return 0;
        return static_cast<int>(args[0].get<std::vector<Value>>().size());
    }

    Value set_union(std::vector<Value>& args) {
        if (args.size() < 2) return std::vector<Value>();
        std::vector<Value> result;
        if (args[0].holds_alternative<std::vector<Value>>()) {
            result = args[0].get<std::vector<Value>>();
        }
        if (args[1].holds_alternative<std::vector<Value>>()) {
            const auto& b = args[1].get<std::vector<Value>>();
            for (const auto& item : b) {
                if (!listContains(result, item)) {
                    result.push_back(item);
                }
            }
        }
        return result;
    }

    Value set_intersect(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::vector<Value>>() ||
            !args[1].holds_alternative<std::vector<Value>>())
            return std::vector<Value>();
        const auto& a = args[0].get<std::vector<Value>>();
        const auto& b = args[1].get<std::vector<Value>>();
        std::vector<Value> result;
        for (const auto& item : a) {
            if (listContains(b, item)) {
                result.push_back(item);
            }
        }
        return result;
    }

    Value set_difference(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::vector<Value>>() ||
            !args[1].holds_alternative<std::vector<Value>>())
            return std::vector<Value>();
        const auto& a = args[0].get<std::vector<Value>>();
        const auto& b = args[1].get<std::vector<Value>>();
        std::vector<Value> result;
        for (const auto& item : a) {
            if (!listContains(b, item)) {
                result.push_back(item);
            }
        }
        return result;
    }

    Value set_symmetric_diff(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::vector<Value>>() ||
            !args[1].holds_alternative<std::vector<Value>>())
            return std::vector<Value>();
        const auto& a = args[0].get<std::vector<Value>>();
        const auto& b = args[1].get<std::vector<Value>>();
        std::vector<Value> result;
        // Elements in a but not in b
        for (const auto& item : a) {
            if (!listContains(b, item)) {
                result.push_back(item);
            }
        }
        // Elements in b but not in a
        for (const auto& item : b) {
            if (!listContains(a, item)) {
                result.push_back(item);
            }
        }
        return result;
    }

    Value set_is_subset(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::vector<Value>>() ||
            !args[1].holds_alternative<std::vector<Value>>())
            return false;
        const auto& a = args[0].get<std::vector<Value>>();
        const auto& b = args[1].get<std::vector<Value>>();
        for (const auto& item : a) {
            if (!listContains(b, item)) return false;
        }
        return true;
    }

    Value set_to_list(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>())
            return std::vector<Value>();
        return args[0];
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["set_new"] = NativeFunction{set_new, 0};
        globals["set_from_list"] = NativeFunction{set_from_list, 1};
        globals["set_add"] = NativeFunction{set_add, 2};
        globals["set_remove"] = NativeFunction{set_remove, 2};
        globals["set_contains"] = NativeFunction{set_contains, 2};
        globals["set_size"] = NativeFunction{set_size, 1};
        globals["set_union"] = NativeFunction{set_union, 2};
        globals["set_intersect"] = NativeFunction{set_intersect, 2};
        globals["set_difference"] = NativeFunction{set_difference, 2};
        globals["set_symmetric_diff"] = NativeFunction{set_symmetric_diff, 2};
        globals["set_is_subset"] = NativeFunction{set_is_subset, 2};
        globals["set_to_list"] = NativeFunction{set_to_list, 1};
    }
}

// ============ PATH LIBRARY ============
namespace Path {
    Value path_join(std::vector<Value>& args) {
        if (args.empty()) return std::string("");
        std::filesystem::path result;
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) {
                if (result.empty()) {
                    result = std::filesystem::path(arg.get<std::string>());
                } else {
                    result /= arg.get<std::string>();
                }
            }
        }
        return result.string();
    }

    Value path_dirname(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::filesystem::path p(args[0].get<std::string>());
        return p.parent_path().string();
    }

    Value path_basename(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::filesystem::path p(args[0].get<std::string>());
        return p.filename().string();
    }

    Value path_extension(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::filesystem::path p(args[0].get<std::string>());
        return p.extension().string();
    }

    Value path_stem(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::filesystem::path p(args[0].get<std::string>());
        return p.stem().string();
    }

    Value path_resolve(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        try {
            std::filesystem::path p(args[0].get<std::string>());
            return std::filesystem::absolute(p).string();
        } catch (...) {
            return std::string("");
        }
    }

    Value path_normalize(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::filesystem::path p(args[0].get<std::string>());
        return p.lexically_normal().string();
    }

    Value path_is_absolute(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return false;
        std::filesystem::path p(args[0].get<std::string>());
        return p.is_absolute();
    }

    Value path_relative(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::string>() ||
            !args[1].holds_alternative<std::string>()) return std::string("");
        try {
            std::filesystem::path from_path(args[0].get<std::string>());
            std::filesystem::path to_path(args[1].get<std::string>());
            return to_path.lexically_relative(from_path).string();
        } catch (...) {
            return std::string("");
        }
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["path_join"] = NativeFunction{path_join, -1};
        globals["path_dirname"] = NativeFunction{path_dirname, 1};
        globals["path_basename"] = NativeFunction{path_basename, 1};
        globals["path_extension"] = NativeFunction{path_extension, 1};
        globals["path_stem"] = NativeFunction{path_stem, 1};
        globals["path_resolve"] = NativeFunction{path_resolve, 1};
        globals["path_normalize"] = NativeFunction{path_normalize, 1};
        globals["path_is_absolute"] = NativeFunction{path_is_absolute, 1};
        globals["path_relative"] = NativeFunction{path_relative, 2};
    }
}

// ============ CSV LIBRARY ============
namespace CSV {
    // Helper: parse a single CSV line handling quoted fields
    static std::vector<std::string> parseLine(const std::string& line) {
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (inQuotes) {
                if (c == '"') {
                    // Check for escaped quote ("")
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        field += '"';
                        ++i;
                    } else {
                        inQuotes = false;
                    }
                } else {
                    field += c;
                }
            } else {
                if (c == '"') {
                    inQuotes = true;
                } else if (c == ',') {
                    fields.push_back(field);
                    field.clear();
                } else {
                    field += c;
                }
            }
        }
        fields.push_back(field);
        return fields;
    }

    // Helper: split text into lines, handling \r\n and \n
    static std::vector<std::string> splitLines(const std::string& text) {
        std::vector<std::string> lines;
        std::string line;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\r') {
                if (i + 1 < text.size() && text[i + 1] == '\n') ++i;
                lines.push_back(line);
                line.clear();
            } else if (text[i] == '\n') {
                lines.push_back(line);
                line.clear();
            } else {
                line += text[i];
            }
        }
        if (!line.empty()) lines.push_back(line);
        return lines;
    }

    // Helper: escape a field for CSV output
    static std::string escapeField(const std::string& field) {
        bool needsQuote = false;
        for (char c : field) {
            if (c == ',' || c == '"' || c == '\n' || c == '\r') {
                needsQuote = true;
                break;
            }
        }
        if (!needsQuote) return field;
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += '"';
        return escaped;
    }

    // Helper: Value to string for CSV output
    static std::string valToStr(const Value& v) {
        if (v.holds_alternative<std::string>()) return v.get<std::string>();
        if (v.holds_alternative<int>()) return std::to_string(v.get<int>());
        if (v.holds_alternative<double>()) return std::to_string(v.get<double>());
        if (v.holds_alternative<bool>()) return v.get<bool>() ? "true" : "false";
        if (v.holds_alternative<std::monostate>()) return "";
        return "";
    }

    Value csv_parse(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            return std::vector<Value>();
        const auto& text = args[0].get<std::string>();
        auto lines = splitLines(text);
        std::vector<Value> result;
        for (const auto& line : lines) {
            if (line.empty()) continue;
            auto fields = parseLine(line);
            std::vector<Value> row;
            for (const auto& f : fields) {
                row.push_back(Value(f));
            }
            result.push_back(Value(row));
        }
        return result;
    }

    Value csv_parse_header(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            return std::vector<Value>();
        const auto& text = args[0].get<std::string>();
        auto lines = splitLines(text);
        if (lines.empty()) return std::vector<Value>();

        // First line is headers
        auto headers = parseLine(lines[0]);
        std::vector<Value> result;
        for (size_t i = 1; i < lines.size(); ++i) {
            if (lines[i].empty()) continue;
            auto fields = parseLine(lines[i]);
            std::unordered_map<std::string, Value> row;
            for (size_t j = 0; j < headers.size(); ++j) {
                if (j < fields.size()) {
                    row[headers[j]] = Value(fields[j]);
                } else {
                    row[headers[j]] = Value(std::string(""));
                }
            }
            result.push_back(Value(row));
        }
        return result;
    }

    Value csv_stringify(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>())
            return std::string("");
        const auto& rows = args[0].get<std::vector<Value>>();
        std::string result;
        for (size_t i = 0; i < rows.size(); ++i) {
            if (!rows[i].holds_alternative<std::vector<Value>>()) continue;
            const auto& row = rows[i].get<std::vector<Value>>();
            for (size_t j = 0; j < row.size(); ++j) {
                if (j > 0) result += ',';
                result += escapeField(valToStr(row[j]));
            }
            result += '\n';
        }
        return result;
    }

    Value csv_read(std::vector<Value>& args) {
        if (args.empty() || !args[0].holds_alternative<std::string>())
            return std::vector<Value>();
        const auto& filepath = args[0].get<std::string>();
        std::ifstream file(filepath);
        if (!file.is_open()) return std::vector<Value>();
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        // Reuse csv_parse
        std::vector<Value> parseArgs;
        parseArgs.push_back(Value(content));
        return csv_parse(parseArgs);
    }

    Value csv_write(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::string>() ||
            !args[1].holds_alternative<std::vector<Value>>())
            return Value();
        const auto& filepath = args[0].get<std::string>();
        // Stringify the data
        std::vector<Value> strArgs;
        strArgs.push_back(args[1]);
        Value csvStr = csv_stringify(strArgs);
        if (!csvStr.holds_alternative<std::string>()) return Value();
        std::ofstream file(filepath);
        if (!file.is_open()) return Value();
        file << csvStr.get<std::string>();
        file.close();
        return Value();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["csv_parse"] = NativeFunction{csv_parse, 1};
        globals["csv_parse_header"] = NativeFunction{csv_parse_header, 1};
        globals["csv_stringify"] = NativeFunction{csv_stringify, 1};
        globals["csv_read"] = NativeFunction{csv_read, 1};
        globals["csv_write"] = NativeFunction{csv_write, 2};
    }
}

// ============ EVENT LIBRARY ============
namespace Event {
    // Events use maps as emitters.
    // Structure: {"__events": {"event_name": [callback1, callback2, ...], ...}}
    // Since we can't call Yen functions from C++, event_emit returns the list of
    // callbacks for the given event, allowing Yen code to iterate and call them.

    Value event_new(std::vector<Value>& args) {
        std::unordered_map<std::string, Value> emitter;
        emitter["__events"] = Value(std::unordered_map<std::string, Value>());
        return emitter;
    }

    Value event_on(std::vector<Value>& args) {
        if (args.size() < 3 ||
            !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>())
            return args.empty() ? Value() : args[0];
        auto emitter = args[0].get<std::unordered_map<std::string, Value>>();
        const auto& event_name = args[1].get<std::string>();
        const auto& callback = args[2];

        // Get or create __events map
        if (emitter.find("__events") == emitter.end()) {
            emitter["__events"] = Value(std::unordered_map<std::string, Value>());
        }
        if (!emitter["__events"].holds_alternative<std::unordered_map<std::string, Value>>()) {
            emitter["__events"] = Value(std::unordered_map<std::string, Value>());
        }
        auto events = emitter["__events"].get<std::unordered_map<std::string, Value>>();

        // Get or create the listener list for this event
        std::vector<Value> listeners;
        if (events.find(event_name) != events.end() &&
            events[event_name].holds_alternative<std::vector<Value>>()) {
            listeners = events[event_name].get<std::vector<Value>>();
        }
        listeners.push_back(callback);
        events[event_name] = Value(listeners);
        emitter["__events"] = Value(events);
        return emitter;
    }

    Value event_emit(std::vector<Value>& args) {
        if (args.size() < 3 ||
            !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>())
            return std::vector<Value>();
        const auto& emitter = args[0].get<std::unordered_map<std::string, Value>>();
        const auto& event_name = args[1].get<std::string>();
        const auto& data = args[2];

        // Get __events map
        auto eventsIt = emitter.find("__events");
        if (eventsIt == emitter.end() ||
            !eventsIt->second.holds_alternative<std::unordered_map<std::string, Value>>())
            return std::vector<Value>();
        const auto& events = eventsIt->second.get<std::unordered_map<std::string, Value>>();

        // Get listeners for this event
        auto listIt = events.find(event_name);
        if (listIt == events.end() ||
            !listIt->second.holds_alternative<std::vector<Value>>())
            return std::vector<Value>();
        const auto& listeners = listIt->second.get<std::vector<Value>>();

        // Return list of [callback, data] pairs for Yen code to iterate and call
        std::vector<Value> result;
        for (const auto& cb : listeners) {
            std::vector<Value> pair;
            pair.push_back(cb);
            pair.push_back(data);
            result.push_back(Value(pair));
        }
        return result;
    }

    Value event_off(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>())
            return args.empty() ? Value() : args[0];
        auto emitter = args[0].get<std::unordered_map<std::string, Value>>();
        const auto& event_name = args[1].get<std::string>();

        if (emitter.find("__events") != emitter.end() &&
            emitter["__events"].holds_alternative<std::unordered_map<std::string, Value>>()) {
            auto events = emitter["__events"].get<std::unordered_map<std::string, Value>>();
            events.erase(event_name);
            emitter["__events"] = Value(events);
        }
        return emitter;
    }

    Value event_once(std::vector<Value>& args) {
        // Same as event_on for now - one-time removal must be handled at Yen level
        // after event_emit returns the callbacks
        return event_on(args);
    }

    Value event_listeners(std::vector<Value>& args) {
        if (args.size() < 2 ||
            !args[0].holds_alternative<std::unordered_map<std::string, Value>>() ||
            !args[1].holds_alternative<std::string>())
            return 0;
        const auto& emitter = args[0].get<std::unordered_map<std::string, Value>>();
        const auto& event_name = args[1].get<std::string>();

        auto eventsIt = emitter.find("__events");
        if (eventsIt == emitter.end() ||
            !eventsIt->second.holds_alternative<std::unordered_map<std::string, Value>>())
            return 0;
        const auto& events = eventsIt->second.get<std::unordered_map<std::string, Value>>();

        auto listIt = events.find(event_name);
        if (listIt == events.end() ||
            !listIt->second.holds_alternative<std::vector<Value>>())
            return 0;
        return static_cast<int>(listIt->second.get<std::vector<Value>>().size());
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["event_new"] = NativeFunction{event_new, 0};
        globals["event_on"] = NativeFunction{event_on, 3};
        globals["event_emit"] = NativeFunction{event_emit, 3};
        globals["event_off"] = NativeFunction{event_off, 2};
        globals["event_once"] = NativeFunction{event_once, 3};
        globals["event_listeners"] = NativeFunction{event_listeners, 2};
    }
}

// Placeholder implementations for remaining libraries
namespace Thread { void registerFunctions(std::unordered_map<std::string, Value>&) {} }
namespace Net { void registerFunctions(std::unordered_map<std::string, Value>&) {} }
namespace HTTP { void registerFunctions(std::unordered_map<std::string, Value>&) {} }
namespace Runtime { void registerFunctions(std::unordered_map<std::string, Value>&) {} }

// Main registration function
void registerAllLibraries(std::unordered_map<std::string, Value>& globals) {
    Core::registerFunctions(globals);
    Math::registerFunctions(globals);
    String::registerFunctions(globals);
    Collections::registerFunctions(globals);
    Map::registerFunctions(globals);
    Json::registerFunctions(globals);
    Utility::registerFunctions(globals);
    IO::registerFunctions(globals);
    FS::registerFunctions(globals);
    Time::registerFunctions(globals);
    Crypto::registerFunctions(globals);
    Encoding::registerFunctions(globals);
    Log::registerFunctions(globals);
    Env::registerFunctions(globals);
    Process::registerFunctions(globals);
    Platform::registerFunctions(globals);
    Regex::registerFunctions(globals);
    NetSocket::registerFunctions(globals);
    NetHTTP::registerFunctions(globals);
    OS::registerFunctions(globals);
    Async::registerFunctions(globals);
    Thread::registerFunctions(globals);
    Net::registerFunctions(globals);
    HTTP::registerFunctions(globals);
    Runtime::registerFunctions(globals);
    DateTime::registerFunctions(globals);
    Testing::registerFunctions(globals);
    Color::registerFunctions(globals);
    Set::registerFunctions(globals);
    Path::registerFunctions(globals);
    CSV::registerFunctions(globals);
    Event::registerFunctions(globals);
}

} // namespace YenNative
