/***************************************************************************/ /**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_export(char **args);
int lsh_echo(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "export",
    "echo"};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit,
    &lsh_export,
    &lsh_echo};

int lsh_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}

/************************************************************
Auxiliary functions
*************************************************************/
/*
* Retorna si el caracter c es un número de acuerdo a su código ascii
*/
bool isNumber(char c)
{
  int c_ascii = (int)c;
  return c_ascii > 47 && c_ascii < 58;
}
/*
* Retorna si el caracter c es una letra mayúscula o minúscula de acuerdo a su código ascii
*/
bool isLetter(char c)
{
  int c_ascii = (int)c;
  return (c_ascii > 64 && c_ascii < 91) || (c_ascii > 96 && c_ascii < 123);
}

/************************************************************
  Builtin function implementations.
*************************************************************/

// Máximo número de variables a guardar
#define MAX_NUM_OF_VARIABLES 5
// Máximo tamaño del nombre de la variable
#define MAX_LENGTH_OF_VARIABLE_NAME 10
// Máximo tamaño del valor de la variable
#define MAX_LENGTH_OF_VARIABLE_VALUE 15
// Número de variables creadas
int num_of_variables = 0;
// Listas de nombres y valores de variables
char variables_names[MAX_NUM_OF_VARIABLES][MAX_LENGTH_OF_VARIABLE_NAME + 1];
char variables_values[MAX_NUM_OF_VARIABLES][MAX_LENGTH_OF_VARIABLE_VALUE + 1];


/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_echo(char **args)
{
  if (args[1] == NULL)
    return 1;

  int ind_arg = 1;
  while (args[ind_arg] != NULL)
  {
    if (args[ind_arg][0] == '$')
    {
      for (int i = 0; i < num_of_variables; i++)
      {
        int j = 0, k = 1;
        while (variables_names[i][j] != '\0' && k < strlen(args[ind_arg]))
        {
          if (variables_names[i][j] == args[ind_arg][k])
          {
            j++;
            k++;
          }
          else break;
        }

        if (k == strlen(args[ind_arg]))
        {
          fprintf(stderr, "%s", variables_values[i]);
          break;
        }
      }
      fprintf(stderr, " ");
      ind_arg++;
    }
    else
      fprintf(stderr, "%s ", args[ind_arg++]);
  }

  fprintf(stderr, "\n");
  return 1;
}

/**
   @brief Bultin command: export variable.
   @param args List of args.  args[0] is "export".
   @return Always returns 1, to continue executing.
 */
int lsh_export(char **args)
{
  //Si solo se recibe el comando export, muestra todas las variables almacenadas
  if (args[1] == NULL)
  {
    for (int i = 0; i < num_of_variables; i++)
      printf("declare %s = \"%s\"\n", variables_names[i], variables_values[i]);
    return 1;
  }

  //Si el primer caracter del nombre de la variable no es una letra, se descarta
  if (!isLetter(args[1][0]))
  {
    fprintf(stderr, "lsh: export: first letter of variable name must be a letter\n");
    return 1;
  }
  int ind_arg = 1;
  int ind_tmp = 0;
  int size_variable_name = 0;
  char variable_name[MAX_LENGTH_OF_VARIABLE_NAME + 1];

  //Recorro el primer argumento hasta encontrar '=' o alcanzar el límite de tamaño del nombre de variable
  for (int i = 0; i < MAX_LENGTH_OF_VARIABLE_NAME; i++)
  {
    if (args[ind_arg][i] == '=' || ind_tmp == strlen(args[1]))
      break;
    variable_name[i] = args[ind_arg][i];
    ind_tmp++;
  }

  //i>0
  size_variable_name = ind_tmp;
  if (ind_tmp < strlen(args[ind_arg]))
  {
    //Si aun hay caracteres en el argumento, debe seguir '=', sino excede el tamaño
    if (args[ind_arg][ind_tmp++] != '=')
    {
      fprintf(stderr, "lsh: export: variable name exceeds a limit of %d characters\n", MAX_LENGTH_OF_VARIABLE_NAME);
      return 1;
    }
    //Si llega al final del arg, pasa al siguiente arg
    if (ind_tmp == strlen(args[ind_arg]))
    {
      ind_arg++;
      ind_tmp = 0;
      if (args[ind_arg] == NULL)
      {
        fprintf(stderr, "lsh: export: expected more arguments\n");
        return 1;
      }
    }
  }
  else //==
  {
    ind_arg++;
    ind_tmp = 0;
    // Pasa al siguiente arg
    if (args[ind_arg] == NULL)
    {
      fprintf(stderr, "lsh: export: expected more arguments\n");
      return 1;
    }
    // Si el sgte arg es '=', pasa al siguiente
    if (strlen(args[ind_arg]) == 1 && args[ind_arg][0] == '=')
    {
      ind_arg++;
      if (args[ind_arg] == NULL)
      {
        fprintf(stderr, "lsh: export: expected more arguments\n");
        return 1;
      }
    }
    else if (args[ind_arg][0] == '=')
    {
      ind_tmp++;
    }
  }
  variable_name[size_variable_name] = '\0';

  //Todo lo que viene después, será parte del valor
  char variable_value[MAX_LENGTH_OF_VARIABLE_VALUE + 1];
  int ind_tmp_value = 0;
  while (args[ind_arg] != NULL)
  {
    //Si se termina de leer el argumento, se agrega un espacio
    if (ind_arg > 1 && ind_tmp_value > 0)
      variable_value[ind_tmp_value++] = ' ';
    for (; ind_tmp < strlen(args[ind_arg]); ind_tmp++)
    {
      variable_value[ind_tmp_value++] = args[ind_arg][ind_tmp];
    }
    ind_arg++;
    ind_tmp = 0;
  }
  variable_value[ind_tmp_value] = '\0';
  //Si la variable tieme más caracteres que el máxmio, da error
  if (ind_tmp_value > MAX_LENGTH_OF_VARIABLE_VALUE)
  {
    fprintf(stderr, "lsh: export: variable value exceeds a limit of %d characters\n", MAX_LENGTH_OF_VARIABLE_VALUE);
    return 1;
  }
  //Si variable_name existe, reemplaza su valor, sino lo agrega
  bool added = 0;
  for (int i = 0; i < num_of_variables; i++)
  {
    if (strcmp(variables_names[i], variable_name) == 0)
    {
      strcpy(variables_values[i], variable_value);
      added = 1;
      break;
    }
  }
  if (added == 0)
  {
    if (num_of_variables == MAX_NUM_OF_VARIABLES)
      fprintf(stderr, "lsh: export: maximum number of variables reached\n");
    else
    {
      strcpy(variables_names[num_of_variables], variable_name);
      strcpy(variables_values[num_of_variables++], variable_value);
    }
  }
  return 1;
}

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("lsh");
    }
  }
  return 1;
}

/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("lsh");
  }
  else
  {
    // Parent process
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL)
  {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1)
  {
    if (feof(stdin))
    {
      exit(EXIT_SUCCESS); // We received an EOF
    }
    else
    {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer)
  {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Read a character
    c = getchar();

    if (c == EOF)
    {
      exit(EXIT_SUCCESS);
    }
    else if (c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize)
    {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token, **tokens_backup;

  if (!tokens)
  {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= bufsize)
    {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens)
      {
        free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do
  {
    char s[100];
    printf("> %s$ ", getcwd(s, 100));

    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
