# Simple Unix Shell (C)

## 🇫🇷 Version française

### Description
Ce projet est une implémentation simplifiée d’un **shell Unix** développée en **C** dans le cadre d’un travail universitaire.

Le programme lit les commandes saisies par l’utilisateur, les **analyse**, puis les **exécute** en utilisant les appels système de Unix.

Ce projet permet de comprendre plusieurs concepts importants de la programmation système :

- création de processus (`fork`)
- exécution de programmes (`exec`)
- gestion des processus (`wait`)
- parsing et tokenisation des commandes

---

### Fonctionnalités

- exécution de commandes système
- gestion des **pipes** (`|`)
- opérateurs logiques (`&&`, `||`)
- séquencement de commandes (`;`)
- parsing et tokenisation des commandes

---

### Compilation

Le projet utilise **CMake**.
---
## 🇬🇧 English Version

### Description
This project is a simplified implementation of a **Unix shell written in C** developed as part of a university assignment.

The program reads commands entered by the user, **parses** them, and **executes** them using Unix system calls.

This project helps explore several key concepts of systems programming:

- process creation (`fork`)
- program execution (`exec`)
- process management (`wait`)
- command parsing and tokenization

---

### Features

- execution of system commands
- support for **pipes** (`|`)
- logical operators (`&&`, `||`)
- command sequencing (`;`)
- command parsing and tokenization

---

### Build

The project uses **CMake**.