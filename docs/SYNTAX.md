# YEN Language Syntax Guide

Complete syntax reference for the YEN programming language.

## Comments

```yen
// Single-line comment

/*
 * Multi-line comment
 */
```

## Variables

### Mutable Variables

```yen
var x = 10;
var name = "Alice";
var items = [1, 2, 3];
```

### Immutable Variables

```yen
let constant = 42;
let pi = 3.14159;
```

### Type Annotations

```yen
var age: int = 25;
var price: float = 19.99;
var name: string = "Bob";
```

## Data Types

### Primitives

```yen
// Integers
var i: int = 42;

// Floats
var f: float = 3.14;

// Booleans
var flag: bool = true;

// Strings
var text: string = "Hello";
```

### Collections

```yen
// Lists
var numbers = [1, 2, 3, 4, 5];
var mixed = [1, "two", 3.0, true];

// Maps (dictionaries)
var person = {
    "name": "Alice",
    "age": 30
};
```

## Operators

### Arithmetic

```yen
var sum = a + b;          // Addition
var diff = a - b;         // Subtraction
var product = a * b;      // Multiplication
var quotient = a / b;     // Division
var remainder = a % b;    // Modulo
var power = a ** b;       // Exponentiation
```

### Comparison

```yen
a == b    // Equal
a != b    // Not equal
a < b     // Less than
a <= b    // Less than or equal
a > b     // Greater than
a >= b    // Greater than or equal
```

### Logical

```yen
a && b    // Logical AND
a || b    // Logical OR
!a        // Logical NOT
```

### Bitwise

```yen
a & b     // Bitwise AND
a | b     // Bitwise OR
a ^ b     // Bitwise XOR
~a        // Bitwise NOT
a << b    // Left shift
a >> b    // Right shift
```

### Compound Assignment

```yen
x += 5;   // x = x + 5
x -= 3;   // x = x - 3
x *= 2;   // x = x * 2
x /= 4;   // x = x / 4
```

## Control Flow

### If-Else

```yen
if (condition) {
    // code
} else if (other_condition) {
    // code
} else {
    // code
}
```

### Loops

```yen
// While loop
while (condition) {
    // code
}

// Infinite loop
loop {
    if (done) break;
}

// For-in loop
for item in collection {
    // code
}

// Range-based for loop
for i in 0..10 {
    // i = 0, 1, 2, ..., 9
}

for i in 0..=10 {
    // i = 0, 1, 2, ..., 10
}
```

### Match Expressions

```yen
match (value) {
    0 => print "zero";
    1 => print "one";
    2..10 => print "small number";
    10..=100 => print "medium number";
    _ => print "large number";
}

// With pattern guards
match (x) {
    n when n > 0 => print "positive";
    n when n < 0 => print "negative";
    _ => print "zero";
}
```

## Functions

### Function Declaration

```yen
func greet(name: string) {
    print "Hello, " + name;
}

func add(a: int, b: int) -> int {
    return a + b;
}
```

### Lambda Expressions

```yen
// Simple lambda
let square = |x| x * x;

// Multiple parameters
let add = |a, b| a + b;

// With closures
var multiplier = 10;
let multiply = |x| x * multiplier;
```

## Data Structures

### Structs

```yen
struct Point {
    x;
    y;
}

var p = Point { x: 10, y: 20 };
print p.x;
```

### Classes

```yen
class Person {
    let name;
    let age;

    func greet() {
        print "Hello, I'm " + this.name;
    }

    func birthday() {
        this.age = this.age + 1;
    }
}

var person = Person {
    name: "Alice",
    age: 30
};

person.greet();
person.birthday();
```

### Enums

```yen
enum Color {
    Red,
    Green,
    Blue
}

match (color) {
    Color.Red => print "Red";
    Color.Green => print "Green";
    Color.Blue => print "Blue";
}
```

## Advanced Features

### Defer Statements

```yen
func process_file(path: string) {
    var file = open(path);
    defer close(file);  // Executed when scope exits
    
    // Process file...
    // file is automatically closed
}
```

### Assertions

```yen
assert(x > 0, "x must be positive");
assert(list_length(items) == 5);
```

### Range Expressions

```yen
var range = 0..10;        // Exclusive: [0, 1, ..., 9]
var inclusive = 0..=10;   // Inclusive: [0, 1, ..., 10]

for i in range {
    print i;
}
```

### Pattern Matching

```yen
// Literal patterns
match (x) {
    42 => print "the answer";
    _ => print "not the answer";
}

// Range patterns
match (score) {
    0..60 => print "F";
    60..70 => print "D";
    70..80 => print "C";
    80..90 => print "B";
    90..=100 => print "A";
    _ => print "Invalid score";
}

// Variable binding
match (value) {
    x => print "Got: " + x;
}

// Tuple patterns (future)
match (point) {
    (0, 0) => print "Origin";
    (x, 0) => print "On X axis";
    (0, y) => print "On Y axis";
    (x, y) => print "Point at " + x + "," + y;
}
```

## Modules and Imports

```yen
// Import module
import "path/to/module.yen";

// Export (future feature)
export func public_function() {
    // ...
}
```

## String Interpolation

```yen
var name = "Alice";
var age = 30;

// Future feature
// print "${name} is ${age} years old";

// Current workaround
print name + " is " + age + " years old";
```

## Best Practices

### Use `let` for Constants

```yen
let MAX_SIZE = 100;
let CONFIG_PATH = "/etc/config";
```

### Use Type Annotations for Clarity

```yen
func calculate_price(quantity: int, price: float) -> float {
    return quantity * price;
}
```

### Handle Errors with Assertions

```yen
func divide(a: int, b: int) -> int {
    assert(b != 0, "Division by zero");
    return a / b;
}
```

### Use Defer for Resource Cleanup

```yen
func process_data(path: string) {
    var resource = acquire_resource(path);
    defer release_resource(resource);
    
    // Process data
    // Resource is automatically released
}
```

### Prefer Pattern Matching over If-Else Chains

```yen
// Instead of:
if (x == 1) {
    // ...
} else if (x == 2) {
    // ...
} else {
    // ...
}

// Use:
match (x) {
    1 => { /* ... */ }
    2 => { /* ... */ }
    _ => { /* ... */ }
}
```
