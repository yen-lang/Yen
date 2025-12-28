# YEN Programming Language

YEN is a modern, statically-typed systems programming language with a focus on simplicity, safety, and performance.

## Features

- **Pattern Matching** - Powerful pattern matching with guards, ranges, and destructuring
- **Lambda Expressions** - First-class functions with closure support
- **Memory Safety** - Defer statements for RAII-style resource management
- **Rich Standard Library** - Comprehensive native libraries for common tasks
- **Type System** - Strong static typing with type inference
- **LLVM Backend** - AOT compilation with world-class optimizations

## Quick Start

### Building

```bash
mkdir build && cd build
cmake ..
make
```

This will create two executables:
- `yen` - The interpreter
- `yenc` - The compiler (requires LLVM 17+)

### Hello World

```yen
print "Hello, World!";
```

Run with:
```bash
./yen hello.yen
```

## Language Features

### Variables and Types

```yen
// Mutable variable
var x = 10;

// Immutable variable
let y = 20;

// Type annotations
var name: string = "YEN";
let count: int = 42;
```

### Functions

```yen
func add(a: int, b: int) -> int {
    return a + b;
}

func greet(name: string) {
    print "Hello, " + name;
}
```

### Lambda Expressions

```yen
let square = |x| x * x;
print square(5);  // 25

// With closures
var multiplier = 10;
let multiply = |x| x * multiplier;
print multiply(5);  // 50
```

### Pattern Matching

```yen
var x = 42;

match (x) {
    0 => print "zero";
    1..10 => print "single digit";
    10..=100 => print "two digits";
    _ => print "other";
}
```

### Range Expressions

```yen
// Exclusive range
for i in 0..5 {
    print i;  // 0, 1, 2, 3, 4
}

// Inclusive range
for i in 1..=5 {
    print i;  // 1, 2, 3, 4, 5
}
```

### Defer Statements

```yen
{
    defer print "Cleanup!";
    defer print "More cleanup!";
    print "Working...";
}
// Output:
// Working...
// More cleanup!
// Cleanup!
```

### Assertions

```yen
assert(x > 0, "x must be positive");
```

### Classes and Structs

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
```

## Standard Library

YEN comes with a comprehensive standard library:

### Core Library
```yen
core_is_int(42)           // Type checking
core_to_string(123)       // Type conversion
```

### Math Library
```yen
math_sqrt(16)             // 4.0
math_pow(2, 3)            // 8.0
math_random()             // Random [0, 1)
math_PI                   // 3.14159...
```

### String Library
```yen
str_upper("hello")        // "HELLO"
str_split("a,b,c", ",")   // ["a", "b", "c"]
str_replace(text, "old", "new")
```

### Collections Library
```yen
list_push([1, 2], 3)      // [1, 2, 3]
list_reverse([1, 2, 3])   // [3, 2, 1]
list_sort([3, 1, 2])      // [1, 2, 3]
```

### IO & Filesystem
```yen
io_write_file("file.txt", "content")
var content = io_read_file("file.txt")

fs_exists("file.txt")     // true/false
fs_create_dir("mydir")
```

### Time Library
```yen
var timestamp = time_now()
time_sleep(1000)          // Sleep 1 second
```

### Crypto Library
```yen
crypto_xor(data, key)     // XOR cipher
crypto_hash("text")       // Hash function
crypto_random_bytes(16)   // Random bytes
```

### Encoding Library
```yen
encoding_base64_encode("text")
encoding_hex_encode("data")
```

### Logging
```yen
log_info("Information message")
log_warn("Warning message")
log_error("Error message")
```

### Environment Variables
```yen
env_set("MY_VAR", "value")
var val = env_get("MY_VAR")
```

### Process & Shell Execution
```yen
// Execute command and get exit code
var exit_code = process_exec("mkdir /tmp/test")

// Execute command and capture output
var files = process_shell("ls -la")
print files

// Get current directory
var dir = process_cwd()

// Change directory
process_chdir("/tmp")

// Execute with arguments
process_spawn("git", "status")
```

## Examples

See the `examples/` directory for more examples:

### Basic Examples
- `hello.yen` - Hello World
- `fibonacci.yen` - Fibonacci sequence
- `features_demo.yen` - Language features showcase
- `modern_features_demo.yen` - Modern features demo

### Library Examples
- `collections_demo.yen` - Collections usage
- `string_demo.yen` - String manipulation

### Shell & System Examples
- `os_shell_simple.yen` - **Simple shell command examples**
- `shell_commands.yen` - **Comprehensive shell operations**
- `system_automation.yen` - **System automation utilities**

### Platform-Specific Examples
- Windows examples (console, file I/O, message boxes)
- macOS examples (system info, POSIX operations)
- POSIX examples (file I/O, processes)

## Building with LLVM (Optional)

To build the compiler with LLVM support:

1. Install LLVM 17+
2. Set `LLVM_DIR` in CMakeLists.txt if needed
3. Build as normal

The compiler (`yenc`) will be built if LLVM is detected.

## Requirements

- C++17 compatible compiler
- CMake 3.10+
- LLVM 17+ (optional, for compiler)

## License

This project is open source. See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Disclaimer

This is an educational project demonstrating language implementation concepts.
While functional, it is not intended for production use without further development and testing.
