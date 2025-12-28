# Process & Shell Library - YEN

Biblioteca para execução de comandos shell e gerenciamento de processos.

## Funções Disponíveis

### process_shell(command)

Executa um comando shell e retorna a saída como string.

**Sintaxe:**
```yen
var output = process_shell(command: string) -> string
```

**Parâmetros:**
- `command` - Comando shell a ser executado

**Retorna:**
- String contendo a saída do comando
- String vazia se houver erro

**Exemplos:**
```yen
// Listar arquivos
var files = process_shell("ls -la");
print files;

// Ver diretório atual
var pwd = process_shell("pwd");
print "Diretório: " + pwd;

// Informações do sistema
var info = process_shell("uname -a");
print info;

// Usar pipes
var count = process_shell("ls | wc -l");
print "Total de arquivos: " + count;

// Buscar em arquivos
var grep = process_shell("grep 'func' *.yen");
print grep;
```

---

### process_exec(command)

Executa um comando shell e retorna o código de saída.

**Sintaxe:**
```yen
var exit_code = process_exec(command: string) -> int
```

**Parâmetros:**
- `command` - Comando shell a ser executado

**Retorna:**
- `0` se o comando executou com sucesso
- Código de erro diferente de zero se falhou

**Exemplos:**
```yen
// Criar diretório
var result = process_exec("mkdir -p /tmp/mydir");
if (result == 0) {
    print "Diretório criado!";
} else {
    print "Erro ao criar diretório";
}

// Copiar arquivos
var copy = process_exec("cp file.txt /tmp/");
print "Código de saída: " + copy;

// Deletar arquivos
process_exec("rm -rf /tmp/old_files");

// Executar script
process_exec("chmod +x script.sh");
process_exec("./script.sh");
```

---

### process_spawn(command, arg1, arg2, ...)

Executa um comando com argumentos (variadic function).

**Sintaxe:**
```yen
var exit_code = process_spawn(command: string, ...args) -> int
```

**Parâmetros:**
- `command` - Comando base
- `args` - Argumentos variáveis para o comando

**Retorna:**
- Código de saída do comando

**Exemplos:**
```yen
// Executar com argumentos
process_spawn("ls", "-la", "/tmp");

// Git commands
process_spawn("git", "add", ".");
process_spawn("git", "commit", "-m", "Update");

// Compilar código
process_spawn("gcc", "-o", "program", "main.c");
```

---

### process_cwd()

Retorna o diretório de trabalho atual.

**Sintaxe:**
```yen
var current_dir = process_cwd() -> string
```

**Parâmetros:**
- Nenhum

**Retorna:**
- String com o caminho absoluto do diretório atual

**Exemplos:**
```yen
// Ver diretório atual
var dir = process_cwd();
print "Trabalhando em: " + dir;

// Salvar diretório original
var original_dir = process_cwd();

// Fazer alguma operação...

// Voltar ao diretório original
process_chdir(original_dir);
```

---

### process_chdir(path)

Muda o diretório de trabalho atual.

**Sintaxe:**
```yen
var result = process_chdir(path: string) -> int
```

**Parâmetros:**
- `path` - Caminho do novo diretório

**Retorna:**
- `0` se mudou com sucesso
- `-1` se falhou

**Exemplos:**
```yen
// Mudar para /tmp
var result = process_chdir("/tmp");
if (result == 0) {
    print "Mudou para /tmp";
}

// Mudar para home
process_chdir("/home/user");

// Usar caminho relativo
process_chdir("../parent");

// Navegar e voltar
var original = process_cwd();
process_chdir("/tmp");
// ... fazer algo ...
process_chdir(original);
```

---

## Casos de Uso Comuns

### 1. Backup de Arquivos

```yen
func backup(source, dest) {
    // Criar diretório de backup
    process_exec("mkdir -p " + dest);

    // Copiar arquivos
    var result = process_exec("cp -r " + source + " " + dest);

    if (result == 0) {
        print "Backup concluído!";
    } else {
        print "Erro no backup";
    }
}

backup("/home/user/docs", "/backup/docs");
```

### 2. Monitor de Sistema

```yen
func system_monitor() {
    print "=== MONITOR DO SISTEMA ===";

    // CPU e memória
    var top = process_shell("top -l 1 | head -10");
    print top;

    // Espaço em disco
    var disk = process_shell("df -h");
    print disk;

    // Processos ativos
    var ps = process_shell("ps aux | head -20");
    print ps;
}

system_monitor();
```

### 3. Automação de Deploy

```yen
func deploy() {
    print "Iniciando deploy...";

    // Build
    var build = process_exec("make build");
    if (build != 0) {
        print "Erro no build!";
        return;
    }

    // Testes
    var test = process_exec("make test");
    if (test != 0) {
        print "Testes falharam!";
        return;
    }

    // Deploy
    process_exec("rsync -avz build/ user@server:/app/");
    print "Deploy concluído!";
}

deploy();
```

### 4. Processamento de Logs

```yen
func analyze_logs(log_file) {
    // Contar erros
    var errors = process_shell("grep ERROR " + log_file + " | wc -l");
    print "Erros: " + errors;

    // Contar warnings
    var warnings = process_shell("grep WARN " + log_file + " | wc -l");
    print "Warnings: " + warnings;

    // Últimos erros
    var last_errors = process_shell("grep ERROR " + log_file + " | tail -5");
    print "Últimos erros:";
    print last_errors;
}

analyze_logs("/var/log/app.log");
```

### 5. Git Automation

```yen
func git_status() {
    var status = process_shell("git status");
    print status;

    var branch = process_shell("git branch --show-current");
    print "Branch atual: " + branch;
}

func git_commit(message) {
    process_exec("git add .");
    var result = process_exec("git commit -m '" + message + "'");

    if (result == 0) {
        print "Commit realizado!";
    } else {
        print "Nada para commitar";
    }
}

git_status();
git_commit("Update files");
```

### 6. Download e Extract

```yen
func download_and_extract(url, dest) {
    print "Downloading " + url;

    // Download
    var dl = process_exec("curl -L -o /tmp/file.tar.gz " + url);
    if (dl != 0) {
        print "Erro no download";
        return;
    }

    // Extract
    process_exec("mkdir -p " + dest);
    process_exec("tar -xzf /tmp/file.tar.gz -C " + dest);

    // Cleanup
    process_exec("rm /tmp/file.tar.gz");

    print "Extraído para: " + dest;
}

download_and_extract("https://example.com/file.tar.gz", "/opt/app");
```

### 7. Database Backup

```yen
func backup_database(db_name, backup_dir) {
    var timestamp = process_shell("date +%Y%m%d_%H%M%S");
    var filename = backup_dir + "/" + db_name + "_" + timestamp + ".sql";

    print "Fazendo backup de " + db_name;

    var dump = process_exec("mysqldump " + db_name + " > " + filename);

    if (dump == 0) {
        // Comprimir
        process_exec("gzip " + filename);
        print "Backup salvo: " + filename + ".gz";
    } else {
        print "Erro no backup!";
    }
}

backup_database("myapp", "/backups");
```

### 8. Network Diagnostics

```yen
func network_check(host) {
    print "Verificando conexão com " + host;

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

## Notas de Segurança

### ⚠️ Command Injection

Sempre sanitize user input antes de passar para comandos shell:

```yen
// INSEGURO!
var user_input = get_user_input();
process_shell("rm -rf " + user_input);  // Perigoso!

// SEGURO
func sanitize(input) {
    // Remover caracteres perigosos
    // Validar input
    return input;
}

var safe_input = sanitize(user_input);
process_shell("rm -rf " + safe_input);
```

### Best Practices

1. **Validar inputs** antes de usar em comandos
2. **Usar process_spawn** quando possível (mais seguro que process_shell)
3. **Verificar códigos de saída** de process_exec
4. **Tratar erros** adequadamente
5. **Limpar arquivos temporários** após uso
6. **Usar caminhos absolutos** quando possível

---

## Plataformas Suportadas

- ✅ **Linux** - Todos os comandos POSIX
- ✅ **macOS** - Comandos BSD/POSIX
- ⚠️ **Windows** - Requer adaptação de comandos (use PowerShell ou WSL)

### Compatibilidade Windows

```yen
// Detectar plataforma
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

## Performance

- `process_shell` usa `popen()` - overhead de pipe
- `process_exec` usa `system()` - mais rápido
- Para operações intensivas, considere usar bibliotecas nativas

## Veja Também

- [IO Library](STDLIB.md#io-library) - File I/O
- [FS Library](STDLIB.md#fs-library) - Filesystem operations
- [ENV Library](STDLIB.md#env-library) - Environment variables
