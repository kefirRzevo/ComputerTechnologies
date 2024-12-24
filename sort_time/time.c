#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct timeval t1, t2, dt;

  gettimeofday(&t1, NULL);

  int pid = fork();
  if (pid == 0) {
    memmove(argv, argv + 1, sizeof(char *) * argc);
    argv[argc] = NULL;

    execvp(*argv, argv);
    return 0;
  }
  wait(NULL);

  gettimeofday(&t2, NULL);

  dt.tv_sec = t2.tv_sec - t1.tv_sec;
  dt.tv_usec = t2.tv_usec - t1.tv_usec;

  printf("Time: %lf\n", dt.tv_sec + 0.000001 * dt.tv_usec);
  return 0;
}
