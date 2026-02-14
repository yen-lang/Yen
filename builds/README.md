# Yen - Build & Release

## Quick Build (local)

### Linux
```bash
./builds/build-linux.sh
```
Gera: `dist/yen-1.1.0-linux-x86_64.tar.gz` + `dist/yen-1.1.0-linux-x86_64.deb`

### macOS
```bash
./builds/build-macos.sh
```
Gera: `dist/yen-1.1.0-macos-arm64.tar.gz` + `dist/yen-1.1.0-macos-arm64.pkg`

### Windows
```bat
builds\build-windows.bat
```
Gera: `dist\yen-1.1.0-windows-x64.zip`

## CI/CD (GitHub Actions)

Para ativar o build automatico nas 3 plataformas:

```bash
mkdir -p .github/workflows
cp builds/ci-build.yml .github/workflows/ci-build.yml
git add .github/workflows/ci-build.yml
git commit -m "Add CI/CD build workflow"
git push
```

O workflow:
- Compila em **Linux**, **macOS** e **Windows** automaticamente
- Gera artefatos para download em cada PR/push
- Cria **GitHub Release** com todos os pacotes quando uma tag `v*` e publicada

### Criar um release

```bash
git tag v1.1.0
git push origin v1.1.0
```

## Install script (Linux/macOS)

```bash
curl -fsSL https://raw.githubusercontent.com/yen-lang/yen/main/builds/install.sh | bash
```

## Dependencias por plataforma

| Plataforma | Dependencias |
|-----------|-------------|
| Linux | `build-essential cmake libcurl4-openssl-dev` |
| macOS | Xcode CLI Tools, `cmake` (via Homebrew) |
| Windows | Visual Studio Build Tools, CMake, vcpkg (opcional) |
