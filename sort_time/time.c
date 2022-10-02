#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>


int main(int argc, char* argv[])
{
    struct timeval  t1, t2, dt;

    gettimeofday(&t1, NULL);
    int pid = fork();
    if(pid == 0)
    {
        execv("./sort", &argv[1]);
        exit(0);
    }
    wait(NULL);
    gettimeofday(&t2, NULL);

    dt.tv_sec  = t2.tv_sec  - t1.tv_sec;
    dt.tv_usec = t2.tv_usec - t1.tv_usec;

    printf("Time: %lf\n", dt.tv_sec+0.000001*dt.tv_usec);
    return 0;
}
