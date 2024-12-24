#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t value;

enum SIGS {
  ZERO = SIGUSR1,
  ONE = SIGUSR2,
};

int is_error(const char *msg) {
  fprintf(stderr, "%s", msg);
  return -1;
}

void reader_handler(int signal) { return; }

int reader(pid_t writer_pid, int fd_in) {
  sigset_t sig_set;
  sigemptyset(&sig_set);

  struct sigaction sig_act;
  sig_act.sa_mask = 0;
  sig_act.__sigaction_u.__sa_handler = reader_handler;
  sigemptyset(&sig_act.sa_mask);

  if (sigaction(ZERO, &sig_act, NULL) == -1)
    return is_error("Bad sigaction\n");

  if (sigaction(ONE, &sig_act, NULL) == -1)
    return is_error("Bad sigaction\n");

  while (1) {
    char byte = 0;
    if (read(fd_in, &byte, sizeof(byte)) <= 0)
      return is_error("Bad read\n");

    for (size_t j = 0; j < 8; j++) {
      if ((byte >> j) & 1) {
        kill(writer_pid, ONE);
      } else {
        kill(writer_pid, ZERO);
      }
      sigsuspend(&sig_set);
    }
  }

  return 0;
}

void writer_handler(int signal) {
  if (signal == ONE) {
    value = 1;
  } else {
    value = 0;
  }
}

int writer(pid_t reader_pid, int fd_out) {
  sigset_t sig_set = 0;
  sigemptyset(&sig_set);

  struct sigaction sig_act;
  sig_act.sa_flags = 0;
  sig_act.__sigaction_u.__sa_handler = writer_handler;
  sigemptyset(&sig_act.sa_mask);

  if (sigaction(ZERO, &sig_act, NULL) == -1)
    return is_error("Bad sigaction\n");

  if (sigaction(ONE, &sig_act, NULL) == -1)
    return is_error("Bad sigaction\n");

  while (1) {
    char byte = 0;
    for (size_t j = 0; j < 8; j++) {
      sigsuspend(&sig_set);
      byte |= ((char)value << j);
      kill(reader_pid, ZERO);
    }
    if (write(fd_out, &byte, sizeof(byte)) < 0)
      return is_error("Bad write\n");
  }

  return 0;
}

int cat(int fd_in, int fd_out) {
  sigset_t block_sigs = 0, old_sigs = 0;
  sigemptyset(&block_sigs);
  sigaddset(&block_sigs, ZERO);
  sigaddset(&block_sigs, ONE);

  if (sigprocmask(SIG_BLOCK, &block_sigs, &old_sigs) == -1)
    return is_error("Bad sigprocmask\n");

  pid_t reader_pid = getpid();

  pid_t writer_pid = fork();
  if (writer_pid == 0)
    return writer(reader_pid, fd_out);

  reader(writer_pid, fd_in);
  kill(writer_pid, SIGTERM);

  if (sigprocmask(SIG_BLOCK, &old_sigs, NULL) == -1)
    return is_error("Bad sigprocmask\n");

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 1)
    return cat(0, 1);

  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0)
      return is_error("Open failed\n");

    if (cat(fd, 1) < 0)
      return is_error("Cat failed\n");
  }

  return 0;
}
