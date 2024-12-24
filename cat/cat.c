#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int cat(int in, int out) {
  char buf[32] = {};
  int n = 0;

  do {
    n = read(in, buf, sizeof(buf));
    if (n < 0)
      return n;

    n = write(out, buf, n);
    if (n < 0)
      return n;
  } while (n > 0);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 1)
    cat(0, 1);

  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0)
      assert(0);

    if (cat(fd, 1) < 0)
      assert(0);
  }

  return 0;
}
