#!/bin/bash
# ============================================================================
# Yen Language - macOS Build Script
# Builds the interpreter and creates .tar.gz and .pkg packages
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
DIST_DIR="$PROJECT_DIR/dist"
VERSION="1.1.0"
ARCH="$(uname -m)"

echo "=== Yen Language - macOS Build ==="
echo "Version: $VERSION"
echo "Architecture: $ARCH"
echo ""

# Check dependencies
echo "[1/5] Checking dependencies..."
for cmd in cmake clang++ make; do
    if ! command -v $cmd &> /dev/null; then
        echo "ERROR: $cmd not found. Install Xcode Command Line Tools:"
        echo "  xcode-select --install"
        echo "  brew install cmake"
        exit 1
    fi
done

# Configure
echo "[2/5] Configuring..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release

# Build
echo "[3/5] Building..."
make yen -j"$(sysctl -n hw.ncpu)"

echo "[4/5] Running tests..."
cd "$PROJECT_DIR"
if [ -f tests/run_tests.sh ]; then
    bash tests/run_tests.sh || echo "WARNING: Some tests failed"
fi

# Package
echo "[5/5] Packaging..."
mkdir -p "$DIST_DIR"

# .tar.gz package
TARBALL="yen-${VERSION}-macos-${ARCH}.tar.gz"
STAGING_DIR=$(mktemp -d)
mkdir -p "$STAGING_DIR/yen/bin"
mkdir -p "$STAGING_DIR/yen/lib/yen/stdlib"
cp "$BUILD_DIR/yen" "$STAGING_DIR/yen/bin/"
cp -r "$PROJECT_DIR/stdlib/"* "$STAGING_DIR/yen/lib/yen/stdlib/" 2>/dev/null || true
cp "$PROJECT_DIR/LICENSE" "$STAGING_DIR/yen/" 2>/dev/null || true
tar -czf "$DIST_DIR/$TARBALL" -C "$STAGING_DIR" yen
rm -rf "$STAGING_DIR"
echo "  Created: dist/$TARBALL"

# .pkg installer
PKG_NAME="yen-${VERSION}-macos-${ARCH}.pkg"
PKG_ROOT=$(mktemp -d)
mkdir -p "$PKG_ROOT/usr/local/bin"
mkdir -p "$PKG_ROOT/usr/local/lib/yen/stdlib"
cp "$BUILD_DIR/yen" "$PKG_ROOT/usr/local/bin/"
cp -r "$PROJECT_DIR/stdlib/"* "$PKG_ROOT/usr/local/lib/yen/stdlib/" 2>/dev/null || true

pkgbuild \
    --root "$PKG_ROOT" \
    --identifier "com.yen-lang.yen" \
    --version "$VERSION" \
    --install-location "/" \
    "$DIST_DIR/$PKG_NAME"

rm -rf "$PKG_ROOT"
echo "  Created: dist/$PKG_NAME"

echo ""
echo "=== Build complete! ==="
echo "Binary: $BUILD_DIR/yen"
echo "Packages: $DIST_DIR/"
ls -lh "$DIST_DIR/"
