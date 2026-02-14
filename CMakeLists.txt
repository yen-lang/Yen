cmake_minimum_required(VERSION 3.10)
project(yen)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# ============================================================================
# LIBCURL DETECTION (for HTTP/HTTPS support)
# ============================================================================

find_package(CURL QUIET)
if(NOT CURL_FOUND)
    # Try Homebrew paths on macOS
    find_library(CURL_LIBRARIES NAMES curl PATHS /opt/homebrew/lib /usr/local/lib)
    find_path(CURL_INCLUDE_DIRS curl/curl.h PATHS /opt/homebrew/include /usr/local/include)
    if(CURL_LIBRARIES AND CURL_INCLUDE_DIRS)
        set(CURL_FOUND TRUE)
    endif()
endif()

if(CURL_FOUND)
    message(STATUS "Found libcurl: ${CURL_LIBRARIES}")
    include_directories(${CURL_INCLUDE_DIRS})
    add_definitions(-DHAVE_LIBCURL)
else()
    message(WARNING "libcurl not found - HTTP client will use raw sockets (no HTTPS)")
endif()

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

# Link libcurl to interpreter
if(CURL_FOUND)
    target_link_libraries(yen ${CURL_LIBRARIES})
endif()

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

# ============================================================================
# INSTALL TARGETS
# ============================================================================

install(TARGETS yen RUNTIME DESTINATION bin)

# Install stdlib files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/stdlib/ DESTINATION lib/yen/stdlib)

# ============================================================================
# CPACK PACKAGING
# ============================================================================

set(CPACK_PACKAGE_NAME "yen")
set(CPACK_PACKAGE_VERSION "1.1.0")
set(CPACK_PACKAGE_VENDOR "Yen Language")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Yen Programming Language Interpreter")
set(CPACK_PACKAGE_DESCRIPTION "A modern, expressive programming language with a tree-walking interpreter")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/yen-lang/yen")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT "yen-lang")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Yen Language")
    set(CPACK_NSIS_PACKAGE_NAME "Yen")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ;productbuild")
    set(CPACK_SYSTEM_NAME "macos")
elseif(UNIX)
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "yen-lang")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4-openssl-dev")
endif()

include(CPack)
