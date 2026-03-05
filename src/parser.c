#include "parser.h"
#include "tokenizer.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

struct command* cmd_parse(struct token* tokens)
{
    // Si pas de tokens ou ligne vide, on retourne NULL
    if (!tokens || tokens->category == TOK_NEWLINE) return NULL;

    // Utilisation d'une sentinelle pour construire la liste chaînée facilement
    struct command sentinel = {NULL, NULL, OP_TERMINATOR};
    struct command *current_cmd = &sentinel;
    struct token *curr_tok = tokens;

    // On boucle tant qu'on a des tokens et qu'on n'est pas à la fin de la ligne
    while (curr_tok && curr_tok->category != TOK_NEWLINE) {

        // 1. Allouer la structure de la nouvelle commande
        struct command *new_cmd = malloc(sizeof(struct command));
        if (!new_cmd) return NULL;
        new_cmd->next = NULL;
        new_cmd->args = NULL;
        new_cmd->op = OP_TERMINATOR;

        // 2. Compter combien il y a d'arguments (mots) avant le prochain opérateur
        int arg_count = 0;
        struct token *temp = curr_tok;
        while (temp && (temp->category == TOK_SYMBOL || temp->category == TOK_STRING_LITERAL)) {
            arg_count++;
            temp = temp->next;
        }

        // 3. Allouer le tableau args (nombre d'arguments + 1 pour le NULL final)
        new_cmd->args = malloc(sizeof(char*) * (arg_count + 1));
        if (!new_cmd->args) {
            free(new_cmd);
            return NULL;
        }

        // 4. Copier les arguments dans la commande avec strdup
        for (int i = 0; i < arg_count; i++) {
            new_cmd->args[i] = strdup(curr_tok->value);
            curr_tok = curr_tok->next;
        }
        new_cmd->args[arg_count] = NULL; // Toujours finir par NULL pour execvp

        // 5. Identifier l'opérateur qui suit la commande
        if (curr_tok && curr_tok->category != TOK_NEWLINE) {
            switch (curr_tok->category) {
                case TOK_PIPE:        new_cmd->op = OP_PIPE; break;
                case TOK_SEMICOLON:   new_cmd->op = OP_SEPARATOR; break;
                case TOK_LOGICAL_AND: new_cmd->op = OP_AND; break;
                case TOK_LOGICAL_OR:  new_cmd->op = OP_OR; break;
                default:              new_cmd->op = OP_TERMINATOR; break;
            }
            // On avance au token suivant après l'opérateur pour préparer la commande suivante
            curr_tok = curr_tok->next;
        } else {
            // Pas d'opérateur trouvé, c'est la fin de la ligne
            new_cmd->op = OP_TERMINATOR;
        }

        // 6. Ajouter la commande à notre liste chaînée
        current_cmd->next = new_cmd;
        current_cmd = new_cmd;
    }

    return sentinel.next;
}

void cmd_free(struct command *command)
{
    struct command *curr = command;
    while (curr) {
        struct command *next = curr->next;

        // Libérer chaque chaîne de caractères dans le tableau args
        if (curr->args) {
            for (int i = 0; curr->args[i] != NULL; i++) {
                free(curr->args[i]);
            }
            // Libérer le tableau de pointeurs lui-même
            free(curr->args);
        }

        // Libérer la structure de la commande
        free(curr);

        // Passer à la suivante
        curr = next;
    }
}

void cmd_debug_print(const struct command* commands)
{
    for(const struct command* cmd = commands; cmd; cmd = cmd->next)
    {
        for(int i = 0; cmd->args[i]; i++)
        {
            printf("%s ", cmd->args[i]);
        }

        switch (cmd->op) {
            case OP_TERMINATOR: printf("OP_TERMINATOR"); break;
            case OP_SEPARATOR: printf("OP_SEPARATOR"); break;
            case OP_AND: printf("OP_AND"); break;
            case OP_OR: printf("OP_OR"); break;
            case OP_PIPE: printf("OP_PIPE"); break;
            default: printf("OP_INVALID"); break;
        }

        printf("\n");
    }
}

