# YEN VS Code Extension - Quick Start Guide

## 🚀 Quick Installation

### For End Users

```bash
# Navigate to the extension directory
cd extension-vscode

# Install vsce globally (first time only)
npm install -g vsce

# Package the extension
vsce package

# Install in VS Code
code --install-extension yen-language-1.0.0.vsix
```

## 📦 What's Included

### Files Structure

```
extension-vscode/
├── package.json              # Extension manifest
├── syntaxes/
│   └── yen.tmLanguage.json  # Syntax highlighting grammar
├── language-configuration.json  # Language settings
├── snippets/
│   └── yen.json             # Code snippets
├── images/
│   ├── icon.svg             # Extension icon
│   └── icon.png             # PNG version
├── examples/
│   └── sample.yen           # Sample code
├── README.md                # User documentation
├── CHANGELOG.md             # Version history
├── INSTALLATION.md          # Detailed install guide
└── LICENSE                  # MIT License
```

## ✨ Features

### 1. Syntax Highlighting

All YEN syntax is beautifully highlighted:

- **Keywords**: `func`, `var`, `let`, `class`, `match`, `defer`
- **Types**: `int`, `float`, `bool`, `string`
- **Operators**: `+`, `-`, `*`, `/`, `==`, `..`, `..=`, `=>`
- **Standard Library**: All 80+ functions highlighted
- **Comments**: Line and block comments

### 2. Code Snippets (50+)

Try these snippets by typing the prefix and pressing `Tab`:

| Prefix | Expands To |
|--------|------------|
| `func` | Function declaration |
| `class` | Class definition |
| `struct` | Struct definition |
| `match` | Match expression |
| `for` | For-in loop |
| `forr` | For range loop |
| `if` | If statement |
| `ife` | If-else statement |
| `lambda` | Lambda expression |
| `shell` | Shell command |
| `fread` | File read |
| `fwrite` | File write |
| `log` | Log message |
| `main` | Main function template |

### 3. Smart Editing

- **Auto-closing**: Brackets `{}`, `[]`, `()` and quotes `""`, `''`
- **Auto-indentation**: Smart indentation for blocks
- **Comment toggling**: `Ctrl+/` or `Cmd+/`
- **Code folding**: Collapse functions and blocks
- **Bracket matching**: Highlight matching brackets

### 4. Commands

- **Run YEN File**: `Ctrl+Shift+R` (or `Cmd+Shift+R` on Mac)
- Access via Command Palette: `Ctrl+Shift+P` → "YEN: Run File"

## 🎯 Quick Test

1. Install the extension (see above)
2. Create a new file: `test.yen`
3. Try these snippets:

```yen
// Type 'func' and press Tab
func greet(name) {
    print "Hello, " + name + "!";
}

// Type 'match' and press Tab
match (value) {
    pattern => result;
    _ => default;
}

// Type 'shell' and press Tab
var output = process_shell("ls -la");
```

4. Verify syntax highlighting works
5. Press `Ctrl+/` to toggle comments
6. Try `Ctrl+Shift+R` to run (if YEN is installed)

## 🛠️ Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/yen-lang/Yen.git
cd Yen/extension-vscode

# Package extension
npm install -g vsce
vsce package
```

### Testing Changes

1. Open extension folder in VS Code:
```bash
code .
```

2. Press `F5` to launch Extension Development Host
3. Create a `.yen` file in the new window
4. Test your changes
5. Make edits and reload (`Ctrl+R`)

### Adding New Snippets

Edit `snippets/yen.json`:

```json
{
  "My Snippet": {
    "prefix": "mysnip",
    "body": [
      "func ${1:name}() {",
      "\t$0",
      "}"
    ],
    "description": "My custom snippet"
  }
}
```

### Modifying Syntax Highlighting

Edit `syntaxes/yen.tmLanguage.json`:

```json
{
  "name": "support.function.mylib.yen",
  "match": "\\b(my_function|my_other_function)\\b"
}
```

## 📚 Documentation

- [README.md](README.md) - Full user documentation
- [INSTALLATION.md](INSTALLATION.md) - Detailed installation guide
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [vsc-extension-quickstart.md](vsc-extension-quickstart.md) - VS Code extension guide

## 🐛 Troubleshooting

### Extension Not Loading

```bash
# Reload VS Code window
Ctrl+Shift+P → "Developer: Reload Window"

# Check extensions log
View → Output → Extensions
```

### Syntax Highlighting Not Working

1. Check file extension is `.yen`
2. Check language mode (bottom right) shows "YEN"
3. Manually set: Click language → Select "YEN"

### Snippets Not Expanding

1. Type the prefix (e.g., `func`)
2. Press `Tab` (not Enter)
3. Make sure file is `.yen`

## 📋 Checklist for Publishing

Before publishing to VS Code Marketplace:

- [ ] Test all snippets work correctly
- [ ] Verify syntax highlighting for all language features
- [ ] Test on Windows, macOS, and Linux
- [ ] Add screenshots to README
- [ ] Update version in `package.json`
- [ ] Update CHANGELOG.md
- [ ] Create publisher account
- [ ] Get Personal Access Token
- [ ] Test VSIX package locally
- [ ] Publish: `vsce publish`

## 🎨 Color Themes

Extension works with all VS Code themes:

- **Recommended**:
  - Dark+ (default)
  - Monokai
  - One Dark Pro
  - Dracula
  - Nord
  - GitHub Dark

## 🔗 Resources

- [VS Code Extension API](https://code.visualstudio.com/api)
- [Publishing Extensions](https://code.visualstudio.com/api/working-with-extensions/publishing-extension)
- [Extension Marketplace](https://marketplace.visualstudio.com/vscode)
- [YEN Language Repository](https://github.com/yen-lang/Yen)

## 💡 Tips

1. **Use Workspace Settings**: Configure `yen.interpreterPath` per project
2. **Keyboard Shortcuts**: Customize in File → Preferences → Keyboard Shortcuts
3. **Snippets**: Create your own in User Snippets
4. **Color Customization**: Override colors in User Settings

## ❤️ Contributing

Contributions welcome!

1. Fork the repository
2. Create feature branch
3. Make changes
4. Test thoroughly
5. Submit pull request

See [CONTRIBUTING.md](../CONTRIBUTING.md) for more details.

---

**Happy coding with YEN! 🚀**
