@echo off
REM ============================================================================
REM Yen Language - Windows Build Script
REM Builds the interpreter and creates a .zip package
REM Requires: Visual Studio 2019+ or Build Tools, CMake, vcpkg (optional)
REM ============================================================================

setlocal enabledelayedexpansion

set VERSION=1.1.0
set SCRIPT_DIR=%~dp0
set PROJECT_DIR=%SCRIPT_DIR%..
set BUILD_DIR=%PROJECT_DIR%\build
set DIST_DIR=%PROJECT_DIR%\dist

echo === Yen Language - Windows Build ===
echo Version: %VERSION%
echo.

REM Check CMake
echo [1/5] Checking dependencies...
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: cmake not found. Install CMake from https://cmake.org/download/
    echo   Or: choco install cmake
    exit /b 1
)

REM Configure
echo [2/5] Configuring...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Try with vcpkg if available
if defined VCPKG_INSTALLATION_ROOT (
    echo Using vcpkg toolchain: %VCPKG_INSTALLATION_ROOT%
    cmake "%PROJECT_DIR%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%VCPKG_INSTALLATION_ROOT%\scripts\buildsystems\vcpkg.cmake"
) else (
    cmake "%PROJECT_DIR%" -DCMAKE_BUILD_TYPE=Release
)

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM Build
echo [3/5] Building...
cmake --build . --config Release --target yen

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

REM Tests
echo [4/5] Running tests...
cd /d "%PROJECT_DIR%"
if exist tests\run_tests.sh (
    echo NOTE: Tests require bash. Run tests/run_tests.sh manually in Git Bash or WSL.
)

REM Package
echo [5/5] Packaging...
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

REM Create staging directory
set STAGING=%TEMP%\yen-staging
if exist "%STAGING%" rmdir /s /q "%STAGING%"
mkdir "%STAGING%\yen\bin"
mkdir "%STAGING%\yen\lib\yen\stdlib"

REM Copy files
if exist "%BUILD_DIR%\Release\yen.exe" (
    copy "%BUILD_DIR%\Release\yen.exe" "%STAGING%\yen\bin\" >nul
) else if exist "%BUILD_DIR%\yen.exe" (
    copy "%BUILD_DIR%\yen.exe" "%STAGING%\yen\bin\" >nul
)

xcopy "%PROJECT_DIR%\stdlib\*" "%STAGING%\yen\lib\yen\stdlib\" /s /e /q >nul 2>&1
copy "%PROJECT_DIR%\LICENSE" "%STAGING%\yen\" >nul 2>&1

REM Create ZIP using PowerShell
set ZIP_NAME=yen-%VERSION%-windows-x64.zip
powershell -Command "Compress-Archive -Path '%STAGING%\yen' -DestinationPath '%DIST_DIR%\%ZIP_NAME%' -Force"
echo   Created: dist\%ZIP_NAME%

REM Create NSIS installer via CPack if available
cd /d "%BUILD_DIR%"
cpack -G ZIP -C Release >nul 2>&1 && echo   Created CPack ZIP

REM Cleanup
rmdir /s /q "%STAGING%" >nul 2>&1

echo.
echo === Build complete! ===
echo Binary: %BUILD_DIR%\Release\yen.exe
echo Packages: %DIST_DIR%\
dir "%DIST_DIR%\" 2>nul

endlocal
