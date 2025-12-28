# YEN Compiler Status

## Execution Modes

YEN provides two execution modes:

### ✅ Interpreter Mode (`yen`) - **FULLY FUNCTIONAL**

The interpreter is **100% functional** and supports all language features and standard library functions.

**Usage:**
```bash
./yen script.yen
```

**Features:**
- ✅ All language syntax (variables, functions, classes, structs)
- ✅ Pattern matching with guards and ranges
- ✅ Lambda expressions with closures
- ✅ Defer statements
- ✅ **12 Standard Library Modules** (Core, Math, String, Collections, IO, FS, Time, Crypto, Encoding, Log, Env, Process)
- ✅ **Shell command execution** (process_shell, process_exec, process_cwd, process_chdir)
- ✅ File I/O and filesystem operations
- ✅ All built-in functions

**Performance:**
- Fast startup time (instant)
- Ideal for development, scripting, and prototyping
- No compilation step required

**Example:**
```bash
# Run any YEN script
./yen examples/shell_commands.yen
./yen examples/system_automation.yen
./yen examples/fibonacci.yen
```

---

### ⚠️ Compiler Mode (`yenc`) - **IN DEVELOPMENT**

The LLVM-based ahead-of-time compiler is currently under development.

**Status:** Not yet functional for production use

**Planned Features:**
- LLVM IR generation
- Native executable output
- Optimization passes
- Cross-platform compilation
- Near-C performance

**Current State:**
- Type checker implemented ✅
- Basic LLVM infrastructure in place ✅
- Code generation incomplete ⚠️
- Standard library linking needed ⚠️

---

## Recommendations

### For Development and Scripting: Use `yen` (Interpreter)

The interpreter is recommended for:
- **Development** - Fast iteration cycles
- **Scripting** - System automation, file processing
- **Testing** - Quick testing of code
- **Shell integration** - Running shell commands
- **Learning YEN** - Interactive experimentation

**All examples work perfectly with the interpreter!**

### For Production (Future): Use `yenc` (Compiler)

Once completed, the compiler will be recommended for:
- Production deployments
- Maximum performance
- Standalone executables
- Distribution to end users

---

## Running Examples

All examples in the `examples/` directory work with the interpreter:

```bash
# Basic examples
./yen examples/hello.yen
./yen examples/fibonacci.yen
./yen examples/features_demo.yen

# Shell automation examples
./yen examples/os_shell_simple.yen
./yen examples/shell_commands.yen
./yen examples/system_automation.yen

# Library examples
./yen examples/collections_demo.yen
./yen examples/string_demo.yen
```

---

## Standard Library Availability

### Interpreter (`yen`): ✅ ALL LIBRARIES AVAILABLE

All 12 standard library modules are fully functional:

| Module | Status | Available Functions |
|--------|--------|-------------------|
| **Core** | ✅ | Type checking, conversions |
| **Math** | ✅ | sqrt, pow, sin, cos, random, PI, E |
| **String** | ✅ | split, join, upper, lower, trim, replace, contains |
| **Collections** | ✅ | push, pop, length, reverse, sort |
| **IO** | ✅ | read_file, write_file, append_file |
| **FS** | ✅ | exists, is_directory, is_file, create_dir, remove |
| **Time** | ✅ | now, sleep |
| **Crypto** | ✅ | xor, hash, random_bytes |
| **Encoding** | ✅ | base64_encode, hex_encode |
| **Log** | ✅ | info, warn, error |
| **Env** | ✅ | get, set |
| **Process** | ✅ | **exec, shell, spawn, cwd, chdir** |

### Compiler (`yenc`): ⚠️ IN DEVELOPMENT

Standard library linking not yet implemented.

---

## Development Roadmap

### Phase 1: Interpreter (COMPLETED ✅)
- [x] Lexer and Parser
- [x] AST Implementation
- [x] Runtime interpreter
- [x] Type system
- [x] Pattern matching
- [x] Lambda expressions
- [x] Defer statements
- [x] 12 Standard library modules
- [x] Shell command execution

### Phase 2: Compiler (IN PROGRESS ⚠️)
- [x] Type checker
- [x] LLVM infrastructure setup
- [ ] Complete LLVM IR code generation
- [ ] Standard library linking
- [ ] Native function binding
- [ ] Optimization passes
- [ ] Debug information generation

### Phase 3: Advanced Features (PLANNED 📅)
- [ ] Ownership and borrowing
- [ ] Memory safety guarantees
- [ ] Async/await runtime
- [ ] Package manager
- [ ] Module system improvements
- [ ] Language server protocol (LSP)

---

## FAQ

### Q: Can I use YEN in production now?

**A:** Yes, if you use the **interpreter mode** for scripting and automation tasks. The interpreter is stable and fully functional.

For compiled binaries with maximum performance, wait for the compiler to be completed.

### Q: Do all standard library functions work?

**A:** Yes! All 12 library modules are **100% functional** in interpreter mode, including the new Process/Shell functions.

### Q: When will the compiler be ready?

**A:** The compiler is being actively developed. Follow the GitHub repository for updates.

### Q: Can I contribute to the compiler development?

**A:** Absolutely! Contributions are welcome. See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

### Q: Which execution mode should I use?

**A:** Use the **interpreter (`yen`)** for now. It's fast, fully functional, and supports all features.

---

## Performance Comparison

| Metric | Interpreter (`yen`) | Compiler (`yenc`) |
|--------|-------------------|------------------|
| Startup Time | Instant | N/A (not ready) |
| Iteration Speed | Fast | N/A |
| Runtime Performance | Good for scripting | Native speed (planned) |
| Standard Library | ✅ Full support | ⚠️ In development |
| Use Case | Development, scripting | Production (future) |
| Status | **READY TO USE** | **IN DEVELOPMENT** |

---

## Example: Using the Interpreter

```bash
# Write a YEN script
cat > hello.yen << 'EOF'
// Shell command example
var files = process_shell("ls -la");
print "Files:";
print files;

// Math operations
var result = math_sqrt(16);
print "sqrt(16) = " + result;

// String operations
var words = str_split("Hello,World,YEN", ",");
print "Words:";
print words;
EOF

# Run with interpreter (WORKS PERFECTLY!)
./yen hello.yen
```

---

## Conclusion

**YEN Interpreter is production-ready for scripting and automation!**

Use `yen` (interpreter) for all your development needs. The compiler will provide additional performance benefits when completed.

For the latest updates, check:
- [GitHub Repository](https://github.com/yen-lang/Yen)
- [Documentation](README.md)
- [Examples](../examples/)

---

**Happy coding with YEN! 🚀**
