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
#include <unistd.h>

namespace YenNative {

// ============ CORE LIBRARY ============
namespace Core {
    // Type checking functions
    Value isInt(std::vector<Value> args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<int>();
    }

    Value isFloat(std::vector<Value> args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<float>() || args[0].holds_alternative<double>();
    }

    Value isBool(std::vector<Value> args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<bool>();
    }

    Value isString(std::vector<Value> args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<std::string>();
    }

    Value isList(std::vector<Value> args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<std::vector<Value>>();
    }

    Value isMap(std::vector<Value> args) {
        if (args.empty()) return false;
        return args[0].holds_alternative<std::unordered_map<std::string, Value>>();
    }

    // Type conversion
    Value toInt(std::vector<Value> args) {
        if (args.empty()) return 0;
        const auto& val = args[0];

        if (val.holds_alternative<int>()) return val.get<int>();
        if (val.holds_alternative<double>()) return static_cast<int>(val.get<double>());
        if (val.holds_alternative<float>()) return static_cast<int>(val.get<float>());
        if (val.holds_alternative<bool>()) return val.get<bool>() ? 1 : 0;
        if (val.holds_alternative<std::string>()) {
            try {
                return std::stoi(val.get<std::string>());
            } catch (...) {
                return 0;
            }
        }
        return 0;
    }

    Value toFloat(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        const auto& val = args[0];

        if (val.holds_alternative<double>()) return val.get<double>();
        if (val.holds_alternative<float>()) return static_cast<double>(val.get<float>());
        if (val.holds_alternative<int>()) return static_cast<double>(val.get<int>());
        if (val.holds_alternative<std::string>()) {
            try {
                return std::stod(val.get<std::string>());
            } catch (...) {
                return 0.0;
            }
        }
        return 0.0;
    }

    Value toString(std::vector<Value> args) {
        if (args.empty()) return std::string("");
        const auto& val = args[0];

        if (val.holds_alternative<std::string>()) return val.get<std::string>();
        if (val.holds_alternative<int>()) return std::to_string(val.get<int>());
        if (val.holds_alternative<double>()) return std::to_string(val.get<double>());
        if (val.holds_alternative<float>()) return std::to_string(val.get<float>());
        if (val.holds_alternative<bool>()) return val.get<bool>() ? std::string("true") : std::string("false");

        return std::string("");
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        // Type checking
        globals["core_is_int"] = NativeFunction{isInt, 1};
        globals["core_is_float"] = NativeFunction{isFloat, 1};
        globals["core_is_bool"] = NativeFunction{isBool, 1};
        globals["core_is_string"] = NativeFunction{isString, 1};
        globals["core_is_list"] = NativeFunction{isList, 1};
        globals["core_is_map"] = NativeFunction{isMap, 1};

        // Type conversion
        globals["core_to_int"] = NativeFunction{toInt, 1};
        globals["core_to_float"] = NativeFunction{toFloat, 1};
        globals["core_to_string"] = NativeFunction{toString, 1};
    }
}

// ============ MATH LIBRARY ============
namespace Math {
    Value abs_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        const auto& val = args[0];

        if (val.holds_alternative<int>()) {
            return std::abs(val.get<int>());
        }
        if (val.holds_alternative<double>()) {
            return std::abs(val.get<double>());
        }
        return 0.0;
    }

    Value sqrt_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) x = static_cast<double>(args[0].get<int>());
        return std::sqrt(x);
    }

    Value pow_fn(std::vector<Value> args) {
        if (args.size() < 2) return 0.0;
        double base = 0.0, exp = 0.0;

        if (args[0].holds_alternative<double>()) base = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) base = static_cast<double>(args[0].get<int>());

        if (args[1].holds_alternative<double>()) exp = args[1].get<double>();
        else if (args[1].holds_alternative<int>()) exp = static_cast<double>(args[1].get<int>());

        return std::pow(base, exp);
    }

    Value sin_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) x = static_cast<double>(args[0].get<int>());
        return std::sin(x);
    }

    Value cos_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) x = static_cast<double>(args[0].get<int>());
        return std::cos(x);
    }

    Value tan_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) x = static_cast<double>(args[0].get<int>());
        return std::tan(x);
    }

    Value floor_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) return args[0].get<int>();
        return std::floor(x);
    }

    Value ceil_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) return args[0].get<int>();
        return std::ceil(x);
    }

    Value round_fn(std::vector<Value> args) {
        if (args.empty()) return 0.0;
        double x = 0.0;
        if (args[0].holds_alternative<double>()) x = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) return args[0].get<int>();
        return std::round(x);
    }

    Value min_fn(std::vector<Value> args) {
        if (args.size() < 2) return 0.0;

        double a = 0.0, b = 0.0;
        if (args[0].holds_alternative<double>()) a = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) a = static_cast<double>(args[0].get<int>());

        if (args[1].holds_alternative<double>()) b = args[1].get<double>();
        else if (args[1].holds_alternative<int>()) b = static_cast<double>(args[1].get<int>());

        return std::min(a, b);
    }

    Value max_fn(std::vector<Value> args) {
        if (args.size() < 2) return 0.0;

        double a = 0.0, b = 0.0;
        if (args[0].holds_alternative<double>()) a = args[0].get<double>();
        else if (args[0].holds_alternative<int>()) a = static_cast<double>(args[0].get<int>());

        if (args[1].holds_alternative<double>()) b = args[1].get<double>();
        else if (args[1].holds_alternative<int>()) b = static_cast<double>(args[1].get<int>());

        return std::max(a, b);
    }

    Value random_fn(std::vector<Value> args) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }

    Value random_int_fn(std::vector<Value> args) {
        if (args.size() < 2) return 0;

        int min = 0, max = 100;
        if (args[0].holds_alternative<int>()) min = args[0].get<int>();
        else if (args[0].holds_alternative<double>()) min = static_cast<int>(args[0].get<double>());

        if (args[1].holds_alternative<int>()) max = args[1].get<int>();
        else if (args[1].holds_alternative<double>()) max = static_cast<int>(args[1].get<double>());

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["math_abs"] = NativeFunction{abs_fn, 1};
        globals["math_sqrt"] = NativeFunction{sqrt_fn, 1};
        globals["math_pow"] = NativeFunction{pow_fn, 2};
        globals["math_sin"] = NativeFunction{sin_fn, 1};
        globals["math_cos"] = NativeFunction{cos_fn, 1};
        globals["math_tan"] = NativeFunction{tan_fn, 1};
        globals["math_floor"] = NativeFunction{floor_fn, 1};
        globals["math_ceil"] = NativeFunction{ceil_fn, 1};
        globals["math_round"] = NativeFunction{round_fn, 1};
        globals["math_min"] = NativeFunction{min_fn, 2};
        globals["math_max"] = NativeFunction{max_fn, 2};
        globals["math_random"] = NativeFunction{random_fn, 0};
        globals["math_random_int"] = NativeFunction{random_int_fn, 2};

        // Constants
        globals["math_PI"] = 3.14159265358979323846;
        globals["math_E"] = 2.71828182845904523536;
    }
}

// ============ STRING LIBRARY ============
namespace String {
    Value length(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return 0;
        return static_cast<int>(args[0].get<std::string>().length());
    }

    Value upper(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    Value lower(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    Value trim(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) return std::string("");
        std::string str = args[0].get<std::string>();

        // Trim left
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));

        // Trim right
        str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), str.end());

        return str;
    }

    Value split(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
            return std::vector<Value>();
        }

        std::string str = args[0].get<std::string>();
        std::string delimiter = args[1].get<std::string>();
        std::vector<Value> result;

        size_t pos = 0;
        std::string token;
        while ((pos = str.find(delimiter)) != std::string::npos) {
            token = str.substr(0, pos);
            result.push_back(Value(token));
            str.erase(0, pos + delimiter.length());
        }
        result.push_back(Value(str));

        return result;
    }

    Value join(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>() || !args[1].holds_alternative<std::string>()) {
            return std::string("");
        }

        const auto& vec = args[0].get<std::vector<Value>>();
        const std::string& separator = args[1].get<std::string>();
        std::string result;

        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i].holds_alternative<std::string>()) {
                result += vec[i].get<std::string>();
            }
            if (i < vec.size() - 1) {
                result += separator;
            }
        }

        return result;
    }

    Value substring(std::vector<Value> args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>()) {
            return std::string("");
        }

        std::string str = args[0].get<std::string>();
        int start = 0, length = str.length();

        if (args[1].holds_alternative<int>()) start = args[1].get<int>();
        if (args[2].holds_alternative<int>()) length = args[2].get<int>();

        if (start < 0 || start >= static_cast<int>(str.length())) return std::string("");

        return str.substr(start, length);
    }

    Value contains(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
            return false;
        }

        const std::string& str = args[0].get<std::string>();
        const std::string& substr = args[1].get<std::string>();

        return str.find(substr) != std::string::npos;
    }

    Value replace(std::vector<Value> args) {
        if (args.size() < 3 || !args[0].holds_alternative<std::string>() ||
            !args[1].holds_alternative<std::string>() || !args[2].holds_alternative<std::string>()) {
            return std::string("");
        }

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
    }
}

// ============ COLLECTIONS LIBRARY ============
namespace Collections {
    Value push(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::vector<Value>>()) {
            return std::vector<Value>();
        }

        auto vec = args[0].get<std::vector<Value>>();
        vec.push_back(args[1]);
        return vec;
    }

    Value pop(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) {
            return std::vector<Value>();
        }

        auto vec = args[0].get<std::vector<Value>>();
        if (!vec.empty()) {
            vec.pop_back();
        }
        return vec;
    }

    Value length(std::vector<Value> args) {
        if (args.empty()) return 0;

        if (args[0].holds_alternative<std::vector<Value>>()) {
            return static_cast<int>(args[0].get<std::vector<Value>>().size());
        }
        if (args[0].holds_alternative<std::string>()) {
            return static_cast<int>(args[0].get<std::string>().size());
        }
        if (args[0].holds_alternative<std::unordered_map<std::string, Value>>()) {
            return static_cast<int>(args[0].get<std::unordered_map<std::string, Value>>().size());
        }

        return 0;
    }

    Value reverse(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) {
            return std::vector<Value>();
        }

        auto vec = args[0].get<std::vector<Value>>();
        std::reverse(vec.begin(), vec.end());
        return vec;
    }

    Value sort(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::vector<Value>>()) {
            return std::vector<Value>();
        }

        auto vec = args[0].get<std::vector<Value>>();
        std::sort(vec.begin(), vec.end(), [](const Value& a, const Value& b) {
            return a < b;
        });
        return vec;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["list_push"] = NativeFunction{push, 2};
        globals["list_pop"] = NativeFunction{pop, 1};
        globals["list_length"] = NativeFunction{length, 1};
        globals["list_reverse"] = NativeFunction{reverse, 1};
        globals["list_sort"] = NativeFunction{sort, 1};
    }
}

// Main registration function
void registerAllLibraries(std::unordered_map<std::string, Value>& globals) {
    Core::registerFunctions(globals);
    Math::registerFunctions(globals);
    String::registerFunctions(globals);
    Collections::registerFunctions(globals);
    IO::registerFunctions(globals);
    FS::registerFunctions(globals);
    Time::registerFunctions(globals);
    Crypto::registerFunctions(globals);
    Encoding::registerFunctions(globals);
    Log::registerFunctions(globals);
    Env::registerFunctions(globals);
    Process::registerFunctions(globals);
    Thread::registerFunctions(globals);
    Net::registerFunctions(globals);
    HTTP::registerFunctions(globals);
    Runtime::registerFunctions(globals);
}

// ============ IO LIBRARY ============
namespace IO {
    Value readFile(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return std::string("");
        }

        std::string path = args[0].get<std::string>();
        std::ifstream file(path);
        if (!file.is_open()) {
            return std::string("");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    Value writeFile(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        std::string content = args[1].get<std::string>();

        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        file << content;
        file.close();
        return true;
    }

    Value appendFile(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        std::string content = args[1].get<std::string>();

        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) {
            return false;
        }

        file << content;
        file.close();
        return true;
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["io_read_file"] = NativeFunction{readFile, 1};
        globals["io_write_file"] = NativeFunction{writeFile, 2};
        globals["io_append_file"] = NativeFunction{appendFile, 2};
    }
}

// ============ FS LIBRARY ============
namespace FS {
    Value fileExists(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        return std::filesystem::exists(path);
    }

    Value isDirectory(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        return std::filesystem::is_directory(path);
    }

    Value isFile(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        return std::filesystem::is_regular_file(path);
    }

    Value createDirectory(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        try {
            return std::filesystem::create_directories(path);
        } catch (...) {
            return false;
        }
    }

    Value removeFile(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return false;
        }

        std::string path = args[0].get<std::string>();
        try {
            return std::filesystem::remove(path);
        } catch (...) {
            return false;
        }
    }

    Value fileSize(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return 0;
        }

        std::string path = args[0].get<std::string>();
        try {
            return static_cast<int>(std::filesystem::file_size(path));
        } catch (...) {
            return 0;
        }
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["fs_exists"] = NativeFunction{fileExists, 1};
        globals["fs_is_directory"] = NativeFunction{isDirectory, 1};
        globals["fs_is_file"] = NativeFunction{isFile, 1};
        globals["fs_create_dir"] = NativeFunction{createDirectory, 1};
        globals["fs_remove"] = NativeFunction{removeFile, 1};
        globals["fs_file_size"] = NativeFunction{fileSize, 1};
    }
}

// ============ TIME LIBRARY ============
namespace Time {
    Value now(std::vector<Value> args) {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return static_cast<double>(millis);
    }

    Value sleep(std::vector<Value> args) {
        if (args.empty()) return Value();

        int ms = 0;
        if (args[0].holds_alternative<int>()) {
            ms = args[0].get<int>();
        } else if (args[0].holds_alternative<double>()) {
            ms = static_cast<int>(args[0].get<double>());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return Value();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["time_now"] = NativeFunction{now, 0};
        globals["time_sleep"] = NativeFunction{sleep, 1};
    }
}

// ============ CRYPTO LIBRARY ============
namespace Crypto {
    // Simple XOR cipher
    Value xor_cipher(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
            return std::string("");
        }

        std::string data = args[0].get<std::string>();
        std::string key = args[1].get<std::string>();

        if (key.empty()) return data;

        std::string result = data;
        for (size_t i = 0; i < data.size(); ++i) {
            result[i] = data[i] ^ key[i % key.size()];
        }

        return result;
    }

    // Simple hash function (not cryptographically secure)
    Value simple_hash(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return 0;
        }

        std::string str = args[0].get<std::string>();
        size_t hash = 0;
        for (char c : str) {
            hash = hash * 31 + static_cast<size_t>(c);
        }

        return static_cast<int>(hash % 1000000007);
    }

    // Random bytes generation
    Value random_bytes(std::vector<Value> args) {
        int count = 16;  // default 16 bytes
        if (!args.empty() && args[0].holds_alternative<int>()) {
            count = args[0].get<int>();
        }

        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 255);

        std::string result;
        for (int i = 0; i < count; ++i) {
            result += static_cast<char>(dis(gen));
        }

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
    // Base64 encoding table
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    Value base64_encode(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return std::string("");
        }

        const std::string& input = args[0].get<std::string>();
        std::string ret;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (char c : input) {
            char_array_3[i++] = c;
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for (int j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (int j = 0; j < i + 1; j++)
                ret += base64_chars[char_array_4[j]];

            while (i++ < 3)
                ret += '=';
        }

        return ret;
    }

    Value hex_encode(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return std::string("");
        }

        const std::string& input = args[0].get<std::string>();
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (unsigned char c : input) {
            ss << std::setw(2) << static_cast<int>(c);
        }

        return ss.str();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["encoding_base64_encode"] = NativeFunction{base64_encode, 1};
        globals["encoding_hex_encode"] = NativeFunction{hex_encode, 1};
    }
}

// ============ LOG LIBRARY ============
namespace Log {
    Value info(std::vector<Value> args) {
        std::cout << "[INFO] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) {
                std::cout << arg.get<std::string>();
            } else if (arg.holds_alternative<int>()) {
                std::cout << arg.get<int>();
            } else if (arg.holds_alternative<double>()) {
                std::cout << arg.get<double>();
            }
        }
        std::cout << std::endl;
        return Value();
    }

    Value warn(std::vector<Value> args) {
        std::cout << "[WARN] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) {
                std::cout << arg.get<std::string>();
            } else if (arg.holds_alternative<int>()) {
                std::cout << arg.get<int>();
            } else if (arg.holds_alternative<double>()) {
                std::cout << arg.get<double>();
            }
        }
        std::cout << std::endl;
        return Value();
    }

    Value error(std::vector<Value> args) {
        std::cerr << "[ERROR] ";
        for (const auto& arg : args) {
            if (arg.holds_alternative<std::string>()) {
                std::cerr << arg.get<std::string>();
            } else if (arg.holds_alternative<int>()) {
                std::cerr << arg.get<int>();
            } else if (arg.holds_alternative<double>()) {
                std::cerr << arg.get<double>();
            }
        }
        std::cerr << std::endl;
        return Value();
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["log_info"] = NativeFunction{info, 1};
        globals["log_warn"] = NativeFunction{warn, 1};
        globals["log_error"] = NativeFunction{error, 1};
    }
}

// ============ ENV LIBRARY ============
namespace Env {
    Value getEnv(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return std::string("");
        }

        std::string name = args[0].get<std::string>();
        const char* value = std::getenv(name.c_str());

        if (value == nullptr) {
            return std::string("");
        }

        return std::string(value);
    }

    Value setEnv(std::vector<Value> args) {
        if (args.size() < 2 || !args[0].holds_alternative<std::string>() || !args[1].holds_alternative<std::string>()) {
            return false;
        }

        std::string name = args[0].get<std::string>();
        std::string value = args[1].get<std::string>();

        #ifdef _WIN32
        return _putenv_s(name.c_str(), value.c_str()) == 0;
        #else
        return setenv(name.c_str(), value.c_str(), 1) == 0;
        #endif
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["env_get"] = NativeFunction{getEnv, 1};
        globals["env_set"] = NativeFunction{setEnv, 2};
    }
}

// ============ PROCESS LIBRARY ============
namespace Process {
    // Execute command and return exit code
    Value exec(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return -1;
        }

        std::string command = args[0].get<std::string>();
        return system(command.c_str());
    }

    // Execute command and return output as string
    Value shell(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return std::string("");
        }

        std::string command = args[0].get<std::string>();
        std::string result;

        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return std::string("");
        }

        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }

        pclose(pipe);
        return result;
    }

    // Execute command with arguments (safe)
    Value spawn(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return -1;
        }

        std::string command = args[0].get<std::string>();

        // Build command with arguments if provided
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].holds_alternative<std::string>()) {
                command += " " + args[i].get<std::string>();
            }
        }

        return system(command.c_str());
    }

    // Get current working directory
    Value cwd(std::vector<Value> args) {
        char buffer[1024];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            return std::string(buffer);
        }
        return std::string("");
    }

    // Change directory
    Value chdir_fn(std::vector<Value> args) {
        if (args.empty() || !args[0].holds_alternative<std::string>()) {
            return -1;
        }

        std::string path = args[0].get<std::string>();
        return chdir(path.c_str());
    }

    void registerFunctions(std::unordered_map<std::string, Value>& globals) {
        globals["process_exec"] = NativeFunction{exec, 1};
        globals["process_shell"] = NativeFunction{shell, 1};
        globals["process_spawn"] = NativeFunction{spawn, -1};  // Variable args
        globals["process_cwd"] = NativeFunction{cwd, 0};
        globals["process_chdir"] = NativeFunction{chdir_fn, 1};
    }
}

// Placeholder implementations for remaining libraries
namespace Thread { void registerFunctions(std::unordered_map<std::string, Value>&) {} }
namespace Net { void registerFunctions(std::unordered_map<std::string, Value>&) {} }
namespace HTTP { void registerFunctions(std::unordered_map<std::string, Value>&) {} }
namespace Runtime { void registerFunctions(std::unordered_map<std::string, Value>&) {} }

} // namespace YenNative
