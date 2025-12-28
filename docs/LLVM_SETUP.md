# LLVM Setup Guide for YEN Compiler

## Overview
YEN compiler (`yenc`) requires LLVM for AOT (Ahead-of-Time) compilation to native executables.

## Installation

### macOS (non-root user)
```bash
brew install llvm
```

If running as root (not recommended), you'll need to:
1. Create a non-root user account
2. Install Homebrew under that account
3. Install LLVM: `brew install llvm`

Or download pre-built binaries from https://releases.llvm.org/

### macOS (manual setup)
```bash
# Download LLVM 17.x from https://github.com/llvm/llvm-project/releases
# Extract and set environment variables:
export LLVM_DIR=/path/to/llvm/lib/cmake/llvm
export PATH=/path/to/llvm/bin:$PATH
```

### Linux
```bash
# Ubuntu/Debian
sudo apt-get install llvm-17 llvm-17-dev

# Fedora
sudo dnf install llvm llvm-devel

# Arch
sudo pacman -S llvm
```

### Windows
Download pre-built binaries from https://releases.llvm.org/download.html

## Verification
```bash
llvm-config --version
# Should output: 17.x.x or later
```

## Build Configuration
Once LLVM is installed, CMake will automatically detect it and enable the compiler build.

## Current Status
- **Interpreter (`yen`)**: Works without LLVM (tree-walk interpreter)
- **Compiler (`yenc`)**: Requires LLVM installation
- **Type System**: Being implemented, works independently of LLVM

## Next Steps After LLVM Installation
1. Rebuild project: `cd build && cmake .. && make`
2. Verify `yenc` executable is created
3. Test compilation: `./yenc test.yen -o test`
