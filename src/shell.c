#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/wait.h>

#include "tokenizer.h"
#include "parser.h"

#define EXECUTION_FAILED (-1)
#define EXECUTION_SUCCESS 1
#define EXECUTION_REQUEST_EXIT 0

#define EXIT_COMMAND_NOT_FOUND 127

/**
 * Cette fonction crée un processus fils qui exécute une commande.
 *
 * @param cmd commande à exécuter
 * @param in_fd descripteur à utiliser comme entrée standard
 * @param out_fd descripteur à utiliser comme sortie standard
 * @param unused_fd descripteur à fermer dans le fils (bout de lecture du pipe suivant), -1 si aucun
 *
 * @return le pid du fils, ou -1 en cas d'échec
 */
static pid_t sh_spawn(const struct command *cmd, int in_fd, int out_fd, int unused_fd)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        // Enfant: brancher les pipes puis exécuter
        if (in_fd != STDIN_FILENO)
        {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (out_fd != STDOUT_FILENO)
        {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }
        if (unused_fd >= 0) close(unused_fd);

        execvp(cmd->args[0], cmd->args);

        fprintf(stderr, "%s: command not found\n", cmd->args[0]);
        _exit(EXIT_COMMAND_NOT_FOUND);
    }

    return pid;
}

/**
 * Cette fonction retourne la dernière commande du pipeline qui commence par cmd
 * (a | b | c forme un seul pipeline).
 *
 * @param cmd première commande du pipeline
 * @param count reçoit le nombre de commandes du pipeline
 *
 * @return la dernière commande du pipeline
 */
static const struct command *sh_pipeline_end(const struct command *cmd, size_t *count)
{
    *count = 1;
    while (cmd->op == OP_PIPE && cmd->next)
    {
        cmd = cmd->next;
        (*count)++;
    }
    return cmd;
}

/**
 * Cette fonction exécute un pipeline de une ou plusieurs commandes reliées par des pipes,
 * et attend la fin de toutes les commandes.
 *
 * @param cmd première commande du pipeline
 *
 * @return le code de retour de la dernière commande du pipeline
 */
static int sh_run_pipeline(const struct command *cmd)
{
    size_t count;
    sh_pipeline_end(cmd, &count);

    pid_t *pids = malloc(count * sizeof(pid_t));
    if (!pids)
    {
        fprintf(stderr, "shell: out of memory\n");
        return 1;
    }

    int in_fd = STDIN_FILENO;
    size_t spawned = 0;

    for (; spawned < count; spawned++, cmd = cmd->next)
    {
        int pipefd[2] = {-1, -1}; // pipefd[0] est le bout de lecture du pipe, pipefd[1] le bout d'écriture
        int out_fd = STDOUT_FILENO;

        // Toutes les commandes sauf la dernière écrivent dans un pipe vers la suivante
        if (spawned < count - 1)
        {
            if (pipe(pipefd) < 0)
            {
                perror("pipe");
                break;
            }
            out_fd = pipefd[1];
        }

        pids[spawned] = sh_spawn(cmd, in_fd, out_fd, pipefd[0]);

        // Le parent ferme ses copies des descripteurs déjà transmis au fils
        if (in_fd != STDIN_FILENO) close(in_fd);
        if (out_fd != STDOUT_FILENO) close(out_fd);
        in_fd = pipefd[0];
    }

    if (in_fd > STDIN_FILENO) close(in_fd); // Pipe non consommé si la boucle s'est arrêtée sur une erreur

    // Attendre tous les fils, seul le code de retour du dernier compte
    int exit_code = 1;
    for (size_t i = 0; i < spawned; i++)
    {
        if (pids[i] < 0) continue;

        int status;
        waitpid(pids[i], &status, 0);

        if (i == count - 1)
        {
            exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        }
    }

    free(pids);
    return exit_code;
}

/**
 * Cette fonction prend une liste de commandes et l'exécute.
 *
 * Les pipelines sont évalués de gauche à droite : après "&&", le pipeline suivant n'est exécuté
 * que si le précédent a réussi, après "||" seulement s'il a échoué, et après ";" dans tous les cas.
 * Un pipeline sauté conserve le code de retour précédent, comme dans bash.
 *
 * @param cmd list chainée de commandes
 *
 * @return le code de retour de la dernière commande exécutée.
 */
int sh_run(const struct command *cmd)
{
    int last_exit_code = 0;
    enum op prev_op = OP_SEPARATOR;

    while (cmd)
    {
        size_t count;
        const struct command *last = sh_pipeline_end(cmd, &count);

        int skip = (prev_op == OP_AND && last_exit_code != 0)
                || (prev_op == OP_OR && last_exit_code == 0);

        if (!skip)
        {
            if (count == 1 && strcmp(cmd->args[0], "exit") == 0) return EXECUTION_REQUEST_EXIT; // Exit command

            last_exit_code = sh_run_pipeline(cmd);
        }

        prev_op = last->op;
        cmd = last->next;
    }

    return last_exit_code == 0 ? EXECUTION_SUCCESS : EXECUTION_FAILED;
}

int main(void)
{
    for(;;)
    {
        struct token* tokens = tok_next_line();

        if(!tokens) return 0; // End of input

        if(tokens->category == TOK_NEWLINE) // Empty line
        {
            tok_free(tokens);
            continue;
        }

        struct command* commands = cmd_parse(tokens);
        tok_free(tokens); // Les commandes possèdent leurs propres copies des arguments

        if(!commands) continue; // Syntax error, already reported

        int status = sh_run(commands);
        cmd_free(commands);

        if(status == EXECUTION_REQUEST_EXIT) return 0;
    }
}
