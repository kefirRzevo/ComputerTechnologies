#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libgen.h> 

int my_write(int fd, const void* buf, size_t nbyte)
{
    int swb = 0;
    while(swb < nbyte)
    {
        int wb = write(fd, buf + swb, nbyte - swb);
        if(wb < 0)
            return errno;
        swb+=wb;
    }
    return swb;
}

int my_read(int fd, void* buf, size_t count)
{
    int srb = 0;
    while(srb < count)
    {
        int rb = read(fd, buf + srb, count - srb);
        if(rb < 0)
            return errno;
        srb+=rb;
    }
    return srb;
}

int my_copy(const char* from, int to_d, int force_flag, char* to_name)
{
    int from_d = open(from, O_RDONLY);
    if(from_d < 0)
    {
        printf("cp: %s/%s can't be opened\n", to_name, from);
        return errno;
    }

    if(force_flag) 
        unlink(to_name);

    struct stat from_st = {};
    if(fstat(from_d, &from_st) < 0)
        return errno;

    long long from_size = from_st.st_size;

    char* buf = (char* )malloc(from_size);
    if(!buf)
        return errno;

    if(my_read(from_d, buf, from_size) < 0)
    {
        free(buf);
        return errno;
    }

    if(my_write(to_d, buf, from_size) < 0)
    {
        free(buf);
        return errno;
    }

    return 0;
}

enum cp_flags
{
    IF = 1 << 0,
    VF = 1 << 1,
    FF = 1 << 2,
};

int print_usage()
{
    printf("usage: cp [-R [-H | -L | -P]] [-fi | -n] [-aclpsvXx] source_file target_file\n"
           "       cp [-R [-H | -L | -P]] [-fi | -n] [-aclpsvXx] source_file ... target_directory\n");
           return 0;
}

int print_is_dir(char* dir)
{
    printf("cp: %s is a directory (not copied).\n", dir);
    return 0;
}

int print_no_dir(char* dir)
{
    printf("cp: %s is not a directory\n", dir);
    return 0;
}

int print_no_file_dir(char* file_dir)
{
    printf("cp: %s: No such file or directory\n", file_dir);
    return 0;
}

int is_error(int error)
{
    fprintf(stderr, "Error: %d\n", error);
    return 0;    
}

int ask(char* filepath, char* dir)
{
    if(dir)
        printf("overwrite %s/%s? (y/n [n]) ", dir, filepath);
    else
        printf("overwrite %s? (y/n [n]) ", filepath);

    char answer = getchar();
    char temp = 0;

    while((temp = getchar()) != '\n' && temp != EOF);

    if(answer != 'y')
    {
        printf("not overwritten\n");
        return 0;
    }
    return 1;
}

int main(int argc, char* argv[])
{
    int flags = 0;
    int error = 0;
    int ret   = 0;

    if(argc == 1)
        return 0;

    while((ret = getopt(argc, argv, "ivf")) != -1)
    {
        switch(ret)
        {
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
                    return print_usage();
        }
    }

    if(argc - optind < 2)
        return print_usage();

    struct stat to = {};
    if(stat(argv[argc - 1], &to) < 0)
        return print_no_file_dir(argv[argc - 1]);

    if(S_ISDIR(to.st_mode))
    {
        int to_d = open(argv[argc - 1], O_RDONLY | O_DIRECTORY);
        for(int i = optind; i < argc - 1; i++)
        {
            if(flags & IF)
                if(!ask(basename(argv[i]), argv[argc - 1]))
                    continue;

            struct stat to_file = {};

            if(stat(argv[i], &to_file) < 0)
                return print_no_file_dir(argv[i]);

            if(S_ISDIR(to_file.st_mode))
            {
                print_is_dir(argv[i]);
                continue;
            }

            int to_f = openat(to_d, basename(argv[i]), O_RDWR);

            if(my_copy(argv[i], to_f, flags | FF, argv[argc - 1]) < 0)
                return is_error(errno);  

            if(flags & VF) 
                printf("%s -> %s/%s\n", argv[i], argv[argc - 1], basename(argv[i]));    
        }    
        return 0; 
    }

    if(argc - optind != 2)
        return print_no_dir(argv[argc - 1]);

    if(flags & IF)
        if(!ask(argv[argc - 1], NULL))
            return 0;

    int to_d = open(argv[argc - 1], O_RDWR);

    if(my_copy(argv[optind], to_d, flags & FF, argv[argc - 1]) < 0)
        return is_error(errno);

    if(flags & VF) 
        printf("%s -> %s\n", argv[optind], argv[argc - 1]);

    return 0;
}
