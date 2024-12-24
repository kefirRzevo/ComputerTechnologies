#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int BUF_CAPACITY = 16;

int is_error(const char *msg) {
  fprintf(stderr, "%s", msg);
  return -1;
}

typedef struct buffer {
  char data[BUF_CAPACITY];
  size_t size;
  size_t capacity;
} buffer;

typedef struct monitor {
  buffer *bufs;

  size_t capacity;
  size_t size;

  size_t head;
  size_t tail;

  pthread_mutex_t mutex;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
} monitor;

int monitor_ctor(monitor *mon, size_t capa) {
  assert(mon);

  if (pthread_mutex_init(&mon->mutex, NULL) != 0)
    return errno;
  if (pthread_mutex_lock(&mon->mutex) != 0)
    return errno;

  if (pthread_cond_init(&mon->not_empty, NULL) != 0)
    return errno;
  if (pthread_cond_init(&mon->not_full, NULL) != 0)
    return errno;

  buffer *data = (buffer *)calloc(capa, sizeof(buffer));
  if (!data)
    return is_error("Bad alloc\n");

  for (size_t i = 0; i < capa; i++) {
    data[i].capacity = BUF_CAPACITY;
    data[i].size = 0;
  }

  mon->bufs = data;
  mon->size = 0;
  mon->capacity = capa;
  mon->head = 0;
  mon->tail = 0;

  if (pthread_mutex_unlock(&mon->mutex) != 0)
    return free(data), errno;
  return 0;
}

int monitor_dtor(monitor *mon) {
  assert(mon);
  if (pthread_mutex_lock(&mon->mutex) != 0)
    return errno;

  free(mon->bufs);
  mon->bufs = NULL;

  mon->capacity = 0;
  mon->head = 0;
  mon->tail = 0;
  mon->size = 0;

  if (pthread_cond_destroy(&mon->not_empty) != 0)
    return errno;
  if (pthread_cond_destroy(&mon->not_full) != 0)
    return errno;
  if (pthread_mutex_unlock(&mon->mutex) != 0)
    return errno;
  if (pthread_mutex_destroy(&mon->mutex) != 0)
    return errno;
  return 0;
}

buffer *get_empty(monitor *mon) {
  assert(mon);
  if (pthread_mutex_lock(&mon->mutex) != 0)
    return NULL;

  if (mon->size == mon->capacity)
    if (pthread_cond_wait(&mon->not_full, &mon->mutex) != 0)
      return NULL;

  buffer *buf = &mon->bufs[mon->head];

  if (pthread_mutex_unlock(&mon->mutex) != 0)
    return NULL;

  return buf;
}

int set_empty(monitor *mon) {
  assert(mon);
  if (pthread_mutex_lock(&mon->mutex) != 0)
    return errno;

  mon->tail = (mon->tail + 1) % mon->capacity;
  mon->size--;

  if (mon->size == mon->capacity - 1)
    if (pthread_cond_signal(&mon->not_full) != 0)
      return errno;

  if (pthread_mutex_unlock(&mon->mutex) != 0)
    return errno;

  return 0;
}

buffer *get_full(monitor *mon) {
  assert(mon);
  if (pthread_mutex_lock(&mon->mutex) != 0)
    return NULL;

  if (mon->size == 0)
    if (pthread_cond_wait(&mon->not_empty, &mon->mutex) != 0)
      return NULL;

  buffer *buf = &mon->bufs[mon->tail];

  if (pthread_mutex_unlock(&mon->mutex) != 0)
    return NULL;

  return buf;
}

int set_full(monitor *mon) {
  assert(mon);
  if (pthread_mutex_lock(&mon->mutex) != 0)
    return errno;

  mon->head = (mon->head + 1) % mon->capacity;
  mon->size++;

  if (mon->size == 1)
    if (pthread_cond_signal(&mon->not_empty) != 0)
      return errno;

  if (pthread_mutex_unlock(&mon->mutex) != 0)
    return errno;

  return 0;
}

int reader(monitor *mon, int fd) {
  assert(mon);

  int n_read = 0;
  int error = 0;

  do {
    buffer *buf = get_empty(mon);
    if (!buf)
      return is_error("Get empty error\n");

    n_read = read(fd, buf->data, buf->capacity);
    if (n_read < 0)
      return n_read;

    buf->size = n_read;

    error = set_full(mon);
    if (error)
      return error;
  } while (n_read);

  return 0;
}

int writer(monitor *mon, int fd) {
  assert(mon);

  int error = 0;
  buffer *buf = NULL;

  do {
    buf = get_full(mon);
    if (!buf)
      return is_error("Get full error\n");

    int n_write = 0;
    int n_remain = buf->size;
    do {
      n_write += write(fd, buf->data + n_write, n_remain);
      n_remain -= n_write;
    } while (n_remain);

    error = set_empty(mon);
    if (error)
      return error;
  } while (buf->size);

  return 0;
}

typedef struct thread_args {
  monitor *mon;
  int fd;
} thread_args;

void *set_reader(void *arg) {
  thread_args *args = (thread_args *)arg;
  reader(args->mon, args->fd);
  return NULL;
}

void *set_writer(void *arg) {
  thread_args *args = (thread_args *)arg;
  writer(args->mon, args->fd);
  return NULL;
}

int cat(int in_fd, int out_fd) {
  int error = 0;
  monitor mon = {};

  error = monitor_ctor(&mon, 16);
  if (error)
    goto cat_error;

  thread_args reader_args = {};
  reader_args.mon = &mon;
  reader_args.fd = in_fd;

  thread_args writer_args = {};
  writer_args.mon = &mon;
  writer_args.fd = out_fd;

  pthread_t reader_thread, writer_thread;

  error = pthread_create(&reader_thread, NULL, set_reader, &reader_args);
  if (error)
    goto cat_error;

  error = pthread_create(&writer_thread, NULL, set_writer, &writer_args);
  if (error)
    goto cat_error;

  error = pthread_join(reader_thread, NULL);
  if (error)
    goto cat_error;

  error = pthread_join(writer_thread, NULL);
  if (error)
    goto cat_error;

cat_error:
  monitor_dtor(&mon);
  return error;
}

int main(int argc, char *argv[]) {
  if (argc == 1)
    cat(0, 1);

  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0)
      assert(0);

    if (cat(fd, 1) < 0)
      assert(0);
  }

  return 0;
}
