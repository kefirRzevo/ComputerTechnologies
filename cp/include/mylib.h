#ifndef MYLIB_H
#define MYLIB_H

//#define DEBUG

int my_copy(const char* from, int to_d, int force_flag, char* to_name);

#ifdef DEBUG
#define $$ fprintf(stderr, "%s, %d\n", __FILE__, __LINE__);
#endif
#ifndef DEBUG
#define $$
#endif


#endif //MYLIB_H
