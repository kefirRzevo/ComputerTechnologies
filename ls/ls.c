#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void ls(char *path, int flags);
void show(char *path, int flags);

void mode_to_str(mode_t mode, char *buf);
void time_to_str(struct timespec time, char *buf, size_t buf_len);
char *unite_path(char *path1, char *path2);

enum ls_flags {
  lF = 1 << 0,
  dF = 1 << 1,
  aF = 1 << 2,
  RF = 1 << 3,
  iF = 1 << 4,
  nF = 1 << 5,
};

char *unite_path(char *path1, char *path2) {
  size_t s1 = strlen(path1);
  size_t s2 = strlen(path2);
  size_t s = s1 + s2 + 2;

  char *new_path = (char *)malloc(s);
  assert(new_path);

  strcpy(new_path, path1);
  new_path[s1] = '/';
  strcpy(new_path + s1 + 1, path2);
  new_path[s - 1] = '\0';

  return new_path;
}

void mode_to_str(mode_t mode, char *buf) {
  const char str[] = "rwxrwxrwx";

  for (int i = 0; i < 9; i++)
    buf[i] = (mode & (1 << (8 - i))) ? str[i] : '-';

  buf[9] = '\0';
}

void time_to_str(struct timespec time, char *buf, size_t buf_len) {
  struct tm *time_ptr = localtime(&time.tv_sec);
  strftime(buf, buf_len, "%d %b %H:%M", time_ptr);
}

void is_error(char *path) {
  printf("ls: %s: No such file or directory\n", path);
  return;
}

void ls(char *path, int flags) {
  DIR *opdir = opendir(path);
  if (!opdir)
    return is_error(path);

  struct dirent *dir = NULL;

  show(path, flags);

  while (1) {
    dir = readdir(opdir);
    if (!dir)
      break;

    if (flags & RF && !(flags & dF)) {
      char *new_path = unite_path(path, dir->d_name);
      assert(new_path);

      struct stat meta = {};
      lstat(new_path, &meta);

      if (S_ISDIR(meta.st_mode)) {
        if (strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")) {
          printf("\n%s:\n", new_path);
          ls(new_path, flags);
        }
      }

      free(new_path);
    }
  }
  closedir(opdir);
}

void show(char *path, int flags) {
  assert(path);

  DIR *opdir = opendir(path);
  if (!opdir)
    return is_error(path);

  struct dirent *dir = NULL;

  while (1) {
    dir = readdir(opdir);
    if (!dir)
      break;

    if ((!(flags & aF)) & (dir->d_name[0] == '.'))
      continue;

    char *new_path = unite_path(path, dir->d_name);
    assert(new_path);

    struct stat meta = {};
    lstat(new_path, &meta);

    if (flags & nF) {
      if (flags & iF)
        printf("%-11llu", dir->d_ino);

      char mode_buf[10] = {};
      char time_buf[32] = {};

      mode_to_str(meta.st_mode, mode_buf);
      time_to_str(meta.st_ctimespec, time_buf, 32);

      printf(" %-11s%-2d%-5d%-3d%6lld %-13s%s\n", mode_buf, meta.st_nlink,
             meta.st_uid, meta.st_gid, meta.st_size, time_buf, dir->d_name);
    } else if (flags & lF) {
      if (flags & iF)
        printf("%-11llu", dir->d_ino);

      char mode_buf[10] = {};
      char time_buf[32] = {};

      mode_to_str(meta.st_mode, mode_buf);
      time_to_str(meta.st_ctimespec, time_buf, 32);

      struct passwd *user = getpwuid(meta.st_uid);
      struct group *group = getgrgid(meta.st_gid);

      printf(" %-11s%-2d%-17s%-7s%6lld %-13s%s\n", mode_buf, meta.st_nlink,
             user->pw_name, group->gr_name, meta.st_size, time_buf,
             dir->d_name);
    } else {
      if (flags & iF)
        printf("%-11llu", dir->d_ino);

      printf("%s\t", dir->d_name);
    }
    free(new_path);
  }

  if (!(flags & iF) && !(flags & lF) && !(flags & nF))
    printf("\n");

  closedir(opdir);
}

int main(int argc, char *argv[]) {
  int flags = 0;
  int error = 0;
  int ret = 0;

  while ((ret = getopt(argc, argv, "ldaRin")) != -1) {
    switch (ret) {
    case 'l':
      flags = flags | lF;
      break;
    case 'd':
      flags = flags | dF;
      break;
    case 'a':
      flags = flags | aF;
      break;
    case 'R':
      flags = flags | RF;
      break;
    case 'i':
      flags = flags | iF;
      break;
    case 'n':
      flags = flags | nF;
      break;
    case '?':
    default:
      assert(0);
    }
  }

  if (optind == argc) {
    ls(".", flags);
  }

  for (int i = optind; i < argc; i++) {
    if (argc - optind > 1)
      printf("%s:\n", argv[i]);

    ls(argv[i], flags);
  }

  return 0;
}
