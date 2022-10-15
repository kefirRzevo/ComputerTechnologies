#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libgen.h> 
#include <assert.h>
#include <dirent.h>


enum ls_flags
{
    lF = 1 << 0,
    dF = 1 << 1,
    aF = 1 << 2,
    RF = 1 << 3,
    iF = 1 << 4,
    nF = 1 << 5,
};


char* unite_path(char* path1, char* path2)
{
    size_t s1 = strlen(path1);
    size_t s2 = strlen(path2);
    size_t s  = s1 + s2 + 2;

    char* new_path = (char* )malloc(s);
    assert(new_path);

    strcpy(new_path, path1);
    new_path[s1] = '/';
    strcpy(new_path + s1 + 1, path2);
    new_path[s - 1] = '\0';

    return new_path;
}


void ls(DIR* opdir, char* path, int flags)
{
    if(flags | RF)
    {
        struct dirent* dir = readdir(opdir);
        char* path = unite_path(path, dir->d_name);
    }
    
}

void show(DIR* opdir, char* path, int flags)
{
    struct dirent* dir = NULL;

    while(dir = readdir(opdir))
    {
        assert(dir);

        char* new_path = unite_path(path, dir->d_name);
        struct stat meta = {};
        stat(new_path, &meta);

        if(flags & lF)
        {
            printf("%llu\t%s\t%d\n", dir->d_ino, dir->d_name, meta.st_ctimespec.tv_sec);
        }
        else if(flags & iF)
        {
            printf("%llu\t%s\n", dir->d_ino, dir->d_name);
        }
        else if(flags & nF)
        {
            printf("%llu\t%s\t%d\n", dir->d_ino, dir->d_name, meta.st_ino);
        }
        else if(flags & aF || dir->d_name[0] != '.')
        {
            printf("%s\n", dir->d_name);
        }

        free(new_path);
    }
}

int main(int argc, char* argv[])
{
    int flags = 0;
    int error = 0;
    int ret   = 0;

    while((ret = getopt(argc, argv, "ldaRin")) != -1)
    {
        switch(ret)
        {
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

    if(optind == argc)
    {
        DIR* opdir = opendir(".");
        ls(opdir, ".", flags);
        closedir(opdir);
    }

    for(int i = optind; i < argc; i++)
    {
        DIR* opdir = opendir(argv[i]);
        ls(opdir, argv[i], flags);
        closedir(opdir);
    }

    return 0;
}
