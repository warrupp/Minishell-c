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

/**
 * Cette fonction prend une liste de commandes et l'exécute.
 *
 * @param cmd list chainée de commandes
 *
 * @return le code de retour de la dernière commande exécutée.
 */
int sh_run(const struct command *cmd)
{   
    if (!cmd || cmd->args[0] == NULL) return EXECUTION_FAILED; // Empty command
    if (strcmp(cmd->args[0], "exit") == 0) return EXECUTION_REQUEST_EXIT; // Exit command


    int pipefd[2]; // pipefd[0] est l'entrée du pipe, pipefd[1] est la sortie du pipe

    if (cmd->op == OP_PIPE) {
        pipe(pipefd);
    }

    int exit_code;
    int exit_code2;

    pid_t pid = fork();
    int status;

    pid_t pid2;
    int status2;

    if (pid == 0)
        {
        
        if (cmd->op == OP_PIPE) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
        }
            
        // Enfant: execute
        execvp(cmd->args[0], cmd->args);
        
        printf("%s: command not found\n", cmd->args[0]);
        exit(1);
        }



    if (cmd->op == OP_PIPE) 
    {   

        if (!cmd->next || cmd->next->args[0] == NULL) return EXECUTION_FAILED; // Empty command
        if (strcmp(cmd->next->args[0], "exit") == 0) return EXECUTION_REQUEST_EXIT; // Exit command

        pid2 = fork();
        
        
        if (pid2 == 0) 
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            
            cmd = cmd->next;
            
            execvp(cmd->args[0], cmd->args);
            printf("%s: command not found\n", cmd->args[0]);
            exit(1); 
        }

        close(pipefd[0]);
        close(pipefd[1]);

        waitpid(pid2, &status2, 0);
    
    }

    waitpid(pid, &status, 0);
    
    if (cmd->op == OP_TERMINATOR)
    {
        if (WIFEXITED(status))
        {
            exit_code = WEXITSTATUS(status);  
        }
        
        if (exit_code == 0)
        {
            return EXECUTION_SUCCESS;
        }
        else 
        {
            return EXECUTION_FAILED;
        }   
    }
        

    if (cmd->op == OP_SEPARATOR )
    {
        return sh_run(cmd->next);
    }

    if (cmd->op == OP_AND)
    {

        if (WIFEXITED(status))
        {
        exit_code = WEXITSTATUS(status);  
        }
        if (exit_code == 0)
        {   
            return sh_run(cmd->next);
        }
        else
        {   
            if (cmd->next->op != OP_TERMINATOR){
                return sh_run(cmd->next->next);
            }
            return EXECUTION_FAILED;

        }

    }
    


    if (cmd->op == OP_OR)
    {
        if (WIFEXITED(status))
        {
        exit_code = WEXITSTATUS(status);
        }

        if (exit_code == 0)
        {   
            if (cmd->next->op != OP_TERMINATOR){
                return sh_run(cmd->next->next);
            }
            return EXECUTION_SUCCESS;
        }
        else
        {
            return sh_run(cmd->next);
        }
        
    }

  if (cmd->op == OP_PIPE) 
  {
      if (WIFEXITED(status) && WIFEXITED(status2)) 
      {
          exit_code = WEXITSTATUS(status);
          exit_code2 = WEXITSTATUS(status2);
      }

      if (cmd->next->op == OP_TERMINATOR) 
      {
          return exit_code2 == 0 ? EXECUTION_SUCCESS : EXECUTION_FAILED;
      }

      return sh_run(cmd->next->next);
  }

    return EXECUTION_REQUEST_EXIT;
}

int main(void)
{
    for(;;)
    {
        struct token* tokens = tok_next_line();

        if(!tokens) return EXECUTION_FAILED; // Tokenizer error

        //tok_debug_print(tokens);

        struct command* commands = cmd_parse(tokens);

        if(!commands)
        {
            tok_free(tokens);
            return EXECUTION_FAILED; // Parser error
        }



        //cmd_debug_print(commands);
        //cmd_free(commands);
        //tok_free(tokens);
        //continue;


        int status = sh_run(commands);


        if(status == EXECUTION_REQUEST_EXIT)
        {
            exit(0);
        }

        cmd_free(commands);
        tok_free(tokens);
    }
}
