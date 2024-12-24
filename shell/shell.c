#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct command {
  char *name;
  char **args;
} cmd;

const size_t MAX_CMDS_BUF = 128;
const size_t MAX_N_CMDS = 4;
const size_t MAX_N_ARGS = 8;

#define error(ERROR, ...)                                                      \
  fprintf(stderr, "Line %d:", __LINE__), fprintf(stderr, __VA_ARGS__),         \
      fprintf(stderr, "\n"), ERROR

int get_cmds_line(char *buf) {
  long n_read = read(0, buf, MAX_CMDS_BUF);
  if (n_read < 0)
    return error(n_read, "Read error");

  if (n_read == MAX_CMDS_BUF)
    return error(-1, "Too many cmds");

  return 0;
}

int get_cmds(cmd **cmds_ptr, char *cmds_buf, size_t *n_cmds_ptr) {
  cmd *cmds = (cmd *)malloc(MAX_N_CMDS * sizeof(cmd));
  if (!cmds)
    return error(errno, "Malloc error");

  size_t n_cmds = 0;

  char *token = strtok(cmds_buf, "|");
  while (token != NULL) {
    cmds[n_cmds].name = token;
    n_cmds++;

    if (n_cmds == MAX_N_CMDS)
      return free(cmds), error(-1, "Too many cmds");

    token = strtok(NULL, "|");
  }

  for (size_t i = 0; i < n_cmds; i++) {
    char **args = (char **)malloc(MAX_N_ARGS * sizeof(char *));
    if (!args)
      return free(cmds), error(errno, "Malloc error");

    size_t n_args = 0;
    char *cmd_token = strtok(cmds[i].name, " \n");
    cmds[i].name = cmd_token;

    while (cmd_token != NULL) {
      args[n_args] = cmd_token;
      n_args++;

      if (n_args + 1 == MAX_N_ARGS)
        return free(cmds), free(args), error(-1, "Too many args");

      cmd_token = strtok(NULL, " \n");
    }

    args[n_args] = NULL;
    cmds[i].args = args;
  }

  *cmds_ptr = cmds;
  *n_cmds_ptr = n_cmds;

  return 0;
}

void free_cmds(struct command *cmds, size_t n_cmds) {
  for (size_t i = 0; i < n_cmds; i++)
    free(cmds[i].args);

  free(cmds);
}

void print_cmds(struct command *cmds, size_t n_cmds) {
  for (size_t i = 0; i < n_cmds; i++) {
    fprintf(stderr, "Command %zu\n", i);
    fprintf(stderr, "Name %s\n", cmds[i].name);

    size_t j = 0;
    char *temp = cmds[i].args[j];
    while (temp != NULL) {
      fprintf(stderr, "%zu argument %s\n", j, temp);
      temp = cmds[i].args[++j];
    }
    fprintf(stderr, "\n");
  }
}

int main() {
  while (1) {
    char cmds_buf[MAX_CMDS_BUF] = {};
    cmd *cmds = NULL;
    size_t n_cmds = 0;

    if (get_cmds_line(cmds_buf))
      return -1;

    if (get_cmds(&cmds, cmds_buf, &n_cmds))
      return -1;

    if (n_cmds == 0) {
      free_cmds(cmds, n_cmds);
      continue;
    }

    size_t n_pipes = n_cmds - 1;
    int(*pipes)[2] = malloc(n_pipes * 2 * sizeof(int));
    if (!pipes)
      return free_cmds(cmds, n_cmds), EXIT_FAILURE;

    for (size_t i = 0; i < n_pipes; i++) {
      if (pipe(pipes[i]) == -1)
        return free(pipes), free_cmds(cmds, n_cmds), errno;
    }

    for (size_t i = 0; i < n_cmds; i++) {
      pid_t pid = fork();
      if (pid == 0) {
        if (i != 0) {
          if (dup2(pipes[i - 1][0], 0) == -1)
            return error(errno, "write dup2 fail"), errno;
        }

        if (i < n_cmds - 1) {
          if (dup2(pipes[i][1], 1) == -1)
            return error(errno, "read dup2 fail");
        }

        for (size_t j = 0; j < n_pipes; j++)
          close(pipes[j][0]), close(pipes[j][1]);

        execvp(cmds[i].name, cmds[i].args);
        return error(errno, "smth after execvp");
      }
    }

    for (size_t j = 0; j < n_pipes; j++) {
      close(pipes[j][0]);
      close(pipes[j][1]);
    }

    int status = 0;
    for (size_t i = 0; i < n_cmds; i++) {
      wait(&status);
      status = WEXITSTATUS(status);
    }

    free_cmds(cmds, n_cmds);
    free(pipes);
  }

  return 0;
}
