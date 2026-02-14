# Yen Language Fundamentals Course

A structured course covering the Yen programming language from the ground up.
3 modules, 5 lessons each, with code examples and exercises.

---

## Module 1: Core Foundations

### Lesson 1.1 -- Introduction to Yen

**What is Yen?**

Yen is a modern, dynamically-typed programming language designed for scripting, automation, and general-purpose programming. It combines a clean, expressive syntax with powerful features inspired by languages like Python, Rust, and Swift.

**Key characteristics:**
- Dynamic typing with optional type annotations
- Tree-walking interpreter (instant startup, no compilation step)
- Rich standard library (12 modules)
- First-class functions and lambdas
- Pattern matching, classes, traits, and more

**Running your first program:**

Create a file called `hello.yen`:

```yen
print "Hello, Yen!";
```

Run it:

```bash
./yen hello.yen
```

**Comments:**

```yen
// This is a single-line comment

/*
 * This is a
 * multi-line comment
 */
```

**The `print` statement:**

`print` is the primary way to output values. It automatically converts values to strings.

```yen
print "Hello, World!";
print 42;
print true;
print [1, 2, 3];
```

**Exercise 1.1:** Create a file that prints your name, your favorite number, and the phrase "I am learning Yen!" on three separate lines.

---

### Lesson 1.2 -- Variables and Data Types

**Variable declarations:**

Yen has two keywords for declaring variables:

- `let` -- declares an **immutable** (read-only) binding
- `var` -- declares a **mutable** (changeable) binding

```yen
let name = "Alice";    // immutable -- cannot be reassigned
var age = 25;          // mutable -- can be reassigned

age = 26;              // OK
// name = "Bob";       // ERROR: cannot reassign immutable variable
```

**Primitive data types:**

```yen
// Integers
let count = 42;

// Floating-point numbers
let pi = 3.14159;

// Booleans
let active = true;
let done = false;

// Strings
let greeting = "Hello, World!";

// None (null value)
let nothing = None;
```

**Optional type annotations:**

```yen
var age: int = 25;
var price: float = 19.99;
var name: string = "Bob";
var flag: bool = true;
```

**Type checking with `is`:**

```yen
print 42 is int;        // true
print 3.14 is float;    // true
print "hi" is string;   // true
print true is bool;     // true
print [1,2] is list;    // true
print None is None;     // true
```

**Type conversion:**

```yen
let num = 42;
let text = str(num);        // "42"
let back = core_to_int("42");  // 42
let f = core_to_float("3.14"); // 3.14
```

**Exercise 1.2:** Declare variables for a person's first name (immutable), last name (immutable), and age (mutable). Print each one, then increment the age by 1 and print it again.

---

### Lesson 1.3 -- Operators and Expressions

**Arithmetic operators:**

```yen
print 10 + 3;    // 13   (addition)
print 10 - 3;    // 7    (subtraction)
print 10 * 3;    // 30   (multiplication)
print 10 / 3;    // 3.33 (division)
print 10 % 3;    // 1    (modulo / remainder)
print 2 ** 10;   // 1024 (exponentiation)
```

**String concatenation and multiplication:**

```yen
print "Hello" + ", " + "World!";  // Hello, World!
print "-" * 20;                    // --------------------
print 3 * "ha";                    // hahaha
```

**Comparison operators:**

```yen
print 5 == 5;    // true
print 5 != 3;    // true
print 5 < 10;    // true
print 5 <= 5;    // true
print 5 > 3;     // true
print 5 >= 5;    // true
```

**Chained comparisons** (a Yen-exclusive feature):

```yen
let val = 5;
print 1 < val < 10;     // true  (equivalent to: 1 < val && val < 10)
print 0 <= val <= 5;     // true
```

**Logical operators:**

```yen
print true && false;   // false  (AND)
print true || false;    // true   (OR)
print !true;            // false  (NOT)
```

**Compound assignment:**

```yen
var x = 10;
x += 5;    // x is now 15
x -= 3;    // x is now 12
x *= 2;    // x is now 24
x /= 4;    // x is now 6
x %= 4;    // x is now 2
```

**Increment and decrement:**

```yen
var counter = 0;
counter++;   // counter is now 1
counter++;   // counter is now 2
counter--;   // counter is now 1
```

**The `in` operator:**

Test membership in lists, maps, and strings:

```yen
let fruits = ["apple", "banana", "cherry"];
print "apple" in fruits;       // true
print "grape" in fruits;       // false

let config = {"host": "localhost", "port": 8080};
print "host" in config;        // true

let sentence = "The quick brown fox";
print "quick" in sentence;     // true
```

**Exercise 1.3:** Write a program that:
1. Creates a list of 5 numbers
2. Uses `in` to check if the number 42 is in the list
3. Computes the sum of the first and last elements using indexing
4. Prints a border of `"="` repeated 30 times

---

### Lesson 1.4 -- Control Flow

**If / else if / else:**

```yen
let score = 85;

if (score >= 90) {
    print "Excellent!";
} else if (score >= 70) {
    print "Good job!";
} else {
    print "Keep trying!";
}
```

**While loop:**

```yen
var count = 0;
while (count < 5) {
    print "Count: " + str(count);
    count++;
}
```

**Do-while loop** (runs at least once):

```yen
var i = 0;
do {
    print "i = " + str(i);
    i += 1;
} while (i < 3);
```

**For-in loop** (iterate over collections):

```yen
let names = ["Alice", "Bob", "Charlie"];
for name in names {
    print "Hello, " + name + "!";
}
```

**Range-based for loops:**

```yen
// Exclusive range: 0, 1, 2, 3, 4
for i in 0..5 {
    print i;
}

// Inclusive range: 0, 1, 2, 3, 4, 5
for i in 0..=5 {
    print i;
}
```

**Loop (infinite loop with break):**

```yen
var x = 0;
loop {
    x += 1;
    if (x == 5) { break; }
}
print x;  // 5
```

**Break and continue:**

```yen
for i in 0..10 {
    if (i % 2 == 0) { continue; }  // skip even numbers
    if (i > 7) { break; }          // stop at 7
    print i;                        // prints: 1, 3, 5, 7
}
```

**Unless and until** (inverted conditions):

```yen
let val = 5;
unless (val == 0) {
    print "val is not zero";
}

var n = 0;
until (n >= 3) {
    n++;
}
print n;  // 3
```

**Repeat loop:**

```yen
repeat 5 {
    print "Hello!";
}

// With iteration variable
var items = [];
repeat 3 as i {
    items = items + [i];
}
print items;  // [0, 1, 2]
```

**Exercise 1.4:** Write a FizzBuzz program: for numbers 1 to 20, print "Fizz" if divisible by 3, "Buzz" if divisible by 5, "FizzBuzz" if divisible by both, or the number itself otherwise. Use a range-based for loop.

---

### Lesson 1.5 -- Functions

**Declaring functions:**

```yen
func greet(name) {
    print "Hello, " + name + "!";
}

greet("World");  // Hello, World!
```

**Return values:**

```yen
func add(a, b) {
    return a + b;
}

let result = add(10, 20);
print result;  // 30
```

**Default parameters:**

```yen
func greet(name, greeting = "Hello") {
    return greeting + ", " + name + "!";
}

print greet("Alice");          // Hello, Alice!
print greet("Bob", "Hi");     // Hi, Bob!
```

**Multiple default parameters:**

```yen
func make_point(x, y = 0, z = 0) {
    return [x, y, z];
}

print make_point(1);         // [1, 0, 0]
print make_point(1, 2);     // [1, 2, 0]
print make_point(1, 2, 3);  // [1, 2, 3]
```

**Named arguments:**

```yen
func calc(a, b, c) {
    return a - b + c;
}

print calc(c: 30, a: 10, b: 5);  // 35
```

**Functions as first-class values:**

```yen
func apply(f, val) {
    return f(val);
}

func double(x) {
    return x * 2;
}

print apply(double, 21);  // 42
```

**Guard clauses:**

```yen
func process(val) {
    guard val > 0 else { return "must be positive"; }
    return val * 2;
}

print process(5);   // 10
print process(-1);  // must be positive
```

**Recursion:**

```yen
func factorial(n) {
    if (n <= 1) { return 1; }
    return n * factorial(n - 1);
}

print factorial(5);   // 120
print factorial(10);  // 3628800
```

**Exercise 1.5:** Write a recursive function `fibonacci(n)` that returns the nth Fibonacci number. Then write a function `fib_list(count)` that builds and returns a list of the first `count` Fibonacci numbers using a for-in loop with a range.

---

## Module 2: Collections, Strings, and Patterns

### Lesson 2.1 -- Lists

**Creating lists:**

```yen
let empty = [];
let numbers = [1, 2, 3, 4, 5];
let mixed = [1, "two", 3.0, true, None];
let nested = [[1, 2], [3, 4], [5, 6]];
```

**Accessing elements:**

```yen
let list = [10, 20, 30, 40, 50];

print list[0];   // 10  (first element)
print list[4];   // 50  (last element)
print list[-1];  // 50  (negative indexing: last element)
print list[-2];  // 40  (second to last)
```

**Modifying lists:**

```yen
var list = [10, 20, 30];

// Push (add to end)
push(list, 40);
print list;  // [10, 20, 30, 40]

// Index assignment
list[0] = 99;
print list;  // [99, 20, 30, 40]

// Negative index assignment
list[-1] = 0;
print list;  // [99, 20, 30, 0]
```

**Slicing:**

```yen
let nums = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

print nums[0:3];   // [0, 1, 2]     (start:end, exclusive)
print nums[3:7];   // [3, 4, 5, 6]
print nums[:5];    // [0, 1, 2, 3, 4]  (from beginning)
print nums[7:];    // [7, 8, 9]        (to end)
print nums[-3:];   // [7, 8, 9]        (last 3 elements)
```

**List utility functions:**

```yen
let list = [3, 1, 4, 1, 5, 9];

print len(list);              // 6
print list_contains(list, 4); // true
print list_sort(list);        // [1, 1, 3, 4, 5, 9]
print list_reverse(list);     // [9, 5, 1, 4, 1, 3]

let sliced = list_slice(list, 1, 4);
print sliced;  // [1, 4, 1]
```

**Method syntax on lists:**

```yen
print [3, 1, 2].sort();         // [1, 2, 3]
print [1, 2, 3].reverse();      // [3, 2, 1]
print [1, 2, 3].contains(2);    // true
print [1, 2, 3].length();       // 3
print ["a", "b", "c"].join("-"); // a-b-c
print [1, 2, 3].push(4);        // [1, 2, 3, 4]
```

**Nested list access:**

```yen
let matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
print matrix[0][0];  // 1
print matrix[1][1];  // 5
print matrix[2][2];  // 9
```

**Exercise 2.1:** Create a list of 10 numbers. Write a program that:
1. Prints the first 3 and last 3 using slicing
2. Reverses the list and prints it
3. Checks if the number 42 is in the list using `in`
4. Creates a new list combining the first half and last half with a `[0]` element in the middle

---

### Lesson 2.2 -- Maps (Dictionaries)

**Creating maps:**

```yen
let person = {
    "name": "Alice",
    "age": 30,
    "city": "New York"
};
```

**Accessing values:**

```yen
print person["name"];   // Alice
print person["age"];    // 30
```

**Map operations:**

```yen
var config = {};

// Set values
map_set(config, "host", "localhost");
map_set(config, "port", 8080);
map_set(config, "debug", true);

// Get with default
print map_get(config, "host", "unknown");     // localhost
print map_get(config, "missing", "default");  // default

// Check existence
print map_has(config, "host");   // true
print map_has(config, "ssl");    // false

// Size
print map_size(config);  // 3

// Get all keys
let keys = map_keys(config);
print keys;  // [host, port, debug]
```

**Merging maps:**

```yen
let defaults = {"color": "blue", "size": 12};
let custom = {"color": "red", "font": "Arial"};
let merged = map_merge(defaults, custom);
print merged;
// {"color": "red", "size": 12, "font": "Arial"}
```

**Using `in` with maps:**

```yen
let settings = {"theme": "dark", "lang": "en"};
print "theme" in settings;      // true
print "password" in settings;   // false
```

**Iterating over maps with `map_entries`:**

```yen
let scores = {"Alice": 95, "Bob": 87, "Charlie": 92};
let entries = map_entries(scores);

for [name, score] in entries {
    print name + " scored " + str(score);
}
```

**Exercise 2.2:** Build a simple phonebook program:
1. Create a map with 3 contacts (name -> phone number)
2. Add a new contact using `map_set`
3. Look up a contact that exists and one that doesn't (use `map_get` with a default)
4. Print all contacts using `map_entries` and for-in destructuring

---

### Lesson 2.3 -- Strings

**String basics:**

```yen
let greeting = "Hello, World!";
print str_length(greeting);  // 13
```

**String interpolation:**

```yen
let name = "Alice";
let age = 30;
print "My name is ${name} and I am ${age} years old.";
// My name is Alice and I am 30 years old.

// Expressions inside interpolation
print "2 + 3 = ${2 + 3}";  // 2 + 3 = 5
```

**String functions:**

```yen
print str_upper("hello");                  // HELLO
print str_lower("HELLO");                  // hello
print str_trim("  hello  ");              // hello
print str_split("a,b,c", ",");            // [a, b, c]
print str_join(["a", "b", "c"], "-");     // a-b-c
print str_contains("hello world", "world"); // true
print str_replace("hello world", "world", "Yen");  // hello Yen
print str_substring("Hello", 0, 3);       // Hel
```

**Method syntax on strings:**

```yen
print "hello".upper();               // HELLO
print "WORLD".lower();               // world
print "  hi  ".trim();               // hi
print "a,b,c".split(",");            // [a, b, c]
print "hello".replace("l", "r");     // herro
print "hello".contains("ell");       // true
print "hello".starts_with("he");     // true
print "hello".ends_with("lo");       // true
print "hello".reverse();             // olleh
print "abc".chars();                 // [a, b, c]
print "hello".length();              // 5
```

**String indexing and slicing:**

```yen
let msg = "Hello, World!";
print msg[0];      // H
print msg[-1];     // !
print msg[0:5];    // Hello
print msg[7:12];   // World
print msg[-6:];    // World!
```

**String multiplication:**

```yen
print "-" * 40;        // ----------------------------------------
print "abc" * 3;       // abcabcabc
print 3 * "ha";        // hahaha
print "test" * 0;      // (empty string)
```

**Using `in` with strings:**

```yen
print "world" in "Hello, World!";  // false (case-sensitive)
print "World" in "Hello, World!";  // true
```

**Raw strings and multiline strings:**

```yen
// Raw strings: no escape processing
let path = r"C:\Users\no\escapes";
print path;  // C:\Users\no\escapes

// Multiline strings
let text = """
line one
line two
line three""";
print text;
```

**Exercise 2.3:** Write a program that:
1. Takes a sentence like "the quick brown fox jumps over the lazy dog"
2. Splits it into words
3. Prints how many words there are
4. Joins the words back with " | " as separator
5. Uses string interpolation to print: "Original had X words"

---

### Lesson 2.4 -- Pattern Matching

**Basic match:**

```yen
let x = 42;
match (x) {
    0 => print "zero";
    1 => print "one";
    42 => print "the answer";
    _ => print "something else";
}
// Output: the answer
```

The `_` is the wildcard/default case.

**Matching strings:**

```yen
let lang = "Yen";
match (lang) {
    "Rust" => print "Systems language";
    "Python" => print "Scripting language";
    "Yen" => print "Modern language!";
    _ => print "Unknown";
}
```

**Range patterns:**

```yen
let score = 85;
match (score) {
    0..60 => print "F";
    60..70 => print "D";
    70..80 => print "C";
    80..90 => print "B";
    90..=100 => print "A";
    _ => print "Invalid";
}
// Output: B
```

Ranges with `..` are exclusive of the end; `..=` is inclusive.

**Matching booleans:**

```yen
let flag = true;
match (flag) {
    true => print "enabled";
    false => print "disabled";
}
```

**Block bodies in match arms:**

```yen
let val = 3;
match (val) {
    1 => {
        print "one";
        print "the loneliest number";
    }
    2 => print "two";
    _ => {
        print "value: " + str(val);
        print "not one or two";
    }
}
```

**Exercise 2.4:** Write a function `classify_temp(temp)` that uses `match` with ranges to classify a temperature:
- Below 0: "Freezing"
- 0 to 15: "Cold"
- 15 to 25: "Comfortable"
- 25 to 35: "Warm"
- 35 and above: "Hot"

Test it with several values.

---

### Lesson 2.5 -- Error Handling

**try / catch:**

```yen
try {
    throw "Something went wrong";
} catch (e) {
    print "Caught: " + e;
}
// Output: Caught: Something went wrong
```

**Catching runtime errors:**

```yen
try {
    let x = 10 / 0;
    print "This won't print";
} catch (e) {
    print "Error: " + e;
}
// Output: Error: Division by zero
```

**try / catch / finally:**

The `finally` block always runs, whether an error occurred or not.

```yen
var cleanup_ran = false;

try {
    print "Doing work...";
    throw "oops";
} catch (e) {
    print "Caught: " + e;
} finally {
    cleanup_ran = true;
    print "Cleaning up...";
}

print "Cleanup ran: " + str(cleanup_ran);
// Output:
// Doing work...
// Caught: oops
// Cleaning up...
// Cleanup ran: true
```

**Nested try/catch:**

```yen
try {
    try {
        throw "inner error";
    } catch (e) {
        print "Inner caught: " + e;
        throw "outer error";
    }
} catch (e) {
    print "Outer caught: " + e;
}
// Output:
// Inner caught: inner error
// Outer caught: outer error
```

**Using `throw` to signal errors:**

```yen
func divide(a, b) {
    if (b == 0) {
        throw "Cannot divide by zero";
    }
    return a / b;
}

try {
    print divide(10, 2);   // 5
    print divide(10, 0);   // throws!
} catch (e) {
    print "Error: " + e;
}
```

**Assertions:**

```yen
assert(5 > 3, "5 should be greater than 3");
assert(len([1,2,3]) == 3, "list should have 3 elements");
// Throws an error if the condition is false
```

**Exercise 2.5:** Write a `safe_divide(a, b)` function that:
1. Throws an error if `b` is zero
2. Throws an error if either argument is not a number (use `is int` or `is float`)
3. Returns the result if inputs are valid

Then call it multiple times inside a try/catch to test: a normal division, a divide-by-zero, and a non-numeric input.

---

## Module 3: Advanced Features

### Lesson 3.1 -- Lambdas and Higher-Order Functions

**Lambda expressions:**

Lambdas are anonymous functions defined with `|params| body`.

```yen
// Expression lambda (single expression)
let square = |x| x * x;
print square(5);   // 25

let add = |a, b| a + b;
print add(10, 20); // 30
```

**Block lambdas (multi-line):**

```yen
let compute = |x| {
    var result = x * 2;
    result = result + 10;
    return result;
};
print compute(5);  // 20
```

**Lambda with default parameters:**

```yen
let add = |a, b = 10| a + b;
print add(5);      // 15
print add(5, 20);  // 25
```

**Closures (capturing outer variables):**

```yen
var multiplier = 10;
let multiply = |x| x * multiplier;
print multiply(5);  // 50
```

**map -- transform every element:**

```yen
let nums = [1, 2, 3, 4, 5];
let doubled = map(nums, |x| x * 2);
print doubled;  // [2, 4, 6, 8, 10]
```

**filter -- keep elements matching a condition:**

```yen
let nums = [1, 2, 3, 4, 5, 6, 7, 8];
let evens = filter(nums, |x| x % 2 == 0);
print evens;  // [2, 4, 6, 8]
```

**reduce -- accumulate a single result:**

```yen
let nums = [1, 2, 3, 4, 5];
let sum = reduce(nums, 0, |acc, x| acc + x);
print sum;  // 15

let product = reduce(nums, 1, |acc, x| acc * x);
print product;  // 120
```

**foreach -- iterate with side effects:**

```yen
let names = ["Alice", "Bob", "Charlie"];
foreach(names, |name| print "Hello, " + name + "!");
```

**sort_by -- custom sorting:**

```yen
let words = ["banana", "apple", "cherry"];
let sorted = sort_by(words, |w| str_length(w));
print sorted;  // [apple, banana, cherry] (sorted by length)
```

**find -- first matching element:**

```yen
let nums = [5, 3, 8, 1, 9];
let found = find(nums, |x| x > 6);
print found;  // 8
```

**any / all -- boolean checks:**

```yen
let nums = [1, 2, 3, 4, 5];
print any(nums, |x| x > 4);   // true  (at least one > 4)
print all(nums, |x| x > 0);   // true  (all are > 0)
print all(nums, |x| x > 3);   // false (not all > 3)
```

**More higher-order functions:**

```yen
// flat_map -- map + flatten
let nested = [[1, 2], [3, 4], [5]];
let flat = flat_map(nested, |x| x);
print flat;  // [1, 2, 3, 4, 5]

// zip -- pair elements from two lists
let keys = ["a", "b", "c"];
let vals = [1, 2, 3];
let pairs = zip(keys, vals);
print pairs;  // [[a, 1], [b, 2], [c, 3]]

// enumerate -- pairs of [index, value]
let items = ["x", "y", "z"];
let indexed = enumerate(items);
print indexed;  // [[0, x], [1, y], [2, z]]

// take / drop
let nums = [1, 2, 3, 4, 5];
print take(nums, 3);  // [1, 2, 3]
print drop(nums, 3);  // [4, 5]

// group_by
let words = ["cat", "car", "dog", "cow", "dart"];
let grouped = group_by(words, |w| w[0]);
print map_size(grouped);  // 3 (c, d groups)

// map_filter -- map and discard None results
let result = map_filter([1,2,3,4,5], |x| x > 3 ? x * 10 : None);
print result;  // [40, 50]
```

**Exercise 3.1:** Given a list of numbers `[12, 5, 8, 130, 44, 3, 77, 9]`:
1. Use `filter` to keep only numbers greater than 10
2. Use `map` to double each filtered number
3. Use `reduce` to compute the total sum
4. Use `sort_by` to sort by value
5. Use `find` to get the first number greater than 100

---

### Lesson 3.2 -- Classes and Object-Oriented Programming

**Defining a class:**

```yen
class Person {
    let name;
    let age;

    func init(name, age) {
        this.name = name;
        this.age = age;
    }

    func greet() {
        return "Hi, I'm " + this.name;
    }

    func birthday() {
        this.age = this.age + 1;
    }
}

let alice = Person("Alice", 30);
print alice.greet();     // Hi, I'm Alice
alice.birthday();
print alice.age;         // 31
```

**The `toString()` protocol:**

```yen
class Point {
    let x;
    let y;
    func init(x, y) {
        this.x = x;
        this.y = y;
    }
    func toString() {
        return "Point(${this.x}, ${this.y})";
    }
}

let p = Point(3, 4);
print p;  // Point(3, 4)
```

**Inheritance with `extends`:**

```yen
class Animal {
    let name;
    let sound;
    func init(name) {
        this.name = name;
        this.sound = "...";
    }
    func speak() {
        return this.name + " says " + this.sound;
    }
}

class Dog extends Animal {
    func init(name) {
        super.init(name);
        this.sound = "Woof!";
    }
    func fetch() {
        return this.name + " fetches the ball!";
    }
}

let dog = Dog("Rex");
print dog.speak();   // Rex says Woof!
print dog.fetch();   // Rex fetches the ball!
```

**Static methods and fields:**

```yen
class MathUtils {
    static let PI = 3.14159;
    static func square(x) { return x * x; }
    static func cube(x) { return x * x * x; }
}

print MathUtils.PI;          // 3.14159
print MathUtils.square(5);   // 25
```

**Access modifiers (`pub` / `priv`):**

```yen
class Account {
    priv let balance;
    pub let owner;

    func init(owner, balance) {
        this.owner = owner;
        this.balance = balance;
    }

    pub func getBalance() { return this.balance; }
    priv func secret() { return "secret!"; }
    pub func reveal() { return this.secret(); }
}

let acc = Account("Alice", 1000);
print acc.owner;         // Alice
print acc.getBalance();  // 1000

try {
    print acc.balance;   // throws: private field
} catch (e) {
    print "Access denied";
}
```

**Getters and setters:**

```yen
class Circle {
    let radius;
    func init(r) { this.radius = r; }

    get area() {
        return 3.14159 * this.radius * this.radius;
    }
    set area(a) {
        this.radius = (a / 3.14159) ** 0.5;
    }
}

let c = Circle(5);
print c.area;          // ~78.5
c.area = 314.159;      // sets radius to ~10
print c.radius;        // ~10
```

**Operator overloading:**

```yen
class Vec2 {
    let x; let y;
    func init(x, y) { this.x = x; this.y = y; }

    func __add(other) { return Vec2(this.x + other.x, this.y + other.y); }
    func __sub(other) { return Vec2(this.x - other.x, this.y - other.y); }
    func __eq(other) { return this.x == other.x && this.y == other.y; }
    func __neg() { return Vec2(-this.x, -this.y); }

    func toString() { return "Vec2(${this.x}, ${this.y})"; }
}

let a = Vec2(1, 2);
let b = Vec2(3, 4);
print a + b;        // Vec2(4, 6)
print b - a;        // Vec2(2, 2)
print a == Vec2(1, 2);  // true
print -a;           // Vec2(-1, -2)
```

**Traits and `impl`:**

```yen
trait Printable {
    func display();
}

trait Serializable {
    func serialize();
    func defaultMethod() {
        return "default from trait";
    }
}

class User {
    let name;
    let email;
    func init(name, email) { this.name = name; this.email = email; }
    func display() { return "User: " + this.name; }
    func serialize() { return this.name + ":" + this.email; }
}

impl Printable for User { }
impl Serializable for User { }

let user = User("Alice", "alice@example.com");
print user.display();       // User: Alice
print user.serialize();     // Alice:alice@example.com
print user.defaultMethod(); // default from trait

// Type checking with traits
print user is Printable;     // true
print user is Serializable;  // true
```

**Abstract classes:**

```yen
class Shape {
    func area();       // abstract: no body
    func perimeter();  // abstract: no body
    func describe() { return "I am a shape"; }
}

class Rect extends Shape {
    let w; let h;
    func init(w, h) { this.w = w; this.h = h; }
    func area() { return this.w * this.h; }
    func perimeter() { return 2 * (this.w + this.h); }
}

let r = Rect(5, 3);
print r.area();       // 15
print r.perimeter();  // 16
print r.describe();   // I am a shape (inherited)
```

**Exercise 3.2:** Create a `Shape` hierarchy:
1. An abstract `Shape` class with abstract `area()` and `toString()` methods
2. A `Circle` class (radius) and `Rectangle` class (width, height)
3. Both should implement `area()` and `toString()`
4. Add operator overloading `__eq` to compare shapes by area
5. Create instances and test all methods

---

### Lesson 3.3 -- Destructuring, Spread, Pipe, and Ternary

**Destructuring assignment:**

```yen
// List destructuring with let
let [x, y, z] = [10, 20, 30];
print x;  // 10
print y;  // 20
print z;  // 30

// Destructuring with var (mutable)
var [a, b] = [1, 2];
a = 99;
print a;  // 99

// Destructuring from function return
func get_point() {
    return [3, 4];
}
let [px, py] = get_point();
print "${px}, ${py}";  // 3, 4
```

**For-in destructuring:**

```yen
// With enumerate
let items = ["a", "b", "c"];
for [i, val] in enumerate(items) {
    print "${i}: ${val}";
}
// 0: a
// 1: b
// 2: c

// With zip
let names = ["Alice", "Bob"];
let ages = [30, 25];
for [name, age] in zip(names, ages) {
    print "${name} is ${age}";
}

// With manual pairs
let pairs = [[1, "one"], [2, "two"], [3, "three"]];
for [num, word] in pairs {
    print "${num} = ${word}";
}
```

**Spread operator (`...`):**

```yen
// In list literals
let a = [1, 2, 3];
let b = [4, 5, 6];
let merged = [...a, ...b];
print merged;  // [1, 2, 3, 4, 5, 6]

let extended = [0, ...a, 99];
print extended;  // [0, 1, 2, 3, 99]

// In function calls
func sum3(x, y, z) {
    return x + y + z;
}
let args = [10, 20, 30];
print sum3(...args);  // 60
```

**Pipe operator (`|>`):**

The pipe operator passes the left-hand value as the first argument to the right-hand function.

```yen
func double(x) { return x * 2; }
func add_ten(x) { return x + 10; }
func negate(x) { return -x; }

let result = 5 |> double;
print result;  // 10

// Chained pipes
let result2 = 5 |> double |> add_ten;
print result2;  // 20

let result3 = 3 |> double |> negate;
print result3;  // -6

// With expressions
let result4 = (2 + 3) |> double |> add_ten;
print result4;  // 20
```

**Ternary operator:**

```yen
let x = true ? 1 : 0;
print x;  // 1

let grade = 85;
let letter = grade >= 90 ? "A"
           : grade >= 80 ? "B"
           : grade >= 70 ? "C"
           : "F";
print letter;  // B

let a = 10;
let b = 20;
let max_val = a > b ? a : b;
print max_val;  // 20
```

**Null coalescing operator (`??`):**

Returns the left-hand value if it's not `None`, otherwise returns the right-hand value.

```yen
let val = None ?? "default";
print val;  // default

let val2 = 42 ?? "default";
print val2;  // 42

// Chaining
let first = None;
let second = None;
let third = "found";
let result = first ?? second ?? third;
print result;  // found
```

**Optional chaining (`?.`):**

```yen
class Address {
    let city;
    func init(c) { this.city = c; }
}
class Person {
    let name;
    let address;
    func init(n, a) { this.name = n; this.address = a; }
}

let p1 = Person("Alice", Address("NYC"));
print p1?.name;             // Alice
print p1?.address?.city;    // NYC

let p2 = None;
print p2?.name;             // None (no error!)
```

**Exercise 3.3:** Write a data processing pipeline:
1. Start with a list of numbers `[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]`
2. Use destructuring: `let [first, ...rest] = ...` (or manually split using slicing)
3. Use spread to create a new list: `[0, ...original, 11]`
4. Write 3 functions: `double`, `add_one`, `to_string`
5. Use the pipe operator to transform a single value through all three
6. Use the ternary operator to classify each number as "even" or "odd"
7. Use null coalescing to provide default values for potentially None variables

---

### Lesson 3.4 -- Enums, Structs, and the Module System

**Enums:**

```yen
enum Color {
    Red,
    Green,
    Blue
}

let c = Color.Red;
match (c) {
    Color.Red => print "Red";
    Color.Green => print "Green";
    Color.Blue => print "Blue";
}
```

**Structs:**

Structs are lightweight data containers:

```yen
struct Point {
    x;
    y;
}

var p = Point { x: 10, y: 20 };
print p.x;  // 10
print p.y;  // 20
```

**Importing modules:**

Yen has a rich set of built-in modules:

```yen
import 'regex';      // Regular expressions
import 'os';         // Operating system functions
import 'net';        // Networking
import 'async';      // Async operations
```

**The Standard Library (always available, no import needed):**

```yen
// Math
print math_sqrt(16);          // 4
print math_pow(2, 10);        // 1024
print math_abs(-42);          // 42
print math_floor(3.7);        // 3
print math_ceil(3.2);         // 4
print math_round(3.5);        // 4
print math_min(5, 3);         // 3
print math_max(5, 3);         // 5
print math_random();          // random float [0, 1)

// IO
let content = io_read_file("data.txt");
io_write_file("output.txt", "Hello!");
io_append_file("log.txt", "New entry\n");

// Filesystem
print fs_exists("myfile.txt");
print fs_is_directory("/tmp");

// Time
let start = time_now();
time_sleep(100);  // sleep 100ms
let elapsed = time_now() - start;

// Logging
log_info("Application started");
log_warn("Something unusual");
log_error("Something went wrong");

// Environment
let home = env_get("HOME");
env_set("MY_VAR", "value");

// Process / Shell
let output = process_shell("ls -la");
let exit_code = process_exec("mkdir /tmp/test");
let cwd = process_cwd();
```

**Extend blocks (add methods to built-in types):**

```yen
extend String {
    func is_empty(s) {
        return str_length(s) == 0;
    }
}

print "".is_empty();   // true
print "hi".is_empty(); // false
```

**Exercise 3.4:** Write a small file management utility:
1. Define an enum `FileType { Text, Binary, Unknown }`
2. Write a function that classifies a filename by extension (use `match` on the extension)
3. Use the IO/FS standard library to check if a file exists
4. Use string operations to extract the file extension from a filename

---

### Lesson 3.5 -- Putting It All Together

This final lesson combines everything into a complete project.

**Project: Student Grade Manager**

```yen
// ─── Data Model ─────────────────────────────────

class Student {
    let name;
    let grades;

    func init(name) {
        this.name = name;
        this.grades = [];
    }

    func add_grade(grade) {
        guard grade >= 0 else { throw "Grade cannot be negative"; }
        guard grade <= 100 else { throw "Grade cannot exceed 100"; }
        this.grades = [...this.grades, grade];
    }

    func average() {
        if (len(this.grades) == 0) { return 0; }
        let total = reduce(this.grades, 0, |acc, g| acc + g);
        return total / len(this.grades);
    }

    func highest() {
        return reduce(this.grades, 0, |best, g| g > best ? g : best);
    }

    func lowest() {
        if (len(this.grades) == 0) { return 0; }
        return reduce(this.grades, 100, |worst, g| g < worst ? g : worst);
    }

    func letter_grade() {
        let avg = this.average();
        match (avg) {
            90..=100 => return "A";
            80..90 => return "B";
            70..80 => return "C";
            60..70 => return "D";
            _ => return "F";
        }
    }

    func toString() {
        return "${this.name}: ${this.letter_grade()} (avg: ${this.average()})";
    }
}

// ─── Build Student Roster ─────────────────────

var students = [];

let alice = Student("Alice");
alice.add_grade(95);
alice.add_grade(88);
alice.add_grade(92);

let bob = Student("Bob");
bob.add_grade(72);
bob.add_grade(65);
bob.add_grade(78);

let charlie = Student("Charlie");
charlie.add_grade(100);
charlie.add_grade(97);
charlie.add_grade(99);

students = [alice, bob, charlie];

// ─── Reports ─────────────────────────────────

print "=" * 40;
print "  Student Grade Report";
print "=" * 40;
print "";

foreach(students, |s| {
    print s;
    print "  Grades: " + str(s.grades);
    print "  Highest: " + str(s.highest());
    print "  Lowest: " + str(s.lowest());
    print "";
});

// ─── Analytics ──────────────────────────────

// Find honor roll students (A or B)
let honor_roll = filter(students, |s| {
    let grade = s.letter_grade();
    return grade == "A" || grade == "B";
});

print "-" * 40;
print "Honor Roll:";
foreach(honor_roll, |s| print "  " + s.name);

// Class average
let class_avg = reduce(students, 0, |sum, s| sum + s.average()) / len(students);
print "";
print "Class Average: " + str(class_avg);

// Sort by average (highest first)
let ranked = sort_by(students, |s| -s.average());
print "";
print "Rankings:";
for [i, student] in enumerate(ranked) {
    print "  ${i + 1}. ${student}";
}

// ─── Error Handling ──────────────────────────

print "";
print "-" * 40;
print "Error handling demo:";

try {
    let test = Student("Test");
    test.add_grade(-5);
} catch (e) {
    print "Caught: " + e;
}

try {
    let test = Student("Test");
    test.add_grade(150);
} catch (e) {
    print "Caught: " + e;
}

print "";
print "=" * 40;
print "  Course complete!";
print "=" * 40;
```

**Key concepts used in this project:**
- Classes with methods and `toString()`
- Guard clauses and `throw`
- Lambdas (expression and block bodies)
- Higher-order functions: `reduce`, `filter`, `foreach`, `sort_by`
- For-in destructuring with `enumerate`
- Pattern matching with ranges
- Spread operator for list building
- String interpolation and multiplication
- Try/catch error handling
- Ternary operator

**Exercise 3.5 (Final Project):** Build a **Task Manager** application:
1. Create a `Task` class with: title, description, priority (1-5), completed status
2. Create a `TaskManager` class that stores a list of tasks
3. Implement methods: `add_task`, `complete_task`, `get_pending`, `get_by_priority`
4. Use `filter` and `sort_by` for querying tasks
5. Use `match` to label priorities ("Critical", "High", "Medium", "Low", "Trivial")
6. Use try/catch for validation (e.g., priority must be 1-5)
7. Use string interpolation and `toString()` for display
8. Print a formatted report of all tasks, grouped by status

---

## Quick Reference Card

| Feature | Syntax |
|---|---|
| Immutable variable | `let x = 10;` |
| Mutable variable | `var x = 10;` |
| String interpolation | `"Hello, ${name}!"` |
| If/else | `if (cond) { } else { }` |
| For-in loop | `for x in collection { }` |
| Range loop | `for i in 0..10 { }` |
| While loop | `while (cond) { }` |
| Do-while loop | `do { } while (cond);` |
| Function | `func name(a, b = 0) { }` |
| Lambda | `\|x\| x * 2` |
| Block lambda | `\|x\| { return x * 2; }` |
| Class | `class Foo { let x; func init(x) { this.x = x; } }` |
| Inheritance | `class Bar extends Foo { }` |
| Match | `match (val) { 1 => ...; _ => ...; }` |
| Try/catch | `try { } catch (e) { } finally { }` |
| Spread | `[...a, ...b]` or `func(...args)` |
| Destructuring | `let [a, b] = [1, 2];` |
| Pipe | `val \|> func1 \|> func2` |
| Ternary | `cond ? a : b` |
| Null coalescing | `val ?? default` |
| Optional chain | `obj?.field` |
| In operator | `x in list` |
| Membership | `"key" in map` |
| String multiply | `"-" * 20` |
| Negative index | `list[-1]` |
| Slicing | `list[1:3]`, `str[:5]` |
| Increment | `counter++` |
| Unless | `unless (cond) { }` |
| Until | `until (cond) { }` |
| Repeat | `repeat 5 { }` or `repeat 5 as i { }` |
| Guard | `guard cond else { return; }` |
| Named args | `func(name: "Alice", age: 30)` |
| Type check | `val is int`, `obj is ClassName` |
