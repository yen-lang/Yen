#!/bin/bash

# YEN VS Code Extension Build Script

set -e

echo "========================================="
echo "YEN VS Code Extension Build Tool"
echo "========================================="
echo ""

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if vsce is installed
if ! command -v vsce &> /dev/null; then
    echo -e "${RED}Error: vsce is not installed${NC}"
    echo "Install it with: npm install -g vsce"
    exit 1
fi

# Check if we're in the right directory
if [ ! -f "package.json" ]; then
    echo -e "${RED}Error: package.json not found${NC}"
    echo "Please run this script from the extension-vscode directory"
    exit 1
fi

# Parse command line arguments
ACTION=${1:-package}

case $ACTION in
    package)
        echo -e "${BLUE}[1/2] Packaging extension...${NC}"
        vsce package

        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Extension packaged successfully!${NC}"
            VSIX_FILE=$(ls -t *.vsix | head -1)
            echo ""
            echo "VSIX file created: $VSIX_FILE"
            echo ""
            echo "To install:"
            echo "  code --install-extension $VSIX_FILE"
            echo ""
        else
            echo -e "${RED}✗ Packaging failed${NC}"
            exit 1
        fi
        ;;

    install)
        echo -e "${BLUE}[1/3] Packaging extension...${NC}"
        vsce package

        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Extension packaged${NC}"

            VSIX_FILE=$(ls -t *.vsix | head -1)

            echo -e "${BLUE}[2/3] Installing extension...${NC}"
            code --install-extension $VSIX_FILE

            if [ $? -eq 0 ]; then
                echo -e "${GREEN}✓ Extension installed successfully!${NC}"
                echo ""
                echo -e "${BLUE}[3/3] Reloading VS Code...${NC}"
                echo "Please reload VS Code to activate the extension"
                echo "Press: Ctrl+Shift+P → 'Developer: Reload Window'"
            else
                echo -e "${RED}✗ Installation failed${NC}"
                exit 1
            fi
        else
            echo -e "${RED}✗ Packaging failed${NC}"
            exit 1
        fi
        ;;

    test)
        echo -e "${BLUE}Opening Extension Development Host...${NC}"
        echo "Press F5 in VS Code to launch test window"
        code .
        ;;

    clean)
        echo -e "${BLUE}Cleaning build artifacts...${NC}"
        rm -f *.vsix
        rm -rf node_modules
        echo -e "${GREEN}✓ Cleaned${NC}"
        ;;

    publish)
        echo -e "${BLUE}Publishing to VS Code Marketplace...${NC}"
        echo ""
        echo "Make sure you have:"
        echo "  1. A publisher account"
        echo "  2. A Personal Access Token"
        echo "  3. Logged in with: vsce login <publisher>"
        echo ""
        read -p "Continue? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            vsce publish
        else
            echo "Cancelled"
        fi
        ;;

    *)
        echo "Usage: $0 {package|install|test|clean|publish}"
        echo ""
        echo "Commands:"
        echo "  package  - Package extension to .vsix file"
        echo "  install  - Package and install in VS Code"
        echo "  test     - Open in VS Code for development"
        echo "  clean    - Remove build artifacts"
        echo "  publish  - Publish to VS Code Marketplace"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "Done!"
echo "========================================="
