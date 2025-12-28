# Contributing to YEN

Thank you for your interest in contributing to YEN!

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/yen.git`
3. Create a branch: `git checkout -b feature/your-feature`
4. Make your changes
5. Test your changes
6. Commit: `git commit -am "Add feature"`
7. Push: `git push origin feature/your-feature`
8. Create a Pull Request

## Development Setup

### Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- LLVM 17+ (optional, for compiler development)

### Building

```bash
mkdir build && cd build
cmake ..
make
```

### Running Tests

```bash
# Run interpreter tests
./yen tests/test_arithmetic.yen
./yen tests/test_functions.yen
./yen tests/test_loops.yen

# Run all tests
cd tests
./run_type_tests.sh
```

## Code Style

- Use 4 spaces for indentation
- Follow existing code style
- Add comments for complex logic
- Use meaningful variable and function names

## Areas to Contribute

### Language Features

- Improve error messages
- Add new standard library functions
- Optimize interpreter performance
- Enhance type system

### Documentation

- Improve README
- Add examples
- Write tutorials
- Document APIs

### Testing

- Add unit tests
- Add integration tests
- Test edge cases
- Improve test coverage

### Bug Fixes

- Check issues for reported bugs
- Reproduce and fix bugs
- Add regression tests

## Pull Request Guidelines

1. **One feature per PR** - Keep PRs focused
2. **Test your changes** - Ensure all tests pass
3. **Update documentation** - Document new features
4. **Follow code style** - Match existing patterns
5. **Write clear commits** - Describe what and why

## Reporting Bugs

When reporting bugs, please include:

- YEN version
- Operating system
- Steps to reproduce
- Expected vs actual behavior
- Sample code if applicable

## Feature Requests

Feature requests are welcome! Please provide:

- Clear description of the feature
- Use cases and examples
- Why it would benefit YEN users

## Questions?

Feel free to:
- Open an issue for discussion
- Ask in pull request comments
- Contact maintainers

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Celebrate contributions

Thank you for contributing to YEN!
