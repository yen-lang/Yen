# YEN Compiler Quick Start Guide

## Installation Verification

Check that LLVM and yenc are installed:

```bash
# Check LLVM version
/usr/local/llvm-17/bin/llvm-config --version
# Should output: 17.0.6

# Check compiler exists
ls -lh /opt/Yen/build/yenc
# Should show: -rwxr-xr-x  35M  yenc
```

## Basic Usage

### 1. Type Check Your Code

Before compiling, verify types are correct:

```bash
cd /opt/Yen/build
./yenc ../examples/test_simple.yen --type-check
```

**Output if successful:**
```
Type checking successful!
```

**Output if type error:**
```
Type error: Type mismatch in variable 'x': declared as int32 but initialized with string
```

### 2. Generate LLVM IR

Compile YEN code to LLVM intermediate representation:

```bash
./yenc ../examples/test_simple.yen --emit-llvm
```

This creates `output.ll` with LLVM IR code.

**View the generated IR:**
```bash
cat output.ll
```

### 3. Verbose Mode

See all compilation stages:

```bash
./yenc ../examples/test_simple.yen --emit-llvm -v
```

**Output:**
```
YEN Compiler v0.1.0
Input file: ../examples/test_simple.yen
Output file: output.ll
Optimization level: 2

[1/4] Lexing...
Generated 29 tokens

[2/4] Parsing...
Generated AST with 1 top-level statements

[3/4] Type checking...
Type checking passed

[4/4] Generating code...
Code generation successful
Optimizing (level 2)...
Emitting LLVM IR to output.ll...

Compilation successful!
```

### 4. Optimization Levels

Control optimization:

```bash
# No optimization
./yenc test.yen --emit-llvm --opt=0

# Default (level 2)
./yenc test.yen --emit-llvm

# Maximum optimization
./yenc test.yen --emit-llvm --opt=3
```

## What You Can Compile

### ✅ Supported Features

**Basic Types:**
- `int32`, `int64`, `float64`, `bool`

**Variables:**
```yen
let x: int32 = 42;
let pi: float64 = 3.14159;
let flag: bool = true;
```

**Functions:**
```yen
func add(a: int32, b: int32) -> int32 {
    return a + b;
}
```

**Arithmetic:**
```yen
let sum: int32 = a + b;
let product: int32 = x * y;
let diff: int32 = a - b;
let quotient: float64 = x / y;
```

**Control Flow:**
```yen
// If/else
if (x > 10) {
    return 1;
} else {
    return 0;
}

// While loops
let i: int32 = 0;
while (i < 10) {
    // body
}
```

### ❌ Not Yet Supported

- Function calls (inter-function)
- Print statements
- String literals
- Arrays
- For loops
- Structs/Classes
- Break/continue
- Native executable output

## Example Workflow

### Write YEN Code

Create `myprogram.yen`:
```yen
func calculate() -> int32 {
    let x: int32 = 10;
    let y: int32 = 20;
    let result: int32 = x + y;
    return result;
}
```

### Compile Step-by-Step

```bash
cd /opt/Yen/build

# 1. Type check
./yenc ../myprogram.yen --type-check
# ✅ Type checking successful!

# 2. Generate IR
./yenc ../myprogram.yen --emit-llvm -v
# ✅ Compilation successful!

# 3. View output
cat output.ll
```

### Expected LLVM IR

```llvm
define i32 @calculate() {
entry:
  %x = alloca i32, align 4
  store i32 10, ptr %x, align 4
  %y = alloca i32, align 4
  store i32 20, ptr %y, align 4
  %result = alloca i32, align 4
  %x1 = load i32, ptr %x, align 4
  %y2 = load i32, ptr %y, align 4
  %addtmp = add i32 %x1, %y2
  store i32 %addtmp, ptr %result, align 4
  %result3 = load i32, ptr %result, align 4
  ret i32 %result3
}
```

## Common Errors

### 1. Type Mismatch

**Error:**
```
Type error: Type mismatch in variable 'x': declared as int32 but initialized with string
```

**Fix:**
```yen
// Wrong:
let x: int32 = "hello";

// Right:
let x: int32 = 42;
```

### 2. Undefined Variable

**Error:**
```
Codegen error: Undefined variable: result
```

**Fix:**
Ensure variable is declared before use:
```yen
let result: int32 = 0;
return result;
```

### 3. Function Call Not Implemented

**Error:**
```
Codegen error: Call expression codegen not yet implemented
```

**Workaround:**
Avoid calling functions for now. Inline the code instead:
```yen
// Instead of:
let result: int32 = add(10, 20);

// Use:
let a: int32 = 10;
let b: int32 = 20;
let result: int32 = a + b;
```

### 4. Print Not Working

**Error:**
```
Codegen error: Failed to generate code for print statement
```

**Workaround:**
Remove print statements for now:
```yen
// Instead of:
print result;

// Just:
return result;
```

## Testing Your Installation

Run the test examples:

```bash
cd /opt/Yen/build

# Test 1: Simple function
./yenc ../examples/test_simple.yen --emit-llvm
cat output.ll  # Should show LLVM IR

# Test 2: Type checking
./yenc ../examples/test_main.yen --type-check
# Should output: Type checking successful!

# Test 3: Verbose compilation
./yenc ../examples/working_examples.yen --emit-llvm -v
# Should show all 4 compilation stages
```

## Next Steps

1. **Read the full status**: See `LLVM_COMPILER_STATUS.md` for complete feature list
2. **Try examples**: Check `examples/working_examples.yen` for code that works
3. **Learn type system**: Read `TYPE_ANNOTATIONS.md` for type annotation syntax
4. **Run type tests**: Execute `tests/run_type_tests.sh` to verify type checker

## File Locations

```
/opt/Yen/
├── build/
│   ├── yen          # Interpreter (8.5MB)
│   └── yenc         # Compiler (35MB)
├── examples/
│   ├── test_simple.yen
│   ├── test_main.yen
│   ├── working_examples.yen
│   └── not_yet_working.yen
├── LLVM_COMPILER_STATUS.md
├── TYPE_ANNOTATIONS.md
└── TYPE_CHECKER_DEMO.md
```

## Troubleshooting

### LLVM Not Found

If CMake can't find LLVM:
```bash
# Check LLVM location
ls /usr/local/llvm-17/

# Reconfigure CMake
cd /opt/Yen/build
cmake ..
```

### Build Errors

If compilation fails:
```bash
# Clean and rebuild
cd /opt/Yen/build
make clean
cmake ..
make -j4
```

### Can't Run yenc

If executable not found:
```bash
# Use absolute path
/opt/Yen/build/yenc --help

# Or add to PATH
export PATH="/opt/Yen/build:$PATH"
yenc --help
```

## Getting Help

- Check `LLVM_COMPILER_STATUS.md` for feature status
- Check `examples/not_yet_working.yen` for known limitations
- Check `TYPE_CHECKER_DEMO.md` for type system examples

---

**Happy compiling! 🚀**
