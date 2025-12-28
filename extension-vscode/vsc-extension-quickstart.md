# Welcome to YEN Language Extension

## What's in the folder

* `package.json` - The manifest file declaring language support and snippets
* `syntaxes/yen.tmLanguage.json` - TextMate grammar file for syntax highlighting
* `language-configuration.json` - Language configuration for brackets, comments, etc.
* `snippets/yen.json` - Code snippets for YEN language

## Get up and running

* Press `F5` to open a new window with your extension loaded
* Create a new file with `.yen` extension
* Verify syntax highlighting works and snippets are available

## Make changes

* You can relaunch the extension from the debug toolbar after making changes
* You can also reload (`Ctrl+R` or `Cmd+R` on Mac) the VS Code window to load changes

## Add more language features

* To add more snippets, edit `snippets/yen.json`
* To modify syntax highlighting, edit `syntaxes/yen.tmLanguage.json`
* To change language behavior, edit `language-configuration.json`

## Install your extension

* To share your extension with others:
  * Install `vsce`: `npm install -g vsce`
  * Package extension: `vsce package`
  * Share the `.vsix` file

* To publish to the Marketplace:
  * Create a publisher account at https://marketplace.visualstudio.com/vscode
  * Get a Personal Access Token
  * Login: `vsce login <publisher>`
  * Publish: `vsce publish`

## Testing

1. Open VS Code
2. Press `F5` to launch Extension Development Host
3. Create a new file: `test.yen`
4. Test syntax highlighting
5. Test snippets by typing prefixes and pressing `Tab`
6. Test commands with `Ctrl+Shift+P` → "YEN: Run File"

## Debugging

* Set breakpoints in your extension code
* Use `console.log` for debugging
* View output in Debug Console

## Learn more

* [VS Code Extension API](https://code.visualstudio.com/api)
* [TextMate Grammar](https://macromates.com/manual/en/language_grammars)
* [Extension Guidelines](https://code.visualstudio.com/api/references/extension-guidelines)

## Publishing Checklist

Before publishing:

- [x] Test all snippets
- [x] Verify syntax highlighting for all language features
- [x] Test auto-closing brackets
- [x] Test comment toggling
- [x] Update README.md with screenshots
- [x] Update CHANGELOG.md
- [x] Set appropriate version number
- [ ] Add extension icon (128x128 PNG)
- [ ] Test on Windows, macOS, and Linux
- [ ] Get feedback from users
