# Yen – Build & Release

## Quick Build (Local)

### Linux

```bash
./builds/build-linux.sh
```

Generates:
`dist/yen-1.1.0-linux-x86_64.tar.gz`
`dist/yen-1.1.0-linux-x86_64.deb`

---

### macOS

```bash
./builds/build-macos.sh
```

Generates:
`dist/yen-1.1.0-macos-arm64.tar.gz`
`dist/yen-1.1.0-macos-arm64.pkg`

---

### Windows

```bat
builds\build-windows.bat
```

Generates:
`dist\yen-1.1.0-windows-x64.zip`

---

## CI/CD (GitHub Actions)

To enable automatic builds for all three platforms:

```bash
mkdir -p .github/workflows
cp builds/ci-build.yml .github/workflows/ci-build.yml
git add .github/workflows/ci-build.yml
git commit -m "Add CI/CD build workflow"
git push
```

The workflow will:

* Build automatically on **Linux**, **macOS**, and **Windows**
* Generate downloadable artifacts for each PR and push
* Create a **GitHub Release** with all packages when a `v*` tag is published

---

## Creating a Release

```bash
git tag v1.1.0
git push origin v1.1.0
```

---

## Install Script (Linux/macOS)

```bash
curl -fsSL https://raw.githubusercontent.com/yen-lang/yen/main/builds/install.sh | bash
```

---

## Platform Dependencies

| Platform | Dependencies                                       |
| -------- | -------------------------------------------------- |
| Linux    | `build-essential cmake libcurl4-openssl-dev`       |
| macOS    | Xcode CLI Tools, `cmake` (via Homebrew)            |
| Windows  | Visual Studio Build Tools, CMake, vcpkg (optional) |
