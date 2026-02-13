#ifndef NATIVE_LIBS_H
#define NATIVE_LIBS_H

#pragma once

#include "yen/value.h"
#include <vector>
#include <unordered_map>
#include <string>

// Native library registration system
namespace YenNative {

// Library registration function type
using LibraryInitializer = void(*)(std::unordered_map<std::string, Value>&);

// Register all native libraries
void registerAllLibraries(std::unordered_map<std::string, Value>& globals);

// Individual library registrars
namespace Core {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Math {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace String {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Collections {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Map {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Json {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Utility {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace IO {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace FS {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Time {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Crypto {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Encoding {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Log {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Env {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Process {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Thread {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Net {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace HTTP {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Runtime {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Platform {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

// ---- NEW: Standard library modules (loaded on-demand via import) ----

namespace Regex {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace NetSocket {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace NetHTTP {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace OS {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Async {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace DateTime {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Testing {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Color {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Set {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Path {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace CSV {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

namespace Event {
    void registerFunctions(std::unordered_map<std::string, Value>& globals);
}

} // namespace YenNative

#endif // NATIVE_LIBS_H
