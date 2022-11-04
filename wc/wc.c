#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    memmove(argv, argv + 1, sizeof(char *) * argc);
    argv[argc] = NULL;

    int fds[2] = {};
    pipe(fds);

    int pid = fork();
    if(pid == 0)
    {
        dup2(fds[1], 1);
        char temp[64] = {};

        execvp(*argv, argv);

        close(fds[1]);
        close(fds[0]);

        return 0;
    }
    close(fds[1]);

    int n = 0, b = 0, w = 0, l = 0;
    int is_word = 0;
    char buf[32];

    do{
        n = read(fds[0], buf, sizeof(buf));

        if(n < 0)
            assert(0);

        for(int i = 0; i < n; i++)
        {
            if(buf[i] == '\n')
                l++;

            if(is_word)
            {
                if(isspace(buf[i]))
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
        b+=n;

    }while(n > 0);


    close(fds[0]);
    wait(NULL);

    printf("%d\t%d\t%d\n", l, w, b);
    return 0;
}
