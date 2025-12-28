# Process & Shell Library – YEN

Library for executing shell commands and managing processes.

---

## Available Functions

### process_shell(command)

Executes a shell command and returns its output as a string.

**Syntax:**

```yen
var output = process_shell(command: string) -> string
```

**Parameters:**

* `command` – Shell command to be executed

**Returns:**

* String containing the command output
* Empty string if an error occurs

**Examples:**

```yen
// List files
var files = process_shell("ls -la");
print files;

// Show current directory
var pwd = process_shell("pwd");
print "Directory: " + pwd;

// System information
var info = process_shell("uname -a");
print info;

// Using pipes
var count = process_shell("ls | wc -l");
print "Total files: " + count;

// Search in files
var grep = process_shell("grep 'func' *.yen");
print grep;
```

---

### process_exec(command)

Executes a shell command and returns the exit code.

**Syntax:**

```yen
var exit_code = process_exec(command: string) -> int
```

**Parameters:**

* `command` – Shell command to be executed

**Returns:**

* `0` if the command executed successfully
* Non-zero error code if it failed

**Examples:**

```yen
// Create directory
var result = process_exec("mkdir -p /tmp/mydir");
if (result == 0) {
    print "Directory created!";
} else {
    print "Error creating directory";
}

// Copy files
var copy = process_exec("cp file.txt /tmp/");
print "Exit code: " + copy;

// Delete files
process_exec("rm -rf /tmp/old_files");

// Execute script
process_exec("chmod +x script.sh");
process_exec("./script.sh");
```

---

### process_spawn(command, arg1, arg2, ...)

Executes a command with arguments (variadic function).

**Syntax:**

```yen
var exit_code = process_spawn(command: string, ...args) -> int
```

**Parameters:**

* `command` – Base command
* `args` – Variable arguments for the command

**Returns:**

* Exit code of the command

**Examples:**

```yen
// Execute with arguments
process_spawn("ls", "-la", "/tmp");

// Git commands
process_spawn("git", "add", ".");
process_spawn("git", "commit", "-m", "Update");

// Compile code
process_spawn("gcc", "-o", "program", "main.c");
```

---

### process_cwd()

Returns the current working directory.

**Syntax:**

```yen
var current_dir = process_cwd() -> string
```

**Parameters:**

* None

**Returns:**

* String with the absolute path of the current directory

**Examples:**

```yen
// Show current directory
var dir = process_cwd();
print "Working in: " + dir;

// Save original directory
var original_dir = process_cwd();

// Perform operations...

// Return to original directory
process_chdir(original_dir);
```

---

### process_chdir(path)

Changes the current working directory.

**Syntax:**

```yen
var result = process_chdir(path: string) -> int
```

**Parameters:**

* `path` – New directory path

**Returns:**

* `0` if changed successfully
* `-1` if it failed

**Examples:**

```yen
// Change to /tmp
var result = process_chdir("/tmp");
if (result == 0) {
    print "Changed to /tmp";
}

// Change to home
process_chdir("/home/user");

// Use relative path
process_chdir("../parent");

// Navigate and return
var original = process_cwd();
process_chdir("/tmp");
// ... do something ...
process_chdir(original);
```

---

## Common Use Cases

### 1. File Backup

```yen
func backup(source, dest) {
    // Create backup directory
    process_exec("mkdir -p " + dest);

    // Copy files
    var result = process_exec("cp -r " + source + " " + dest);

    if (result == 0) {
        print "Backup completed!";
    } else {
        print "Backup error";
    }
}

backup("/home/user/docs", "/backup/docs");
```

---

### 2. System Monitor

```yen
func system_monitor() {
    print "=== SYSTEM MONITOR ===";

    // CPU and memory
    var top = process_shell("top -l 1 | head -10");
    print top;

    // Disk space
    var disk = process_shell("df -h");
    print disk;

    // Active processes
    var ps = process_shell("ps aux | head -20");
    print ps;
}

system_monitor();
```

---

### 3. Deployment Automation

```yen
func deploy() {
    print "Starting deployment...";

    // Build
    var build = process_exec("make build");
    if (build != 0) {
        print "Build failed!";
        return;
    }

    // Tests
    var test = process_exec("make test");
    if (test != 0) {
        print "Tests failed!";
        return;
    }

    // Deploy
    process_exec("rsync -avz build/ user@server:/app/");
    print "Deployment completed!";
}

deploy();
```

---

### 4. Log Processing

```yen
func analyze_logs(log_file) {
    // Count errors
    var errors = process_shell("grep ERROR " + log_file + " | wc -l");
    print "Errors: " + errors;

    // Count warnings
    var warnings = process_shell("grep WARN " + log_file + " | wc -l");
    print "Warnings: " + warnings;

    // Last errors
    var last_errors = process_shell("grep ERROR " + log_file + " | tail -5");
    print "Last errors:";
    print last_errors;
}

analyze_logs("/var/log/app.log");
```

---

### 5. Git Automation

```yen
func git_status() {
    var status = process_shell("git status");
    print status;

    var branch = process_shell("git branch --show-current");
    print "Current branch: " + branch;
}

func git_commit(message) {
    process_exec("git add .");
    var result = process_exec("git commit -m '" + message + "'");

    if (result == 0) {
        print "Commit successful!";
    } else {
        print "Nothing to commit";
    }
}

git_status();
git_commit("Update files");
```

---

### 6. Download and Extract

```yen
func download_and_extract(url, dest) {
    print "Downloading " + url;

    // Download
    var dl = process_exec("curl -L -o /tmp/file.tar.gz " + url);
    if (dl != 0) {
        print "Download error";
        return;
    }

    // Extract
    process_exec("mkdir -p " + dest);
    process_exec("tar -xzf /tmp/file.tar.gz -C " + dest);

    // Cleanup
    process_exec("rm /tmp/file.tar.gz");

    print "Extracted to: " + dest;
}

download_and_extract("https://example.com/file.tar.gz", "/opt/app");
```

---

### 7. Database Backup

```yen
func backup_database(db_name, backup_dir) {
    var timestamp = process_shell("date +%Y%m%d_%H%M%S");
    var filename = backup_dir + "/" + db_name + "_" + timestamp + ".sql";

    print "Backing up " + db_name;

    var dump = process_exec("mysqldump " + db_name + " > " + filename);

    if (dump == 0) {
        // Compress
        process_exec("gzip " + filename);
        print "Backup saved: " + filename + ".gz";
    } else {
        print "Backup error!";
    }
}

backup_database("myapp", "/backups");
```

---

### 8. Network Diagnostics

```yen
func network_check(host) {
    print "Checking connection to " + host;

    // Ping
    var ping = process_shell("ping -c 4 " + host);
    print ping;

    // DNS lookup
    var dns = process_shell("nslookup " + host);
    print dns;

    // Traceroute
    var trace = process_shell("traceroute -m 10 " + host);
    print trace;
}

network_check("google.com");
```

---

## Security Notes

### ⚠️ Command Injection

Always sanitize user input before passing it to shell commands:

```yen
// UNSAFE!
var user_input = get_user_input();
process_shell("rm -rf " + user_input);  // Dangerous!

// SAFE
func sanitize(input) {
    // Remove dangerous characters
    // Validate input
    return input;
}

var safe_input = sanitize(user_input);
process_shell("rm -rf " + safe_input);
```

---

### Best Practices

1. **Validate inputs** before using them in commands
2. **Prefer process_spawn** when possible (safer than process_shell)
3. **Check exit codes** from process_exec
4. **Handle errors** properly
5. **Clean up temporary files** after use
6. **Use absolute paths** when possible

---

## Supported Platforms

* Linux – Full POSIX support
* macOS – BSD/POSIX commands
* Windows – Requires command adaptation (use PowerShell or WSL)

### Windows Compatibility

```yen
// Detect platform
var os = process_shell("uname");

if (str_contains(os, "MINGW") || str_contains(os, "MSYS")) {
    // Windows
    var files = process_shell("dir");
} else {
    // Unix-like
    var files = process_shell("ls");
}
```

---

## Performance Notes

* `process_shell` uses `popen()` – pipe overhead
* `process_exec` uses `system()` – faster execution
* For intensive operations, consider native libraries

* Generate **API reference pages**
* Normalize terminology to match Rust/Python-style docs
