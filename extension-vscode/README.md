# YEN Language Support for Visual Studio Code

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/yen-lang/Yen)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](https://opensource.org/licenses/MIT)

Official Visual Studio Code extension for the **YEN programming language** - providing syntax highlighting, code snippets, and language support.

## Features

### 🎨 Syntax Highlighting

Full syntax highlighting support for YEN language features:

- **Keywords**: `func`, `class`, `struct`, `enum`, `var`, `let`, `if`, `else`, `while`, `for`, `match`, `defer`
- **Operators**: Arithmetic, comparison, logical, range (`..`, `..=`), lambda (`|`)
- **Literals**: Numbers (int, float, hex, binary), strings, booleans
- **Standard Library**: All 12 built-in modules with 80+ functions
- **Comments**: Line (`//`) and block (`/* */`) comments

![Syntax Highlighting Example](images/syntax-preview.png)

### 📝 Code Snippets

50+ intelligent code snippets for rapid development:

| Prefix | Description |
|--------|-------------|
| `func` | Function declaration |
| `class` | Class definition |
| `match` | Match expression |
| `for` | For-in loop |
| `shell` | Shell command execution |
| `fread` | File read operation |
| `log` | Logging statements |

Type the prefix and press `Tab` to expand!

### ⚙️ Language Configuration

- **Auto-closing brackets**: `{}`, `[]`, `()`
- **Auto-closing quotes**: `""`, `''`
- **Smart indentation**: Automatic indentation for blocks
- **Code folding**: Fold functions, classes, and blocks
- **Comment toggling**: `Ctrl+/` (or `Cmd+/` on Mac)

### 🚀 Commands

- **Run YEN File**: `Ctrl+Shift+R` (or `Cmd+Shift+R` on Mac)
- **Compile YEN File**: Available in command palette

### 🔧 Settings

Configure YEN extension in your VS Code settings:

```json
{
  "yen.interpreterPath": "/path/to/yen",
  "yen.compilerPath": "/path/to/yenc",
  "yen.formatOnSave": false
}
```

## Installation

### From VS Code Marketplace (Future)

1. Open VS Code
2. Press `Ctrl+P` (or `Cmd+P` on Mac)
3. Type `ext install yen-lang.yen-language`
4. Press `Enter`

### Manual Installation

1. Download the `.vsix` file from [Releases](https://github.com/yen-lang/Yen/releases)
2. Open VS Code
3. Go to Extensions (`Ctrl+Shift+X`)
4. Click `...` menu → `Install from VSIX...`
5. Select the downloaded file

### From Source

```bash
cd extension-vscode
npm install
npm run compile
vsce package
code --install-extension yen-language-1.0.0.vsix
```

## Usage

### Creating a YEN File

1. Create a new file with `.yen` extension
2. Start coding with full syntax highlighting and IntelliSense

### Running YEN Code

Press `Ctrl+Shift+R` (or use Command Palette → `YEN: Run File`)

### Code Snippets Examples

#### Function with Match
Type `func` + `Tab`:
```yen
func greet(name) {
    print "Hello, " + name + "!";
}
```

Type `match` + `Tab`:
```yen
match (value) {
    pattern => result;
    _ => default;
}
```

#### Shell Command
Type `shell` + `Tab`:
```yen
var output = process_shell("ls -la");
```

#### File Operations
Type `fread` + `Tab`:
```yen
var content = io_read_file("file.txt");
```

## Language Features

### Supported Syntax

```yen
// Variables
var mutable = 10;
let immutable = 20;

// Functions
func add(a: int, b: int) -> int {
    return a + b;
}

// Classes
class Person {
    let name;
    let age;

    func greet() {
        print "Hello, I'm " + this.name;
    }
}

// Pattern Matching
match (score) {
    0..60 => print "F";
    60..70 => print "D";
    70..80 => print "C";
    80..90 => print "B";
    90..=100 => print "A";
    _ => print "Invalid";
}

// Lambdas
let square = |x| x * x;
print square(5);  // 25

// Defer
func cleanup() {
    defer print "Cleanup done!";
    // ... work ...
}

// Shell Commands
var files = process_shell("ls -la");
var dir = process_cwd();
process_exec("mkdir /tmp/test");
```

### Standard Library Highlighting

All 12 standard library modules are highlighted:

- **Core**: `core_is_int`, `core_to_string`, etc.
- **Math**: `math_sqrt`, `math_pow`, `math_random`
- **String**: `str_split`, `str_upper`, `str_contains`
- **Collections**: `list_push`, `list_sort`, `list_reverse`
- **IO**: `io_read_file`, `io_write_file`
- **FS**: `fs_exists`, `fs_create_dir`
- **Time**: `time_now`, `time_sleep`
- **Crypto**: `crypto_xor`, `crypto_hash`
- **Encoding**: `encoding_base64_encode`
- **Log**: `log_info`, `log_warn`, `log_error`
- **Env**: `env_get`, `env_set`
- **Process**: `process_exec`, `process_shell`, `process_cwd`

## Color Themes

The extension works great with all VS Code themes! Recommended themes:

- **Dark+** (Default Dark)
- **Monokai**
- **One Dark Pro**
- **Dracula**
- **Nord**

## Requirements

- Visual Studio Code 1.75.0 or higher
- YEN interpreter installed (optional, for running code)

## Extension Settings

This extension contributes the following settings:

* `yen.interpreterPath`: Path to YEN interpreter executable
* `yen.compilerPath`: Path to YEN compiler executable
* `yen.formatOnSave`: Enable/disable format on save

## Known Issues

- Compiler integration is still in development
- Formatter not yet implemented
- IntelliSense/autocomplete planned for future release

## Roadmap

- [ ] IntelliSense/autocomplete support
- [ ] Code formatter integration
- [ ] Debugger support
- [ ] Language Server Protocol (LSP) integration
- [ ] Integrated terminal for running YEN code
- [ ] Linting and error checking
- [ ] Go to definition
- [ ] Find all references
- [ ] Rename symbol

## Contributing

Contributions are welcome! Please visit:

- [YEN Language Repository](https://github.com/yen-lang/Yen)
- [Report Issues](https://github.com/yen-lang/Yen/issues)
- [Contributing Guide](https://github.com/yen-lang/Yen/blob/main/CONTRIBUTING.md)

## Release Notes

### 1.0.0

Initial release of YEN Language Support

- ✅ Full syntax highlighting
- ✅ 50+ code snippets
- ✅ Smart indentation
- ✅ Auto-closing brackets and quotes
- ✅ Comment toggling
- ✅ Code folding
- ✅ Run file command

## Resources

- [YEN Documentation](https://github.com/yen-lang/Yen/tree/main/docs)
- [Language Syntax Guide](https://github.com/yen-lang/Yen/blob/main/docs/SYNTAX.md)
- [Standard Library Reference](https://github.com/yen-lang/Yen/blob/main/docs/STDLIB.md)
- [Examples](https://github.com/yen-lang/Yen/tree/main/examples)

## License

This extension is licensed under the [MIT License](LICENSE).

---

**Enjoy coding with YEN! 🚀**

Made with ❤️ by the YEN Language team
