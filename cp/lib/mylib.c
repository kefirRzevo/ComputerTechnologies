#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include "../include/mylib.h"


static int my_write(int fd, const void* buf, size_t nbyte)
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

static int my_read(int fd, void* buf, size_t count)
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
    int from_d = open(from, O_RDONLY);$$
    if(from_d < 0)
    {
        printf("cp: %s/%s can't be opened\n", to_name, from);
        return errno;
    }

    if(force_flag) 
        unlink(to_name);
$$
    struct stat from_st = {};
    if(fstat(from_d, &from_st) < 0)
        return errno;
$$
    long long from_size = from_st.st_size;
$$
    char* buf = (char* )malloc(from_size);
    if(!buf)
        return errno;
$$
    if(my_read(from_d, buf, from_size) < 0)
    {
        free(buf);
        return errno;
    }
$$
    if(my_write(to_d, buf, from_size) < 0)
    {
        free(buf);
        return errno;
    }
$$
    return 0;
}
