# Change Log

All notable changes to the "yen-language" extension will be documented in this file.

## [1.0.0] - 2025-12-28

### Added
- Initial release of YEN Language Support for VS Code
- Full syntax highlighting for YEN language
  - Keywords (func, class, var, let, if, else, match, defer, etc.)
  - Operators (arithmetic, comparison, logical, range, lambda)
  - Literals (numbers, strings, booleans)
  - Comments (line and block)
  - Standard library functions (80+ functions across 12 modules)
- 50+ code snippets for common patterns
  - Functions, classes, structs, enums
  - Control flow (if, while, for, match)
  - Standard library operations
  - Shell commands
  - File I/O
- Language configuration
  - Auto-closing brackets, quotes, and pipes
  - Smart indentation
  - Code folding
  - Comment toggling (Ctrl+/)
- Keyboard shortcuts
  - Run YEN File: Ctrl+Shift+R (Cmd+Shift+R on Mac)
- Extension settings
  - Interpreter path configuration
  - Compiler path configuration
  - Format on save option
- Commands
  - YEN: Run File
  - YEN: Compile File

### Features
- Supports all YEN language features:
  - Pattern matching with guards and ranges
  - Lambda expressions and closures
  - Defer statements (RAII-style)
  - Classes and structs
  - Enums
  - Range expressions (.. and ..=)
  - Shell command execution
- Syntax highlighting for 12 standard library modules:
  - Core (type checking, conversion)
  - Math (sqrt, pow, trig, random)
  - String (split, join, upper, lower, replace)
  - Collections (push, pop, sort, reverse)
  - IO (read_file, write_file)
  - FS (exists, create_dir, remove)
  - Time (now, sleep)
  - Crypto (xor, hash, random_bytes)
  - Encoding (base64, hex)
  - Log (info, warn, error)
  - Env (get, set)
  - Process (exec, shell, spawn, cwd, chdir)

### Planned for Future Releases
- IntelliSense/autocomplete support
- Code formatter
- Debugger integration
- Language Server Protocol (LSP)
- Error checking and linting
- Go to definition
- Find all references
- Rename symbol
- Integrated terminal

## [Unreleased]

### Planned
- LSP integration for better IntelliSense
- Real-time error checking
- Code formatting
- Debugger support
- More snippets
- Code actions and quick fixes
