#!/bin/bash
# ============================================================================
# Yen Language - Universal Install Script (Linux/macOS)
# Usage: curl -fsSL https://raw.githubusercontent.com/yen-lang/yen/main/builds/install.sh | bash
# ============================================================================
set -e

VERSION="1.1.0"
INSTALL_DIR="/usr/local"
BIN_DIR="$INSTALL_DIR/bin"
LIB_DIR="$INSTALL_DIR/lib/yen"

# Detect OS and architecture
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux)  PLATFORM="linux" ;;
    Darwin) PLATFORM="macos" ;;
    *)
        echo "ERROR: Unsupported OS: $OS"
        echo "For Windows, download from GitHub Releases."
        exit 1
        ;;
esac

echo "=== Yen Language Installer ==="
echo "Version:  $VERSION"
echo "Platform: $PLATFORM ($ARCH)"
echo "Install:  $BIN_DIR/yen"
echo ""

# GitHub release URL
REPO="yen-lang/yen"
TARBALL="yen-${VERSION}-${PLATFORM}-${ARCH}.tar.gz"
URL="https://github.com/${REPO}/releases/download/v${VERSION}/${TARBALL}"

# Download
echo "Downloading $TARBALL..."
TMP_DIR=$(mktemp -d)
trap "rm -rf $TMP_DIR" EXIT

if command -v curl &> /dev/null; then
    curl -fsSL "$URL" -o "$TMP_DIR/$TARBALL"
elif command -v wget &> /dev/null; then
    wget -q "$URL" -O "$TMP_DIR/$TARBALL"
else
    echo "ERROR: Neither curl nor wget found."
    exit 1
fi

# Extract
echo "Extracting..."
tar -xzf "$TMP_DIR/$TARBALL" -C "$TMP_DIR"

# Install (may need sudo)
echo "Installing..."
SUDO=""
if [ ! -w "$BIN_DIR" ]; then
    SUDO="sudo"
    echo "  (requires sudo for $BIN_DIR)"
fi

$SUDO mkdir -p "$BIN_DIR"
$SUDO mkdir -p "$LIB_DIR/stdlib"
$SUDO cp "$TMP_DIR/yen/bin/yen" "$BIN_DIR/yen"
$SUDO chmod +x "$BIN_DIR/yen"
$SUDO cp -r "$TMP_DIR/yen/lib/yen/stdlib/"* "$LIB_DIR/stdlib/" 2>/dev/null || true

# Verify
if command -v yen &> /dev/null; then
    echo ""
    echo "=== Installation complete! ==="
    echo "Run 'yen --help' or 'yen yourfile.yen' to get started."
else
    echo ""
    echo "=== Installed to $BIN_DIR/yen ==="
    echo "Make sure $BIN_DIR is in your PATH."
    echo "  export PATH=\"$BIN_DIR:\$PATH\""
fi
