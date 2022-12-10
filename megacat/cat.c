#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/poll.h>
#include <string.h>
#include <errno.h>

#define __DEBUG__

#ifdef __DEBUG__
#define $$ fprintf(stderr, "%d\n", __LINE__);
#define dump(...) do{ fprintf(stderr, "Line %d ", __LINE__); \
fprintf(stderr, __VA_ARGS__);} while(0)
#else
#define dump(...)
#define $$
#endif

#define is_error(MSG)                                           \
    {                                                           \
        fprintf(stderr, "Line: %d, ", __LINE__);                \
        fprintf(stderr, "Function %s\n", __PRETTY_FUNCTION__);  \
        fprintf(stderr, MSG);                                   \
        goto error;                                             \
    }

const size_t BUF_CAPACITY = 16;

typedef struct pollfd pollfd;

typedef struct pipe_pair
{
    int read[2];
    int write[2];
} pipe_pair;

typedef struct buffer
{
    char data[BUF_CAPACITY];
    size_t size;
    size_t check_sum;
} buffer;

size_t hash_sum(buffer* buf)
{
    assert(buf);

    size_t sum = 0;
    for(size_t i = 0; i < buf->size; i++)
        sum += (unsigned char)buf->data[i];
        
    return sum;
}

int main(int argc, char* argv[])
{
    pipe_pair* pipe_pairs = NULL;
    pollfd* fds = NULL;
    buffer* bufs = NULL;

    if(argc != 3)
        is_error("Argc = 3\n");

    int n_cats = atoi(argv[1]);
    if(n_cats <= 0)
        is_error("Error in n_cats\n");
    
    int n_fds = 2 * n_cats + 2;
    int n_bufs = n_cats + 1;

    pipe_pairs = (pipe_pair* )calloc(n_cats, sizeof(pipe_pair));
    if(!pipe_pairs)
        is_error("Calloc error\n");

    fds = (pollfd* )calloc(n_fds, sizeof(pollfd));
    if(!fds)
        is_error("Calloc error\n");

    bufs = (buffer* )calloc(n_bufs, sizeof(buffer));
    if(!bufs)
        is_error("Calloc error\n");

    for(size_t i = 0; i < n_bufs; i++)
        bufs[i].size = 0;

    for(size_t i = 0; i < n_cats; i++)
    {
        if(pipe(pipe_pairs[i].read) == -1 || pipe(pipe_pairs[i].write) == -1)
            is_error("Bad pipes\n");

        pid_t pid = fork();
        if(pid == 0)
        {
            if(dup2(pipe_pairs[i].read[0], 0) == -1 ||
               dup2(pipe_pairs[i].write[1], 1) == -1)
                is_error("Pipe failed\n");

            for(size_t j = 0; j <= i; j++)
            {
                close(pipe_pairs[j].read[0]);
                close(pipe_pairs[j].read[1]);
                close(pipe_pairs[j].write[0]);
                close(pipe_pairs[j].write[1]);
            }

            execvp(argv[2], &argv[2]);
            is_error("After execvp\n");
        }

        close(pipe_pairs[i].read[0]);
        close(pipe_pairs[i].write[1]);

        fds[2*i+1].fd = pipe_pairs[i].read[1];
        fds[2*i+2].fd = pipe_pairs[i].write[0];
    }

    fds[0].fd = 0;
    fds[n_fds - 1].fd = 1;

    while(1)
    {
        for(size_t i = 0; i < n_fds;)
        {
            size_t n_buf = i/2;
            if(bufs[n_buf].size == 0)
            {
                fds[i].events = POLLIN;
                fds[i+1].events = 0;
            }
            else
            {
                fds[i].events = 0;
                fds[i+1].events = POLLOUT;
            }
            i+=2;
        }

        int n_polled = poll(fds, n_fds, 1000); 
        if(n_polled < 0)
            is_error("Error in poll\n");

        for(size_t i = 0; i < n_fds; i++)
        {
            size_t n_buf = i/2;
            if(fds[i].revents & POLLIN)
            {
                dump("Poll in %d\n", i);
                int n_read = read(fds[i].fd, bufs[n_buf].data, BUF_CAPACITY);
                if(n_read < 0)
                    is_error("Error in read\n");

                dump("To buffer[%zu] read %d bytes\n", n_buf, n_read);
                bufs[n_buf].size = n_read;
                bufs[n_buf].check_sum = hash_sum(&bufs[n_buf]);
                dump("Check sum: %zu\n", bufs[n_buf].check_sum);
            }
            else if(fds[i].revents & POLLOUT)
            {
                dump("Poll out %d\n", i);
                int n_write = write(fds[i].fd, bufs[n_buf].data, bufs[n_buf].size);
                if(n_write < 0)
                    is_error("Error in write");
                dump("From buffer[%zu] wrote %d bytes\n", n_buf, n_write);
                bufs[n_buf].size -= n_write;
            }
        }

        if(n_polled == 0)
        {
            for(size_t i = 0; i < n_cats; i++)
            {
                if(bufs[i].check_sum != bufs[i+1].check_sum)
                {
                    printf("Sum doesn't match:\n"
                           "bufs[%zu].sum = %zu\n"
                           "bufs[%zu].sum = %zu\n", i,   bufs[i].check_sum, 
                                                    i+1, bufs[i+1].check_sum);
                    goto error;
                }
            }
        }
        usleep(100000);
    }

    return 0;

error:
    free(bufs);
    free(fds);
    free(pipe_pairs);
    return -1;
}
