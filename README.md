# YEN Programming Language

<div align="center">

![YEN Logo](https://avatars.githubusercontent.com/u/209428205?s=200&v=4)

**A modern, expressive programming language**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

</div>

---

## Quick Start

```bash
git clone https://github.com/yen-lang/Yen.git
cd Yen
mkdir build && cd build
cmake ..
make
./yen --version
```

Create `hello.yen`:

```yen
print "Hello, World!";
```

```bash
./yen hello.yen
```

## Features

```yen
// Variables
let name = "Alice";       // immutable
var count = 0;            // mutable

// Functions
func greet(name) {
    print "Hello, " + name;
}

// Lambdas
let square = |x| x * x;

// Pattern matching
match (score) {
    0..60 => print "F";
    60..80 => print "C";
    80..100 => print "A";
    _ => print "Invalid";
}

// Classes and structs
class Person {
    let name;
    let age;

    func greet() {
        print "I'm " + this.name;
    }
}

// Lists and iteration
var nums = [1, 2, 3, 4, 5];
for n in nums {
    print square(n);
}

// Defer, try/catch, ranges, destructuring, spread, pipes, and more
```

## Standard Library

| Module | Examples |
|--------|----------|
| **Math** | `math_sqrt`, `math_pow`, `math_random` |
| **String** | `str_split`, `str_upper`, `str_replace` |
| **Collections** | `list_sort`, `list_reverse`, `list_push` |
| **IO / FS** | `io_read_file`, `io_write_file`, `fs_exists` |
| **Time** | `time_now`, `time_sleep` |
| **Crypto** | `crypto_hash`, `crypto_random_bytes` |
| **Encoding** | `encoding_base64_encode`, `encoding_hex_encode` |
| **Log** | `log_info`, `log_warn`, `log_error` |
| **Env / Process** | `env_get`, `process_exec` |

## Documentation

- [Quick Start Guide](docs/QUICKSTART.md)
- [Language Syntax](docs/SYNTAX.md)
- [Standard Library](docs/STDLIB.md)
- [Using Yen](docs/USING_YEN.md)

## Testing

```bash
cd tests
./run_tests.sh
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

[MIT License](LICENSE)
