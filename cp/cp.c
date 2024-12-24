#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int my_copy(const char *file_name, int dir_fd, int force_flag,
            char *cpfile_name) {
  int error = 0;

  int file_fd = open(file_name, O_RDONLY);
  if (file_fd < 0) {
    error = errno;
    printf("cp: %s can't be opened\n", file_name);
    return error;
  }

  if (force_flag) {
    if (unlinkat(dir_fd, cpfile_name, 0) < 0) {
      error = errno;
      close(file_fd);
      return error;
    }
  }

  struct stat file_st = {};
  if (fstat(file_fd, &file_st) < 0) {
    error = errno;
    close(file_fd);
    return error;
  }

  long long file_size = file_st.st_size;

  char *buf = (char *)malloc(file_size);
  if (!buf) {
    error = errno;
    close(file_fd);
    return error;
  }

  int cpfile_fd =
      openat(dir_fd, cpfile_name, O_TRUNC | O_CREAT | O_RDWR, file_st.st_mode);
  if (cpfile_fd < 0) {
    error = errno;
    close(file_fd);
    free(buf);
    return error;
  }

  if (read(file_fd, buf, file_size) < 0) {
    error = errno;
    close(file_fd);
    close(cpfile_fd);
    free(buf);
    return error;
  }

  if (write(cpfile_fd, buf, file_size) < 0) {
    error = errno;
    close(file_fd);
    close(cpfile_fd);
    free(buf);
    return error;
  }

  return 0;
}

enum cp_flags {
  IF = 1 << 0,
  VF = 1 << 1,
  FF = 1 << 2,
};

int usage() {
  printf("usage: cp [-R [-H | -L | -P]] [-fi | -n] [-aclpsvXx] source_file "
         "target_file\n"
         "       cp [-R [-H | -L | -P]] [-fi | -n] [-aclpsvXx] source_file ... "
         "target_directory\n");
  return 0;
}

int is_dir(char *dir) {
  printf("cp: %s is a directory (not copied).\n", dir);
  return 0;
}

int isno_dir(char *dir) {
  printf("cp: %s is not a directory\n", dir);
  return 0;
}

int ask(char *filepath, char *dir) {
  if (dir)
    printf("overwrite %s/%s? (y/n [n]) ", dir, filepath);
  else
    printf("overwrite %s? (y/n [n]) ", filepath);

  char answer = getchar();
  char temp = 0;

  while ((temp = getchar()) != '\n' && temp != EOF)
    ;

  if (answer != 'y') {
    printf("not overwritten\n");
    return 0;
  }

  return 1;
}

int main(int argc, char *argv[]) {
  int flags = 0;
  int error = 0;
  int ret = 0;

  while ((ret = getopt(argc, argv, "ivf")) != -1) {
    switch (ret) {
    case 'v':
      flags = flags | VF;
      break;
    case 'f':
      flags = flags | FF;
      break;
    case 'i':
      flags = flags | IF;
      break;
    case '?':
    default:
      printf("cp: illegal option -- %c\n", (char)optopt);
      return usage();
    }
  }

  if (argc - optind < 2)
    return usage();

  struct stat cpfile_st = {};
  if (stat(argv[argc - 1], &cpfile_st) < 0)
    return errno;

  if (S_ISDIR(cpfile_st.st_mode)) {
    int dir_fd = open(argv[argc - 1], O_RDONLY | O_DIRECTORY);
    if (dir_fd < 0)
      return errno;

    for (int i = optind; i < argc - 1; i++) {
      if (flags & IF) {
        if (fstatat(dir_fd, basename(argv[i]), &cpfile_st, 0) < 0) {
          error = errno;
          close(dir_fd);
          return error;
        }

        if (!ask(basename(argv[i]), argv[argc - 1]))
          continue;
      }

      error = my_copy(argv[i], dir_fd, flags, basename(argv[i]));
      if (error)
        return error;

      if (flags & VF)
        printf("%s -> %s/%s\n", argv[i], argv[argc - 1], basename(argv[i]));
    }
    close(dir_fd);
    return 0;
  }

  if (argc - optind != 2)
    return isno_dir(argv[argc - 1]);

  if (flags & IF) {
    if (stat(argv[optind + 1], &cpfile_st) < 0)
      return errno;

    if (!ask(argv[argc - 1], NULL))
      return 0;
  }

  error = my_copy(argv[optind], AT_FDCWD, flags, argv[optind + 1]);
  if (error)
    return error;

  if (flags & VF)
    printf("%s -> %s\n", argv[optind], argv[optind + 1]);

  return 0;
}
