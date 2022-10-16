#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <assert.h>


int my_read(int fd, void* buf, size_t count);

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

int main(int argc, char* argv[])
{
    int fds[2] = {};

    pipe(fds);
    int pid = fork();
    
    if(pid == 0)
    {
        dup2(fds[1], 1);
        char temp[64] = {};
        for(int i = 0; i < argc - 1; i++)
        {
            strcpy(temp, argv[i+1]);
            strcpy(argv[i], temp);
        }

        argv[argc-1] = NULL;
        execv(argv[0], &argv[1]);

        close(fds[1]);
        close(fds[0]);
    }
    close(fds[1]);

    int n = 0, b = 0, w = 0, l = 0;
    int is_word = 0;
    do
    {
        char buf[32];
        int n = read(fds[0], buf, 32);

        for(int i = 0; i < n; i++)
        {
            if(n < 0)
                assert(0);

            if(buf[i] == '\n')
                l++;

            b++;

            if(is_word)
            {
                if(!isalnum(buf[i]))
                {
                    is_word = 0;
                }
            }
            else
            {
                if(isalnum(buf[i]))
                {
                    w++;
                    is_word = 1;
                }    
            }
        }
    }while(n);

    close(fds[0]);
    wait(NULL);

    close(fds[0]);
    printf("%d\t%d\t%d\n", l, w, b);
    return 0;
}
