# How to Use YEN

## ✅ Recommended Mode: Interpreter (`yen`)

The **YEN interpreter is 100% functional** and is the recommended way to run YEN programs.

---

## Running Programs

```bash
# Basic syntax
./yen script.yen

# Working examples
./yen examples/hello.yen
./yen examples/fibonacci.yen
./yen examples/shell_commands.yen
./yen examples/system_automation.yen
```

---

## All Available Features

The interpreter supports **ALL** language features:

### ✅ Full Syntax Support

* Variables (`var`, `let`)
* Functions
* Classes and Structs
* Pattern matching
* Lambda expressions
* Defer statements
* Loops and conditionals

### ✅ 12 Standard Libraries

* **Core** (type checking, conversion)
* **Math** (sqrt, pow, trigonometry, random)
* **String** (split, join, upper, lower, replace)
* **Collections** (push, pop, sort, reverse)
* **IO** (read_file, write_file)
* **FS** (exists, create_dir, remove)
* **Time** (now, sleep)
* **Crypto** (xor, hash, random_bytes)
* **Encoding** (base64, hex)
* **Log** (info, warn, error)
* **Env** (get, set)
* **Process** (exec, shell, spawn, cwd, chdir) ⭐

---

## Usage Examples

### 1. Shell Commands

```bash
./yen examples/os_shell_simple.yen
```

```yen
// Execute shell commands
var files = process_shell("ls -la");
print files;

// Current directory
var dir = process_cwd();
print "Working in: " + dir;

// Execute and check exit code
var result = process_exec("mkdir /tmp/test");
if (result == 0) {
    print "Success!";
}
```

---

### 2. System Automation

```bash
./yen examples/system_automation.yen
```

See the full file for examples such as:

* Automatic backups
* Disk monitoring
* Logging systems
* Temporary file cleanup
* System reports

---

### 3. Data Processing

```yen
// Read and process a file
var content = io_read_file("data.txt");
var lines = str_split(content, "\n");

for line in lines {
    if (str_contains(line, "ERROR")) {
        log_error(line);
    }
}
```

---

### 4. Mathematical Operations

```yen
var result = math_sqrt(16);
print "sqrt(16) = " + result;  // 4.0

var random = math_random();
print "Random: " + random;  // 0.0 to 1.0
```

---

## ⚠️ Compiler (`yenc`) – UNDER DEVELOPMENT

The LLVM compiler is under development and **should not be used** at this time.

---

### Current Status

* ❌ **NOT FUNCTIONAL** – runtime segmentation fault
* ⚠️ Type checker implemented
* ⚠️ LLVM infrastructure partially complete
* ⚠️ Code generation incomplete

---

### What Is Missing

1. Critical bug fixes (segfaults)
2. Full LLVM IR generation
3. Native library linking
4. Runtime for dynamic values
5. Support for complex types (lists, dynamic strings)
6. Pattern matching code generation
7. Lambda and closure support
8. Optimizations

---

### Why It Does Not Work Yet

The LLVM compiler is significantly more complex than the interpreter because:

1. **Dynamic vs Static Typing**
   YEN uses dynamic typing at runtime, while LLVM requires static types. A boxing/unboxing runtime system is required.

2. **Dynamic Values**
   The `Value` system (variant holding int, double, string, list, etc.) must be represented in LLVM IR using tagged unions or vtables.

3. **Native Libraries**
   C++ native functions must be properly linked with the generated code.

4. **Memory Management**
   Dynamically allocated strings and lists require a memory runtime.

5. **Closures**
   Lambdas with captures require closure structures and trampolines.

---

## 📋 Recommendations

### For Development

**Use the interpreter (`yen`)**

```bash
./yen my_script.yen
```

---

### For Production (Current)

**Use the interpreter with a wrapper script**

```bash
#!/bin/bash
# deploy.sh

cp build/yen /opt/myapp/
cp scripts/main.yen /opt/myapp/

cd /opt/myapp
./yen main.yen
```

---

### For Production (Future)

Once the compiler is complete:

```bash
./yenc main.yen -o myapp --opt=3
./myapp
```

---

## 🚀 Performance

### Interpreter

**Advantages**

* Instant startup
* Fast iteration
* Ideal for scripting and automation
* No compilation step

**Use Cases**

* Automation scripts
* File processing
* System administration
* Prototyping
* DevOps and CI/CD

---

### Compiler (Planned)

**Expected Advantages**

* Near-C performance
* Standalone executable
* LLVM optimizations
* Ideal for long-running applications

---

## 📊 Comparison

| Feature          | Interpreter          | Compiler         |
| ---------------- | -------------------- | ---------------- |
| Status           | ✅ Working            | ❌ In Development |
| Startup          | Instant              | N/A              |
| Performance      | Good                 | N/A              |
| Standard Library | ✅ Complete           | ❌                |
| Shell Commands   | ✅ Yes                | ❌                |
| Pattern Matching | ✅ Yes                | ❌                |
| Lambdas          | ✅ Yes                | ❌                |
| Deployment       | Script + interpreter | N/A              |

---

## 💡 Tips

### 1. Use Shebang for Executable Scripts

```yen
#!/opt/Yen/build/yen
print "Hello from executable script!";
```

```bash
chmod +x script.yen
./script.yen
```

---

### 2. Project Structure

```
my_project/
├── src/
│   ├── main.yen
│   ├── utils.yen
│   └── config.yen
├── tests/
│   └── test_main.yen
└── run.sh
```

---

### 3. Debugging

```yen
func process_data(data) {
    print "[DEBUG] Processing: " + data;
    var result = transform(data);
    print "[DEBUG] Result: " + result;
    return result;
}
```

---

### 4. Error Handling

```yen
var result = process_exec("mkdir /tmp/mydir");
if (result != 0) {
    log_error("Failed to create directory!");
    return;
}

if (!fs_exists("config.txt")) {
    log_warn("Config file not found, using defaults");
}
```

---

## 🔗 More Information

* `README.md` – Project overview
* `SYNTAX.md` – Full language syntax
* `STDLIB.md` – Standard library reference
* `PROCESS_SHELL.md` – Shell command guide
* `COMPILER_STATUS.md` – Detailed compiler status

---

## ❓ FAQ

**Q: Can I use YEN in production?**
Yes. Use the interpreter for scripts and automation.

**Q: Is the interpreter reliable?**
Yes. It is fully implemented and tested.

**Q: When will the compiler be ready?**
It is a work in progress. Follow the GitHub repository for updates.

**Q: Is the interpreter slow?**
No. Performance is excellent for scripting and automation.

**Q: Can I contribute?**
Yes. Contributions are welcome. See `CONTRIBUTING.md`.

---

## ✅ Conclusion

**Use the YEN interpreter (`yen`) today.**

It is production-ready for:

* Automation scripts
* System administration
* File processing
* Shell integration
* Rapid prototyping
* DevOps and CI/CD

The compiler will arrive later for high-performance use cases, but the interpreter is already a powerful and complete tool.

**Happy coding with YEN.**
