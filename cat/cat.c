#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const int BUF_MAX_SIZE = 4096;


int my_cat(int input, int output);
int my_write(int fd, const void* buf, size_t nbyte);

int my_write(int fd, const void* buf, size_t nbyte)
{
    int swb = 0;
    while(swb < nbyte)
    {
        int wb = write(fd, buf + swb, nbyte - swb);
        if(wb < 0)
            return wb;
        swb+=wb;
    }
    return swb;
}


int my_cat(int input, int output)
{
    char buf[BUF_MAX_SIZE] = {};
    int n = 0;
    int err = 0;

    while((n = read(input, buf, BUF_MAX_SIZE)) > 0)
        if((err = my_write(output, buf, n)) < 0)
            fprintf(stderr, "ERROR %d: output %d can't be written.", err, output);

    if(n < 0)
        fprintf(stderr, "ERROR %d: input %d can't be read.", n, input);

    return 1;
}


int main(int argc, char* argv[])
{
    if(argc == 1)
        my_cat(0, 1);

    for(int i = 1; i < argc; i++)
    {
        int fd = open(argv[i], O_RDONLY);
        if(fd < 0)
            fprintf(stderr, "FILE %s can't be opened.", argv[i]);

        my_cat(fd, 1);
    }

    return 0;
}
