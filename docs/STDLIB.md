# YEN Standard Library Reference

Complete reference for YEN's native standard library.

## Core Library

Type checking and conversion utilities.

### Type Checking

```yen
core_is_int(value)        // Returns true if value is an integer
core_is_float(value)      // Returns true if value is a float
core_is_bool(value)       // Returns true if value is a boolean
core_is_string(value)     // Returns true if value is a string
core_is_list(value)       // Returns true if value is a list
core_is_map(value)        // Returns true if value is a map
```

### Type Conversion

```yen
core_to_int(value)        // Converts value to integer
core_to_float(value)      // Converts value to float
core_to_string(value)     // Converts value to string
```

## Math Library

Mathematical functions and constants.

### Basic Operations

```yen
math_abs(x)               // Absolute value
math_sqrt(x)              // Square root
math_pow(base, exp)       // Power (base^exp)
math_floor(x)             // Floor value
math_ceil(x)              // Ceiling value
math_round(x)             // Round to nearest integer
math_min(a, b)            // Minimum of two values
math_max(a, b)            // Maximum of two values
```

### Trigonometric Functions

```yen
math_sin(x)               // Sine
math_cos(x)               // Cosine
math_tan(x)               // Tangent
```

### Random Numbers

```yen
math_random()             // Random float [0.0, 1.0)
math_random_int(min, max) // Random integer [min, max]
```

### Constants

```yen
math_PI                   // Pi (3.14159...)
math_E                    // Euler's number (2.71828...)
```

## String Library

String manipulation functions.

```yen
str_length(s)             // Returns string length
str_upper(s)              // Converts to uppercase
str_lower(s)              // Converts to lowercase
str_trim(s)               // Removes leading/trailing whitespace
str_split(s, delim)       // Splits string by delimiter, returns list
str_join(list, sep)       // Joins list of strings with separator
str_substring(s, start, length)  // Extracts substring
str_contains(s, substr)   // Checks if string contains substring
str_replace(s, old, new)  // Replaces all occurrences
```

## Collections Library

List manipulation functions.

```yen
list_push(list, item)     // Adds item to end of list (returns new list)
list_pop(list)            // Removes last item (returns new list)
list_length(list)         // Returns list length
list_reverse(list)        // Reverses list (returns new list)
list_sort(list)           // Sorts list in ascending order (returns new list)
```

## IO Library

File input/output operations.

```yen
io_read_file(path)        // Reads entire file as string
io_write_file(path, content)  // Writes string to file
io_append_file(path, content) // Appends string to file
```

## Filesystem Library

Filesystem operations.

```yen
fs_exists(path)           // Checks if path exists
fs_is_directory(path)     // Checks if path is a directory
fs_is_file(path)          // Checks if path is a file
fs_create_dir(path)       // Creates directory (including parents)
fs_remove(path)           // Removes file or directory
fs_file_size(path)        // Returns file size in bytes
```

## Time Library

Time and sleep utilities.

```yen
time_now()                // Returns current timestamp in milliseconds
time_sleep(ms)            // Sleeps for specified milliseconds
```

## Crypto Library

Cryptographic and hashing functions.

**Note:** These are basic implementations for educational purposes. 
For production use, consider dedicated cryptography libraries.

```yen
crypto_xor(data, key)     // XOR cipher encryption/decryption
crypto_hash(text)         // Simple hash function
crypto_random_bytes(count) // Generates random bytes
```

## Encoding Library

Data encoding utilities.

```yen
encoding_base64_encode(text)  // Encodes string to Base64
encoding_hex_encode(text)     // Encodes string to hexadecimal
```

## Log Library

Logging functions with different severity levels.

```yen
log_info(message)         // Logs info message to stdout
log_warn(message)         // Logs warning message to stdout
log_error(message)        // Logs error message to stderr
```

## Environment Library

Environment variable operations.

```yen
env_get(name)             // Gets environment variable value
env_set(name, value)      // Sets environment variable
```

## Process Library

Process execution and shell commands. See [detailed documentation](PROCESS_SHELL.md).

```yen
process_exec(command)           // Executes command, returns exit code
process_shell(command)          // Executes command, returns output as string
process_spawn(cmd, ...args)     // Executes command with arguments
process_cwd()                   // Returns current working directory
process_chdir(path)             // Changes current directory
```

**Examples:**
```yen
// Execute and get output
var files = process_shell("ls -la");
print files;

// Execute and check result
var result = process_exec("mkdir /tmp/test");
if (result == 0) {
    print "Success!";
}

// Navigation
var original = process_cwd();
process_chdir("/tmp");
// ... do work ...
process_chdir(original);
```

## Examples

### File Processing

```yen
// Read file
var content = io_read_file("data.txt");

// Process
var lines = str_split(content, "\n");
var uppercase = str_upper(content);

// Write result
io_write_file("output.txt", uppercase);
```

### Math Calculations

```yen
var numbers = [3.7, 2.1, 5.9, 1.3];

var sorted = list_sort(numbers);
var sum = 0;
for n in sorted {
    sum = sum + n;
}

var avg = sum / list_length(numbers);
var rounded = math_round(avg);

print "Average: " + rounded;
```

### Crypto Operations

```yen
var plaintext = "secret message";
var key = "mykey";

// Encrypt
var encrypted = crypto_xor(plaintext, key);

// Decrypt
var decrypted = crypto_xor(encrypted, key);

// Hash
var hash = crypto_hash(plaintext);
print "Hash: " + hash;
```

### Logging

```yen
log_info("Application started");

if (fs_exists("config.txt")) {
    log_info("Config file found");
    var config = io_read_file("config.txt");
} else {
    log_warn("Config file not found, using defaults");
}

log_info("Application finished");
```
