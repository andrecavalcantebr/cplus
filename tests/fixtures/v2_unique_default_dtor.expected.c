#include <stdlib.h>
#include "cplus_cleanup.h"

void example(void) {
    int *p __attribute__((cleanup(_cplus_cleanup_free))) = malloc(sizeof(int));
    *p = 42;
    (void)p;
}
