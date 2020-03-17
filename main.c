#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Function declarations
 */
void cshell_loop(void);
char *cshell_read_line(void);
char **cshell_split_line(char *line);
int cshell_launch(char **args);
int cshell_cd(char **args);
int cshell_help(char **args);
int cshell_exit(char **args);
int cshell_execute(char **args);

int main(int argc, char **argv)
{
  // commands loop
  cshell_loop();
  // shutdown/cleanup
  return EXIT_SUCCESS;
}

/**
 * Executa um loop para ler, interpretar e executar os comandos
 */
void cshell_loop(void)
{
  char *line;
  char **args;
  int status;
  do
  {
    printf("> ");
    line = cshell_read_line();
    args = cshell_split_line(line);
    status = cshell_execute(args);
    free(line);
    free(args);
  } while (status);
}

/**
 * Lê uma linha do que o usuário digita por vez.
 */
#define CSHELL_LINE_BUFFSIZE 1024
char *cshell_read_line(void)
{
  int buffsize = CSHELL_LINE_BUFFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * buffsize);
  int c;

  if (!buffer)
  {
    fprintf(stderr, "CSHELL: erro de alocação!\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // lê um caractere por vêz
    c = getchar();

    // se o caracter for EOF ou Enter (\n), substituímos ele por \0 (Nulo)
    // Pois strings são terminadas com nulo em C.
    if (c == EOF || c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;

    // se exceder o tamanho do buffer que foi alocado, realocamos mais memória.
    if (position >= buffsize)
    {
      buffsize += CSHELL_LINE_BUFFSIZE;
      buffer = realloc(buffer, buffsize);
      if (!buffer)
      {
        fprintf(stderr, "CSHELL: erro de alocação!\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/**
 * Divide a linha de comando em argumentos
 */
#define CSHELL_TOK_BUFFSIZE 64
#define CSHELL_TOK_DELIM " \t\r\n\a"
char **cshell_split_line(char *line)
{
  int buffsize = CSHELL_TOK_BUFFSIZE, position = 0;
  char **tokens = malloc(buffsize * sizeof(char *));
  char *token;

  if (!tokens)
  {
    fprintf(stderr, "CSHELL: erro de alocação!\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, CSHELL_TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;
    if (position >= buffsize)
    {
      buffsize += CSHELL_TOK_BUFFSIZE;
      tokens = realloc(tokens, buffsize * sizeof(char *));
      if (!tokens)
      {
        fprintf(stderr, "CSHELL: erro de alocação!\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, CSHELL_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
 * Lança um novo processo à partir dos comandos recebidos
 */
int cshell_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // child process code runs here
    if (execvp(args[0], args) == -1)
    {
      perror("CSHELL");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // error forking
    perror("CSHELL");
  }
  else
  {
    // parent process
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

/**
 * Definition of built-in shell commands followed by their function declarations
 */
char *built_in_str[] = {"cd", "help", "exit"};
int (*built_in_func[])(char **) = {&cshell_cd, &cshell_help, &cshell_exit};

int cshell_num_builtins()
{
  return sizeof(built_in_str) / sizeof(char *);
}

// built-in function implementations
int cshell_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "CSHELL: Expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("CSHELL");
    }
  }
  return 1;
}

int cshell_help(char **args)
{
  int i;
  printf("CSHELL - A simple linux shell by @thau0x01\n");
  printf("Type the program names and arguments, then hit enter.\n");
  printf("The Following are built-in:\n");

  for (i = 0; i < cshell_num_builtins(); i++)
  {
    printf("  %s\n", built_in_str[i]);
  }

  printf("Use the man command for other programs.\n");
  return 1;
}

int cshell_exit(char **args)
{
  return 0;
}

/**
 * Function responsible for launching built-in functions and external programs together
 */
int cshell_execute(char **args)
{
  int i;
  if (args[0] == NULL)
  {
    // an empty command was entered
    return 1;
  }

  for (i = 0; i < cshell_num_builtins(); i++)
  {
    if (strcmp(args[0], built_in_str[i]) == 0)
    {
      return (*built_in_func[i])(args);
    }
  }
  return cshell_launch(args);
}