# Installing YEN Language Extension for VS Code

## Method 1: From VS Code Marketplace (Future)

Once published, you can install directly from VS Code:

1. Open Visual Studio Code
2. Press `Ctrl+Shift+X` (or `Cmd+Shift+X` on Mac) to open Extensions
3. Search for "YEN Language Support"
4. Click **Install**

## Method 2: Install from VSIX File

### Step 1: Build the Extension

```bash
cd extension-vscode

# Install dependencies (if not already installed)
npm install -g vsce

# Package the extension
vsce package
```

This will create a file named `yen-language-1.0.0.vsix`

### Step 2: Install in VS Code

**Option A: Using VS Code UI**

1. Open VS Code
2. Go to Extensions view (`Ctrl+Shift+X`)
3. Click the `...` menu (three dots) at the top
4. Select **Install from VSIX...**
5. Navigate to and select `yen-language-1.0.0.vsix`
6. Reload VS Code when prompted

**Option B: Using Command Line**

```bash
code --install-extension yen-language-1.0.0.vsix
```

**Option C: Using VS Code Command Palette**

1. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
2. Type "Extensions: Install from VSIX"
3. Select the `yen-language-1.0.0.vsix` file

## Method 3: Development Installation

For testing and development:

1. Clone the repository:
```bash
git clone https://github.com/yen-lang/Yen.git
cd Yen/extension-vscode
```

2. Install dependencies:
```bash
npm install
```

3. Open in VS Code:
```bash
code .
```

4. Press `F5` to launch Extension Development Host
5. A new VS Code window will open with the extension loaded
6. Create a `.yen` file to test

## Verification

After installation, verify it works:

1. Create a new file: `test.yen`
2. Write some YEN code:
```yen
func greet(name) {
    print "Hello, " + name + "!";
}

greet("World");
```
3. Check that syntax is highlighted correctly
4. Try snippets:
   - Type `func` and press `Tab`
   - Type `match` and press `Tab`
   - Type `shell` and press `Tab`

## Uninstallation

### From VS Code UI

1. Go to Extensions view (`Ctrl+Shift+X`)
2. Find "YEN Language Support"
3. Click the gear icon
4. Select **Uninstall**

### From Command Line

```bash
code --uninstall-extension yen-lang.yen-language
```

## Troubleshooting

### Extension Not Working

1. **Reload VS Code**: Press `Ctrl+Shift+P` → "Developer: Reload Window"
2. **Check extension is enabled**: Go to Extensions, make sure it's not disabled
3. **Check file extension**: File must end with `.yen`
4. **Check VS Code version**: Requires VS Code 1.75.0 or higher

### Syntax Highlighting Not Working

1. Verify file has `.yen` extension
2. Check the language mode in the bottom right corner (should say "YEN")
3. Try manually setting language: Click language indicator → Select "YEN"
4. Reload the window: `Ctrl+Shift+P` → "Developer: Reload Window"

### Snippets Not Working

1. Make sure you're in a `.yen` file
2. Type the snippet prefix (e.g., `func`)
3. Press `Tab` (not `Enter`)
4. Check that IntelliSense is enabled in VS Code settings

### Extension Not Showing in Extensions List

1. Make sure VSIX installation completed without errors
2. Check VS Code output: `View` → `Output` → Select "Extensions"
3. Try restarting VS Code
4. Reinstall the extension

## Configuration

After installation, configure paths in VS Code settings:

1. Open Settings: `Ctrl+,` (or `Cmd+,` on Mac)
2. Search for "yen"
3. Set paths:
   - **YEN: Interpreter Path**: Path to `yen` executable
   - **YEN: Compiler Path**: Path to `yenc` executable
   - **YEN: Format On Save**: Enable/disable auto-format

Or edit `settings.json` directly:

```json
{
  "yen.interpreterPath": "/opt/Yen/build/yen",
  "yen.compilerPath": "/opt/Yen/build/yenc",
  "yen.formatOnSave": false
}
```

## Publishing to Marketplace (For Maintainers)

### Prerequisites

1. Create a publisher account at https://marketplace.visualstudio.com/vscode
2. Get a Personal Access Token (PAT)
3. Install vsce: `npm install -g vsce`

### Publishing Steps

```bash
# Login with your publisher name
vsce login <publisher-name>

# Publish the extension
vsce publish

# Or publish with version bump
vsce publish minor  # 1.0.0 → 1.1.0
vsce publish major  # 1.0.0 → 2.0.0
vsce publish patch  # 1.0.0 → 1.0.1
```

## Updates

### Updating the Extension

When a new version is released:

**From Marketplace:**
- VS Code will notify you of updates automatically
- Click "Update" in the notification

**From VSIX:**
- Download the new `.vsix` file
- Install it (will replace the old version)

### Checking for Updates

1. Go to Extensions view
2. Click the "..." menu
3. Select "Check for Extension Updates"

## Support

If you encounter issues:

1. Check the [documentation](https://github.com/yen-lang/Yen/tree/main/docs)
2. Search [existing issues](https://github.com/yen-lang/Yen/issues)
3. Create a [new issue](https://github.com/yen-lang/Yen/issues/new)

---

**Enjoy coding with YEN! 🚀**
