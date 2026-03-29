#include <stdlib.h>

static void int_free(int **pp) {
    if (*pp != NULL) { free(*pp); *pp = NULL; }
}

static inline void _cplus_cleanup_int_free(int **pp) {
    if (*pp != NULL) { int_free(pp); *pp = NULL; }
}

void example(void) {
    int *p __attribute__((cleanup(_cplus_cleanup_int_free))) = malloc(sizeof(int));
    *p = 42;
    (void)p;
}
