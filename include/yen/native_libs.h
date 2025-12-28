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

} // namespace YenNative

#endif // NATIVE_LIBS_H
