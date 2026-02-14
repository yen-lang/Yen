#!/bin/bash
# ============================================================================
# Yen Language - Linux Build Script
# Builds the interpreter and creates .tar.gz and .deb packages
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
DIST_DIR="$PROJECT_DIR/dist"
VERSION="1.1.0"
ARCH="$(uname -m)"

echo "=== Yen Language - Linux Build ==="
echo "Version: $VERSION"
echo "Architecture: $ARCH"
echo ""

# Check dependencies
echo "[1/5] Checking dependencies..."
for cmd in cmake g++ make; do
    if ! command -v $cmd &> /dev/null; then
        echo "ERROR: $cmd not found. Install with:"
        echo "  sudo apt-get install build-essential cmake"
        exit 1
    fi
done

# Check for libcurl
if ! pkg-config --exists libcurl 2>/dev/null; then
    echo "WARNING: libcurl not found. HTTP/HTTPS support will be disabled."
    echo "  Install with: sudo apt-get install libcurl4-openssl-dev"
fi

# Configure
echo "[2/5] Configuring..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release

# Build
echo "[3/5] Building..."
make yen -j"$(nproc)"

echo "[4/5] Running tests..."
cd "$PROJECT_DIR"
if [ -f tests/run_tests.sh ]; then
    bash tests/run_tests.sh || echo "WARNING: Some tests failed"
fi

# Package
echo "[5/5] Packaging..."
mkdir -p "$DIST_DIR"

# .tar.gz package
TARBALL="yen-${VERSION}-linux-${ARCH}.tar.gz"
STAGING_DIR=$(mktemp -d)
mkdir -p "$STAGING_DIR/yen/bin"
mkdir -p "$STAGING_DIR/yen/lib/yen/stdlib"
cp "$BUILD_DIR/yen" "$STAGING_DIR/yen/bin/"
cp -r "$PROJECT_DIR/stdlib/"* "$STAGING_DIR/yen/lib/yen/stdlib/" 2>/dev/null || true
cp "$PROJECT_DIR/LICENSE" "$STAGING_DIR/yen/" 2>/dev/null || true
tar -czf "$DIST_DIR/$TARBALL" -C "$STAGING_DIR" yen
rm -rf "$STAGING_DIR"
echo "  Created: dist/$TARBALL"

# .deb package
if command -v dpkg-deb &> /dev/null; then
    DEB_NAME="yen-${VERSION}-linux-${ARCH}.deb"
    DEB_DIR=$(mktemp -d)
    mkdir -p "$DEB_DIR/DEBIAN"
    mkdir -p "$DEB_DIR/usr/local/bin"
    mkdir -p "$DEB_DIR/usr/local/lib/yen/stdlib"

    cat > "$DEB_DIR/DEBIAN/control" << EOF
Package: yen
Version: $VERSION
Section: devel
Priority: optional
Architecture: $(dpkg --print-architecture)
Depends: libcurl4-openssl-dev
Maintainer: yen-lang
Description: Yen Programming Language Interpreter
 A modern, expressive programming language with a tree-walking interpreter.
EOF

    cp "$BUILD_DIR/yen" "$DEB_DIR/usr/local/bin/"
    cp -r "$PROJECT_DIR/stdlib/"* "$DEB_DIR/usr/local/lib/yen/stdlib/" 2>/dev/null || true
    dpkg-deb --build "$DEB_DIR" "$DIST_DIR/$DEB_NAME"
    rm -rf "$DEB_DIR"
    echo "  Created: dist/$DEB_NAME"
fi

# CPack packages
cd "$BUILD_DIR"
cpack -G TGZ 2>/dev/null && echo "  Created CPack TGZ" || true
cpack -G DEB 2>/dev/null && echo "  Created CPack DEB" || true

echo ""
echo "=== Build complete! ==="
echo "Binary: $BUILD_DIR/yen"
echo "Packages: $DIST_DIR/"
ls -lh "$DIST_DIR/" 2>/dev/null
