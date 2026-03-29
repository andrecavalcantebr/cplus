#include <stdlib.h>
#include "cplus_cleanup.h"

void example(void) {
    int *src __attribute__((cleanup(_cplus_cleanup_free))) = malloc(sizeof(int));
    *src = 99;
    int *dst __attribute__((cleanup(_cplus_cleanup_free))) = src;
    src = NULL;
    (void)dst;
}
