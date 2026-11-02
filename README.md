# minishell-c — A Minimal Unix Shell in C

![Language](https://img.shields.io/badge/language-C17-blue)
![Build](https://img.shields.io/badge/build-CMake-informational)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey)

🇬🇧 English · [🇫🇷 Version française](#-version-française)

A lightweight command interpreter written from scratch in C. It reads command lines from standard input, turns them into tokens, builds a list of commands, and runs them with POSIX system calls (`fork`, `execvp`, `pipe`, `dup2`, `waitpid`).

It covers the full path from raw text to running processes, with no external libraries.

> **Academic context:** this shell was built as a project for a university systems programming course. The goal was to build a working command interpreter from scratch using the POSIX process API. After the course, I revisited the project: I fixed the operator-chaining bugs, added pipelines of any length, and made the shell report syntax errors instead of running malformed lines.

---

## Features

| Feature | Syntax | Example |
|---|---|---|
| Run programs found in `PATH` | `cmd args…` | `ls -la /tmp` |
| Pipelines of any length | `a \| b \| c` | `ls \| sort \| uniq \| wc -l` |
| Run commands in sequence | `a ; b` | `mkdir out ; ls` |
| Run the next command only on success | `a && b` | `make && ./app` |
| Run the next command only on failure | `a \|\| b` | `test -f cfg \|\| echo "missing"` |
| Quoted arguments (`"…"` or `'…'`) | `"…"` | `echo "hello world"` |
| Escape sequences inside quotes | `\n \t \\ \" …` | `printf "a\tb\n"` |
| Built-in exit command | `exit` | `exit` |
| Syntax error reporting | — | `ls &&` reports `syntax error near unexpected token 'newline'` |

Operators are evaluated from left to right, as in `bash`. When `&&` or `||` skips a pipeline, the previous exit status is kept, so `false && a || b` runs `b`.


---

## Architecture

The shell runs in three stages, each in its own module:

```
stdin ──► Tokenizer ──► Parser ──► Executor ──► child processes
          tokenizer.c   parser.c   shell.c
          (tokens)      (commands) (fork / execvp / pipe / waitpid)
```

| Module | Role |
|---|---|
| [`src/tokenizer.c`](src/tokenizer.c) | A hand-written lexer that reads stdin one character at a time. It sorts the input into words, quoted strings (with escape sequences), and the operators `;`, `\|`, `&&`, `\|\|`. Buffers grow as needed (×1.5). On a lexical error it reports the problem and drops the rest of the line, so a malformed line never runs in part. It outputs a linked list of tokens. |
| [`src/parser.c`](src/parser.c) | Groups the tokens into `struct command` nodes. Each node holds an `argv` array ending in `NULL`, ready for `execvp`, plus the operator that links it to the next command. It rejects malformed lines, such as `\| ls`, `ls &&` or `a && \|\| b`, and frees everything it allocated when it hits an error. |
| [`src/shell.c`](src/shell.c) | Runs the list of commands. It groups them into pipelines and forks one child per command. It connects the children with `pipe`/`dup2`, waits for all of them with `waitpid`, and keeps the exit status of the last one. It then applies the `&&`, `\|\|` and `;` rules to decide which pipeline runs next. The main loop reads, parses, runs and frees one line at a time. |

### Key data structures

```c
struct token {                 // output of the tokenizer
    struct token *next;
    char *value;               // NULL for operators
    enum token_category category;
};

struct command {               // output of the parser
    struct command *next;
    char **args;               // NULL-terminated argv
    enum op op;                // ;  &&  ||  |  or end of line
};
```

---

## Build & Run

**Requirements:** a POSIX system (Linux, macOS, or WSL / MSYS2 on Windows), CMake ≥ 3.20, and a C17 compiler (GCC or Clang).

```bash
# Configure and build. The project refuses in-source builds.
cmake -S . -B build
cmake --build build

# Run interactively. There is no prompt: type a command and press Enter.
./build/src/shell

# Or pipe a script into it
printf 'echo hello && ls | wc -l\nexit\n' | ./build/src/shell
```

---

## Concepts demonstrated

- **Process management:** `fork`, `execvp`, `waitpid`, and reading exit codes
- **Inter-process communication:** anonymous pipes, and redirecting file descriptors with `dup2`
- **Lexical analysis:** a hand-written tokenizer that reads one character ahead (`ungetc`)
- **Parsing:** building a linked list of commands, using a sentinel node
- **Manual memory management:** growing buffers, and freeing every token and command
- **Short-circuit logic:** running or skipping commands based on exit codes

---

## Limitations & possible improvements

This is a teaching project, not a full shell. These features are out of scope and would be natural next steps:

- No built-in `cd`, no environment variables, and no `$VAR` expansion.
- No I/O redirection (`<`, `>`, `>>`) and no background jobs (`&`). A lone `&` is reported as unsupported.
- No interactive prompt and no line editing or history.
- `exit` is only recognised as a standalone command. It does not take an exit code and has no effect inside a pipeline.

---

## Project structure

```
.
├── CMakeLists.txt          # Top-level build configuration
├── cmake/
│   ├── dependencies.cmake  # Threads dependency
│   └── testing.cmake       # CTest setup
└── src/
    ├── CMakeLists.txt      # `shell` executable target
    ├── shell.c             # Main loop and executor
    ├── tokenizer.{c,h}     # Lexer
    └── parser.{c,h}        # Parser
```

---

## 🇫🇷 Version française

Un interpréteur de commandes léger, écrit de zéro en C. Il lit les lignes de commande sur l'entrée standard, les découpe en tokens, construit une liste de commandes et les exécute avec les appels système POSIX (`fork`, `execvp`, `pipe`, `dup2`, `waitpid`).

Il couvre tout le chemin du texte brut jusqu'aux processus en cours d'exécution, sans bibliothèque externe.

> **Contexte académique :** ce shell a été réalisé dans le cadre d'un projet pour un cours universitaire de programmation système. L'objectif était de construire un interpréteur de commandes fonctionnel à partir de zéro, avec l'API POSIX de gestion des processus. Après le cours, j'ai repris le projet : j'ai corrigé les bugs d'enchaînement des opérateurs, ajouté les pipelines de longueur quelconque, et fait en sorte que le shell signale les erreurs de syntaxe au lieu d'exécuter des lignes mal formées.

### Fonctionnalités

- Exécution des programmes présents dans le `PATH`
- Pipelines de longueur quelconque : `ls | sort | uniq | wc -l`
- Exécution en séquence : `cmd1 ; cmd2`
- Opérateurs logiques à court-circuit, évalués de gauche à droite comme dans `bash` : `&&` (si succès) et `||` (si échec)
- Arguments entre guillemets (`"…"` ou `'…'`) avec séquences d'échappement (`\n`, `\t`, `\\`, `\"`…)
- Commande interne `exit`
- Signalement des erreurs de syntaxe (`| ls`, `ls &&`, guillemet non fermé…) sans exécuter la ligne fautive

### Architecture

Le shell fonctionne en trois étapes : **Tokenizer** ([`tokenizer.c`](src/tokenizer.c)) → **Parser** ([`parser.c`](src/parser.c)) → **Exécuteur** ([`shell.c`](src/shell.c)).

- **Tokenizer :** analyseur lexical écrit à la main. Il lit stdin caractère par caractère, avec des buffers qui s'agrandissent au besoin.
- **Parser :** regroupe les tokens en une liste chaînée de `struct command`. Chaque commande contient un tableau `argv` terminé par `NULL`, prêt pour `execvp`.
- **Exécuteur :** regroupe les commandes en pipelines et crée un processus fils par commande (`fork`). Il relie les fils avec `pipe`/`dup2`, attend leur fin avec `waitpid`, puis applique les règles de `;`, `&&` et `||`.

### Compilation et exécution

Prérequis : un système POSIX (Linux, macOS, ou WSL / MSYS2 sous Windows), CMake ≥ 3.20 et un compilateur C17.

```bash
cmake -S . -B build
cmake --build build
./build/src/shell
```

### Notions mises en œuvre

Gestion des processus (`fork`, `exec`, `wait`), communication inter-processus avec les pipes, redirection de descripteurs de fichiers, analyse lexicale et syntaxique, gestion manuelle de la mémoire, et logique à court-circuit basée sur les codes de retour.

### Limites et améliorations possibles

- Pas de `cd` intégré, pas de variables d'environnement, pas de redirections (`<`, `>`, `>>`), pas de tâches en arrière-plan (`&`).
- Pas de prompt interactif ni d'historique.
- `exit` n'accepte pas de code de retour.
