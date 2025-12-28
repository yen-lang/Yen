# Como Usar YEN

## ✅ Modo Recomendado: Interpretador (`yen`)

O **interpretador YEN está 100% funcional** e é a forma recomendada de executar programas YEN.

### Executando Programas

```bash
# Sintaxe básica
./yen script.yen

# Exemplos funcionais
./yen examples/hello.yen
./yen examples/fibonacci.yen
./yen examples/shell_commands.yen
./yen examples/system_automation.yen
```

### Todas as Funcionalidades Disponíveis

O interpretador suporta **TODAS** as funcionalidades da linguagem:

✅ **Sintaxe Completa**
- Variáveis (`var`, `let`)
- Funções
- Classes e Structs
- Pattern matching
- Lambda expressions
- Defer statements
- Loops e condicionais

✅ **12 Bibliotecas Padrão**
- Core (type checking, conversão)
- Math (sqrt, pow, trigonometria, random)
- String (split, join, upper, lower, replace)
- Collections (push, pop, sort, reverse)
- IO (read_file, write_file)
- FS (exists, create_dir, remove)
- Time (now, sleep)
- Crypto (xor, hash, random_bytes)
- Encoding (base64, hex)
- Log (info, warn, error)
- Env (get, set)
- **Process (exec, shell, spawn, cwd, chdir)** ⭐

### Exemplos de Uso

#### 1. Shell Commands

```bash
./yen examples/os_shell_simple.yen
```

```yen
// Executar comandos shell
var files = process_shell("ls -la");
print files;

// Diretório atual
var dir = process_cwd();
print "Working in: " + dir;

// Executar e verificar código
var result = process_exec("mkdir /tmp/test");
if (result == 0) {
    print "Success!";
}
```

#### 2. Automação do Sistema

```bash
./yen examples/system_automation.yen
```

Veja o arquivo completo para exemplos de:
- Backup automático
- Monitor de disco
- Sistema de logs
- Limpeza de arquivos temporários
- Relatórios do sistema

#### 3. Processamento de Dados

```yen
// Ler e processar arquivo
var content = io_read_file("data.txt");
var lines = str_split(content, "\n");

for line in lines {
    if (str_contains(line, "ERROR")) {
        log_error(line);
    }
}
```

#### 4. Operações Matemáticas

```yen
var result = math_sqrt(16);
print "sqrt(16) = " + result;  // 4.0

var random = math_random();
print "Random: " + random;  // 0.0 to 1.0
```

---

## ⚠️ Compilador (`yenc`) - EM DESENVOLVIMENTO

O compilador LLVM está em desenvolvimento e **não deve ser usado** no momento.

### Status Atual

❌ **NÃO FUNCIONAL** - Segmentation fault em runtime
⚠️ Type checker implementado
⚠️ Infraestrutura LLVM parcialmente completa
⚠️ Geração de código incompleta

### O Que Falta

1. Correção de bugs críticos (segfault)
2. Geração completa de LLVM IR para todas as expressões
3. Linking com bibliotecas nativas
4. Runtime para gerenciar valores dinâmicos
5. Suporte para tipos complexos (listas, strings dinâmicas)
6. Geração de código para pattern matching
7. Suporte para lambdas e closures
8. Otimizações

### Por Que Não Funciona?

O compilador LLVM é significativamente mais complexo que o interpretador porque:

1. **Tipagem Dinâmica vs Estática**: YEN usa tipagem dinâmica em runtime, mas LLVM requer tipos estáticos. É necessário um sistema de runtime boxing/unboxing.

2. **Valores Dinâmicos**: O sistema de `Value` (variant com int, double, string, list, etc.) precisa ser representado em LLVM IR com tagged unions ou vtables.

3. **Bibliotecas Nativas**: As funções C++ nativas precisam ser linkadas corretamente com o código gerado.

4. **Gerenciamento de Memória**: Strings, listas e outros tipos alocados dinamicamente precisam de um runtime de memória.

5. **Closures**: Lambdas com captures requerem geração de estruturas de closure e trampolinas.

---

## 📋 Recomendações

### Para Desenvolvimento

**Use o interpretador (`yen`)**
```bash
# Desenvolvimento iterativo
./yen meu_script.yen

# Modificar código
vim meu_script.yen

# Executar novamente
./yen meu_script.yen
```

### Para Produção (Atual)

**Use o interpretador em um script wrapper**

```bash
#!/bin/bash
# deploy.sh

# Copiar interpretador e script
cp build/yen /opt/myapp/
cp scripts/main.yen /opt/myapp/

# Executar
cd /opt/myapp
./yen main.yen
```

### Para Produção (Futuro)

Quando o compilador estiver completo:

```bash
# Compilar para executável nativo
./yenc main.yen -o myapp --opt=3

# Distribuir executável standalone
./myapp
```

---

## 🚀 Performance

### Interpretador

**Vantagens:**
- ✅ Startup instantâneo
- ✅ Iteração rápida
- ✅ Ideal para scripts e automação
- ✅ Sem etapa de compilação

**Performance:**
- Adequado para a maioria dos casos de uso
- Scripts de automação
- Processamento de arquivos
- Administração de sistemas
- Prototipagem

### Compilador (Quando Pronto)

**Vantagens Planejadas:**
- ⏱️ Performance próxima a C
- 📦 Executável standalone
- 🔧 Otimizações LLVM
- 🚀 Ideal para aplicações de longa duração

---

## 📊 Comparação

| Recurso | Interpretador | Compilador |
|---------|--------------|------------|
| Status | ✅ Funcional | ❌ Em Desenvolvimento |
| Startup | Instantâneo | N/A |
| Performance | Boa | N/A |
| Biblioteca Padrão | ✅ Completa | ❌ |
| Shell Commands | ✅ Funciona | ❌ |
| Pattern Matching | ✅ Funciona | ❌ |
| Lambdas | ✅ Funciona | ❌ |
| Deployment | Script + interpretador | N/A |

---

## 💡 Dicas

### 1. Use Shebang para Scripts Executáveis

```yen
#!/opt/Yen/build/yen
// script.yen

print "Hello from executable script!";
```

```bash
chmod +x script.yen
./script.yen
```

### 2. Organizar Projetos

```
meu_projeto/
├── src/
│   ├── main.yen
│   ├── utils.yen
│   └── config.yen
├── tests/
│   └── test_main.yen
└── run.sh          # ./yen src/main.yen
```

### 3. Debugging

Use `print` statements generosamente:

```yen
func process_data(data) {
    print "[DEBUG] Processing: " + data;

    var result = transform(data);
    print "[DEBUG] Result: " + result;

    return result;
}
```

### 4. Error Handling

```yen
// Verificar resultado de comandos
var result = process_exec("mkdir /tmp/mydir");
if (result != 0) {
    log_error("Failed to create directory!");
    return;
}

// Verificar se arquivo existe
if (!fs_exists("config.txt")) {
    log_warn("Config file not found, using defaults");
}
```

---

## 🔗 Mais Informações

- [README.md](../README.md) - Visão geral do projeto
- [SYNTAX.md](SYNTAX.md) - Sintaxe completa da linguagem
- [STDLIB.md](STDLIB.md) - Referência da biblioteca padrão
- [PROCESS_SHELL.md](PROCESS_SHELL.md) - Guia de comandos shell
- [COMPILER_STATUS.md](COMPILER_STATUS.md) - Status detalhado do compilador

---

## ❓ FAQ

**P: Posso usar YEN em produção?**
R: Sim! Use o interpretador para scripts e automação de sistemas.

**P: O interpretador é confiável?**
R: Sim, está 100% funcional com todas as funcionalidades implementadas e testadas.

**P: Quando o compilador ficará pronto?**
R: O compilador é um trabalho em progresso. Acompanhe o repositório GitHub para atualizações.

**P: O interpretador é lento?**
R: Para scripts e automação de sistemas, a performance é excelente. Para computação intensiva, aguarde o compilador.

**P: Posso contribuir para o compilador?**
R: Sim! Contribuições são bem-vindas. Veja [CONTRIBUTING.md](../CONTRIBUTING.md).

---

## ✅ Conclusão

**Use o interpretador YEN (`yen`) agora!**

Ele está pronto para uso em:
- ✅ Scripts de automação
- ✅ Administração de sistemas
- ✅ Processamento de arquivos
- ✅ Integração com shell
- ✅ Prototipagem rápida
- ✅ DevOps e CI/CD

O compilador será uma adição futura para casos de uso que exigem máxima performance, mas o interpretador já é uma ferramenta poderosa e completa!

**Happy coding with YEN! 🚀**
