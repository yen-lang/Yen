cmake_minimum_required(VERSION 3.10)
project(yen)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# ============================================================================
# SOURCE FILES
# ============================================================================

# Interpreter sources (always built)
set(INTERPRETER_SOURCES
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/stdlib.cpp
    src/type_checker.cpp
    src/native_libs.cpp
)

# Compiler sources (requires LLVM)
set(COMPILER_SOURCES
    src/compiler_main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/type_checker.cpp
    src/llvm_codegen.cpp
)

# ============================================================================
# LLVM DETECTION
# ============================================================================

# Set LLVM directory hint for find_package
set(LLVM_DIR "/usr/local/llvm-17/lib/cmake/llvm")

# Try to find LLVM
find_package(LLVM CONFIG QUIET)

if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")

    # LLVM definitions and includes
    add_definitions(${LLVM_DEFINITIONS})
    include_directories(${LLVM_INCLUDE_DIRS})

    # Define HAVE_LLVM for conditional compilation
    add_definitions(-DHAVE_LLVM)

    # Get LLVM libraries
    llvm_map_components_to_libnames(LLVM_LIBS
        core
        support
        irreader
        passes
        target
        transformutils
        analysis
        scalaropts
        vectorize
        ipo
        nativecodegen
        aarch64asmparser
        aarch64codegen
        aarch64desc
        aarch64info
    )

    set(HAVE_LLVM TRUE)
    message(STATUS "YEN compiler (yenc) will be built")
else()
    message(WARNING "LLVM not found - compiler (yenc) will not be built")
    message(WARNING "Only the interpreter (yen) will be available")
    message(WARNING "See LLVM_SETUP.md for installation instructions")
    set(HAVE_LLVM FALSE)
endif()

# ============================================================================
# EXECUTABLES
# ============================================================================

# Build interpreter (always)
add_executable(yen ${INTERPRETER_SOURCES})

# Build compiler (only if LLVM is available)
if(HAVE_LLVM)
    add_executable(yenc ${COMPILER_SOURCES})
    target_link_libraries(yenc ${LLVM_LIBS})

    # LLVM requires these flags on some platforms
    if(UNIX AND NOT APPLE)
        target_link_libraries(yenc pthread dl)
    endif()

    message(STATUS "Building both yen (interpreter) and yenc (compiler)")
else()
    message(STATUS "Building only yen (interpreter)")
endif()
