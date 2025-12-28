# YEN Programming Language

<div align="center">

![YEN Logo](https://avatars.githubusercontent.com/u/209428205?s=200&v=4)

**A modern, expressive systems programming language**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![LLVM](https://img.shields.io/badge/LLVM-17-orange.svg)](https://llvm.org/)

[Getting Started](#-quick-start) •
[Documentation](docs/) •
[Examples](examples/) •
[Contributing](CONTRIBUTING.md)

</div>

---

## Overview

YEN is a statically-typed systems programming language that combines the performance of compiled languages with modern features like pattern matching, lambda expressions, and memory safety through RAII-style defer statements.

**Key Highlights:**
- 🎯 Powerful pattern matching with guards and destructuring
- ⚡ Lambda expressions with closure support
- 🛡️ Memory safety without garbage collection
- 📦 Rich standard library with 12+ modules
- 🚀 Dual execution modes: interpreter and LLVM-based compiler
- 🎨 Clean, expressive syntax

---

## ✨ Features

### Language Features

```yen
// Pattern matching with ranges and guards
match (score) {
    0..60 => print "F";
    60..70 => print "D";
    70..80 => print "C";
    80..90 => print "B";
    90..=100 => print "A";
    _ => print "Invalid";
}

// Lambda expressions with closures
var multiplier = 10;
let multiply = |x| x * multiplier;
print multiply(5);  // 50

// RAII-style resource management
func process() {
    defer print "Cleanup!";  // Executes on scope exit
    // ... work ...
}

// Range-based iteration
for i in 0..10 {      // Exclusive: 0 to 9
    print i;
}

for i in 1..=5 {      // Inclusive: 1 to 5
    print i;
}
```

### Type System

```yen
// Strong static typing with inference
var x = 10;           // Inferred as int
let pi = 3.14;        // Inferred as float

// Explicit type annotations
var name: string = "Alice";
var age: int = 30;
var price: float = 19.99;

// Immutability by default with 'let'
let constant = 42;    // Cannot be reassigned
var mutable = 10;     // Can be reassigned
```

### Pattern Matching

```yen
// Literal patterns
match (x) {
    42 => print "The answer";
    _ => print "Something else";
}

// Range patterns
match (age) {
    0..18 => print "Minor";
    18..65 => print "Adult";
    _ => print "Senior";
}

// Guards
match (value) {
    n when n > 0 => print "Positive";
    n when n < 0 => print "Negative";
    _ => print "Zero";
}

// Variable binding
match (result) {
    value => print "Got: " + value;
}
```

### Classes & Structs

```yen
class Person {
    let name;
    let age;

    func greet() {
        print "Hello, I'm " + this.name;
    }
}

struct Point {
    x;
    y;
}

var p = Person { name: "Alice", age: 30 };
p.greet();
```

---

## 📚 Standard Library

YEN includes a comprehensive standard library:

| Module | Description | Key Functions |
|--------|-------------|---------------|
| **Core** | Type checking & conversion | `core_is_int`, `core_to_string`, `core_is_list` |
| **Math** | Mathematical operations | `math_sqrt`, `math_pow`, `math_sin`, `math_random` |
| **String** | String manipulation | `str_split`, `str_upper`, `str_replace`, `str_trim` |
| **Collections** | List operations | `list_sort`, `list_reverse`, `list_push` |
| **IO** | File I/O | `io_read_file`, `io_write_file` |
| **FS** | Filesystem operations | `fs_exists`, `fs_create_dir`, `fs_file_size` |
| **Time** | Time utilities | `time_now`, `time_sleep` |
| **Crypto** | Cryptography basics | `crypto_xor`, `crypto_hash`, `crypto_random_bytes` |
| **Encoding** | Data encoding | `encoding_base64_encode`, `encoding_hex_encode` |
| **Log** | Logging system | `log_info`, `log_warn`, `log_error` |
| **Env** | Environment variables | `env_get`, `env_set` |
| **Process** | Process execution | `process_exec` |

**Example:**

```yen
// File processing with standard library
var content = io_read_file("data.txt");
var lines = str_split(content, "\n");

for line in lines {
    var upper = str_upper(line);
    log_info("Processing: " + upper);
}

io_write_file("output.txt", str_join(lines, "\n"));
```

---

## 🚀 Quick Start

### Prerequisites

- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.10 or higher
- **LLVM 17+** (optional, for compiler)

### Installation

```bash
# Clone the repository
git clone https://github.com/yen-lang/Yen.git
cd Yen

# Build
mkdir build && cd build
cmake ..
make

# Verify installation
./yen --version
```

### Your First Program

Create `hello.yen`:

```yen
print "Hello, World!";
```

Run it:

```bash
./yen hello.yen
```

### Compile to Native Executable

```bash
# Compile (requires LLVM)
./yenc hello.yen -o hello

# Run compiled executable
./hello
```

---

## 📖 Documentation

- **[Quick Start Guide](docs/QUICKSTART.md)** - Get started in 5 minutes
- **[Language Syntax](docs/SYNTAX.md)** - Complete syntax reference
- **[Standard Library](docs/STDLIB.md)** - All built-in functions
- **[Examples](examples/)** - Sample programs and use cases

---

## 💡 Code Examples

### Fibonacci Sequence

```yen
func fibonacci(n: int) -> int {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

for i in 0..10 {
    print "fib(" + i + ") = " + fibonacci(i);
}
```

### File Analysis

```yen
func analyze_log(path: string) {
    assert(fs_exists(path), "File not found");

    defer log_info("Analysis complete");

    var content = io_read_file(path);
    var lines = str_split(content, "\n");

    var errors = 0;
    var warnings = 0;

    for line in lines {
        if (str_contains(line, "ERROR")) {
            errors = errors + 1;
        } else if (str_contains(line, "WARN")) {
            warnings = warnings + 1;
        }
    }

    log_info("Errors: " + errors);
    log_info("Warnings: " + warnings);
}
```

### Data Processing with Lambdas

```yen
var numbers = [1, 2, 3, 4, 5];

// Transform with lambda
let square = |x| x * x;
for n in numbers {
    print "Square of " + n + " is " + square(n);
}

// Sort and analyze
var data = [23, 45, 12, 67, 34];
var sorted = list_sort(data);

print "Min: " + sorted[0];
print "Max: " + sorted[list_length(sorted) - 1];
```

---

## 🏗️ Architecture

```
yen/
├── include/yen/          # Public headers
│   ├── ast.h             # Abstract Syntax Tree
│   ├── compiler.h        # Interpreter
│   ├── lexer.h           # Lexical analyzer
│   ├── parser.h          # Parser
│   ├── value.h           # Value system
│   ├── native_libs.h     # Native library interface
│   └── llvm_codegen.h    # LLVM backend
│
├── src/                  # Implementation
│   ├── main.cpp          # Interpreter entry
│   ├── compiler_main.cpp # Compiler entry
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── compiler.cpp
│   ├── native_libs.cpp   # Standard library
│   └── llvm_codegen.cpp  # Code generation
│
├── examples/             # Example programs
├── tests/                # Test suite
└── docs/                 # Documentation
```

---

## 🧪 Testing

```bash
# Run test suite
cd tests
./run_type_tests.sh

# Run specific tests
../build/yen test_arithmetic.yen
../build/yen test_functions.yen
../build/yen test_loops.yen

# Run examples
cd examples
../build/yen fibonacci.yen
../build/yen features_demo.yen
```

---

## 🛣️ Roadmap

### ✅ Current Features
- [x] Complete lexer and parser
- [x] Pattern matching (literals, ranges, wildcards, guards)
- [x] Lambda expressions with closures
- [x] Range expressions (.. and ..=)
- [x] Defer statements (RAII)
- [x] Assertions
- [x] Classes and structs
- [x] Enums with match
- [x] 12 standard library modules
- [x] LLVM compiler backend
- [x] Type system with annotations

### 🚧 In Development
- [ ] Generics/Templates
- [ ] Traits and interfaces
- [ ] Advanced error handling (Result<T, E>, Option<T>)
- [ ] Module system improvements
- [ ] Package manager

### 📅 Future Plans
- [ ] Async/await support
- [ ] Language Server Protocol (LSP)
- [ ] Debugging tools
- [ ] Performance optimizations
- [ ] More platform APIs

---

## 🤝 Contributing

We welcome contributions! Here's how you can help:

- 🐛 **Report bugs** - Open an issue
- ✨ **Suggest features** - Share your ideas
- 📖 **Improve docs** - Help others learn
- 🧪 **Write tests** - Improve reliability
- 💻 **Submit PRs** - Add features or fix bugs

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## 📊 Performance

YEN offers two execution modes:

| Mode | Use Case | Performance | Startup Time |
|------|----------|-------------|--------------|
| **Interpreter** (`yen`) | Development, scripting | Fast iteration | Instant |
| **Compiler** (`yenc`) | Production, deployment | Native speed | AOT compilation |

**Compiler optimizations:**
- LLVM optimization passes
- Native code generation
- Dead code elimination
- Inline expansion

---

## 📄 License

YEN is licensed under the [MIT License](LICENSE).

```
MIT License

Copyright (c) 2024 YEN Programming Language

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

---

## 🌟 Community

- **GitHub**: [github.com/yen-lang/Yen](https://github.com/yen-lang/Yen)
- **Issues**: [Report bugs or request features](https://github.com/yen-lang/Yen/issues)
- **Discussions**: [Join the conversation](https://github.com/yen-lang/Yen/discussions)

---

## 🙏 Acknowledgments

YEN is inspired by modern language design and built with:
- **LLVM** - Compiler infrastructure
- **C++17** - Implementation language
- Influences from **Rust**, **Python**, and **NIM**

---

## 📞 Support

Need help?

1. Check the [documentation](docs/)
2. Browse [examples](examples/)
3. Search [existing issues](https://github.com/yen-lang/Yen/issues)
4. Ask in [discussions](https://github.com/yen-lang/Yen/discussions)
5. Open a [new issue](https://github.com/yen-lang/Yen/issues/new)

---

<div align="center">

**Built with ❤️ for developers who value simplicity and power**

⭐ Star us on GitHub if you find YEN useful!

[Getting Started](#-quick-start) •
[Documentation](docs/) •
[Examples](examples/) •
[Contributing](CONTRIBUTING.md)

</div>
