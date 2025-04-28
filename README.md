# Yen Language

Yen is a **lightweight, dynamic and expressive programming language**, designed to combine the best ideas from Python, Rust, and C++. It prioritizes simplicity, fast interpretation, and modern programming structures like functions, structs, enums, classes, match/case, and more.

Currently, Yen Language runs only on **Linux**, but support for **Windows** and **macOS** is planned for the near future.

---

## Full Description

**Yen Language** is a programming language designed to be simple, modern, and powerful. Written in C++, it implements a custom lexer, parser, abstract syntax tree (AST), and an interpreter to execute source files (`.yl`).

The syntax of Yen is heavily inspired by Python and Rust, aiming for intuitive code while supporting advanced structures:

- **Variables** (`let`)  
- **Functions** (`func`)  
- **Structs and Classes** with fields and methods  
- **Enums** with match statements  
- **Switch/Case** control structures  
- **List, String, and Map manipulation**  
- **Object-oriented programming (OOP)** support via classes
- **Casting and Type Annotations** (basic level)
- **Logical and Arithmetic operations**
- **Dynamic typing with runtime type checking**

---

## Example

```
# Complete test of YenLang language with functions, conditionals, input, operations, and blocks
/* Block comment
   still here
   and ending below */
func test() {
    print "Working!";
}
test();

func greeting(name) {
    print "Hello, " + name + "!";
}

func add(a, b) {
    return a + b;
}

func check_majority(age) {
    if (age >= 18) {
        print "You are an adult.";
    } else {
        print "You are a minor.";
    }
}

func interact() {
    let name = input("What is your name?");
    let age = input<int>("What is your age?");
    greeting(name);
    check_majority(age);
    let result = add(10, 5);
    print "10 + 5 = " + result;
}

interact();

func test() {
    let pi = 3.14;
    let radius = 5;
    let area = pi * radius * radius;
    print "Circle area: " + area;
}
test();

```

## Features Planned

- **Windows and macOS support** in addition to Linux.
- **Class inheritance** (currently only basic classes are supported).
- **Better error handling and diagnostics**.
- **Native module system** for standard libraries.
- **Advanced types:** Generics, Interfaces.
- **Garbage collection or memory management improvements**.
- **Compiled mode** (ahead-of-time compilation) instead of just interpretation.
- **Foreign Function Interface (FFI)** to bind C libraries.
- **Multithreading and coroutines**.
- **Improved performance optimizations**.

---

## Current Status

- **Parser**: Fully operational.
- **Lexer**: Fully operational.
- **Interpreter**: Fully operational for basic and mid-level programs.
- **Structs, Enums, Classes**: Working.
- **Input, Output, Variables, Expressions**: Working.
- **Support Only for Linux** (for now).

---

## Building

```bash
git clone https://github.com/yourusername/yenlang.git
cd yenlang
g++ -std=c++17 -o yen main.cpp lexer.cpp parser.cpp compiler.cpp
```

## Running

```bash
./yen file.yl
```

Example:

```bash
./yen examples/hello_world.yl
```

---

## Contributing

We warmly welcome contributions!

If you find a bug, want to add a feature, or simply improve the documentation, feel free to fork the project and open a Pull Request (PR).

👉 **Official Repository:** [https://github.com/yen-lang/Yen](https://github.com/yen-lang/Yen)

### How to contribute:

1. Fork the repository on GitHub.
2. Clone your fork locally:
   ```bash
   git clone https://github.com/your-username/Yen.git
   ```
3. Create a new branch:
   ```bash
   git checkout -b feature/my-new-feature
   ```
4. Make your changes.
5. Commit your changes:
   ```bash
   git commit -m "Add my new feature"
   ```
6. Push to your fork:
   ```bash
   git push origin feature/my-new-feature
   ```
7. Open a Pull Request on [https://github.com/yen-lang/Yen](https://github.com/yen-lang/Yen).

---

> **Note:**  
> Please try to keep the code style clean and consistent with the existing code.  
> Discussions, ideas, and improvements are always welcome!

---

## License

MIT License.

![AI Assisted](https://img.shields.io/badge/Code%2040%25%20AI%20-%2060%25%20Human-blueviolet?style=for-the-badge&logo=openai&logoColor=white)

