/*
 * FILE: strbuf.c
 * DESC.: growable byte buffer ADT implementation.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "strbuf.h"

#include <stdlib.h>
#include <string.h>

/* Minimum initial capacity; always a power of two. */
#define STRBUF_MIN_CAP 256U

void strbuf_init(StrBuf *b) {
    b->data = NULL;
    b->len  = 0U;
    b->cap  = 0U;
}

void strbuf_free(StrBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len  = 0U;
    b->cap  = 0U;
}

int strbuf_append_bytes(StrBuf *b, const char *data, size_t n) {
    if (n == 0U) {
        return 1;
    }

    /* +1 for the NUL sentinel */
    size_t needed = b->len + n + 1U;

    if (needed > b->cap) {
        size_t new_cap = (b->cap == 0U) ? STRBUF_MIN_CAP : b->cap * 2U;
        while (new_cap < needed) {
            new_cap *= 2U;
        }
        char *resized = (char *)realloc(b->data, new_cap);
        if (resized == NULL) {
            return 0;
        }
        b->data = resized;
        b->cap  = new_cap;
    }

    memcpy(b->data + b->len, data, n);
    b->len           += n;
    b->data[b->len]   = '\0';
    return 1;
}

int strbuf_append_cstr(StrBuf *b, const char *s) {
    if (s == NULL) {
        return 1;
    }
    return strbuf_append_bytes(b, s, strlen(s));
}

char *strbuf_take(StrBuf *b, size_t *out_len) {
    char *ptr = b->data;
    if (out_len != NULL) {
        *out_len = b->len;
    }
    b->data = NULL;
    b->len  = 0U;
    b->cap  = 0U;
    return ptr;
}
