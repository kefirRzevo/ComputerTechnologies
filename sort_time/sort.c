#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char* argv[])
{
    for(int i = 1; i < argc; i++)
    {
        int temp = atoi(argv[i]);
        int pid  = fork();
        if(pid == 0)
        {
            usleep(20000*temp);
            printf("|%d ", temp);
            exit(0);
        }
    }

    for(int i = 1; i < argc; i++)
        wait(NULL);

    printf("|\n");

    return 0;
}