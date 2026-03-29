/*
 * FILE: strbuf.h
 * DESC.: growable byte buffer ADT — used by diagnostics and lowering for
 *        accumulating output without fixed-size copies.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_STRBUF_H
#define CPLUS_STRBUF_H

#include <stddef.h>

/*
 * StrBuf — owning, growable buffer of bytes.
 *
 * Invariants:
 *   - data[len] == '\0' at all times (safe to pass to C string APIs).
 *   - cap > len when data != NULL (room for the sentinel NUL).
 *   - All three fields are zero when uninitialized or after strbuf_free().
 *
 * Ownership: StrBuf owns data. Transfer ownership via strbuf_take(); after
 * that the buffer is reset to zero and the caller owns the returned pointer
 * and must free() it.
 */
typedef struct {
    char   *data;
    size_t  len;
    size_t  cap;
} StrBuf;

/* Zero all fields. Does not allocate. Safe to call on an uninitialized StrBuf. */
void strbuf_init(StrBuf *b);

/* Free owned memory and reset all fields to zero. */
void strbuf_free(StrBuf *b);

/*
 * Append n bytes from data to the buffer.
 * The NUL sentinel is maintained automatically.
 * Returns 1 on success, 0 on allocation failure (buffer unchanged).
 */
int strbuf_append_bytes(StrBuf *b, const char *data, size_t n);

/*
 * Append a NUL-terminated C string (excluding its NUL) to the buffer.
 * Equivalent to strbuf_append_bytes(b, s, strlen(s)).
 * Returns 1 on success, 0 on allocation failure.
 */
int strbuf_append_cstr(StrBuf *b, const char *s);

/*
 * Transfer ownership of the internal buffer to the caller.
 * If out_len is not NULL, *out_len receives the current length (excluding NUL).
 * The StrBuf is reset to zero — equivalent to strbuf_init() after the call.
 * Returns the buffer pointer (caller must free() it), or NULL if empty.
 *
 *   char *output = strbuf_take(&b, &len);
 *   // b is now empty; caller owns output
 */
char *strbuf_take(StrBuf *b, size_t *out_len);

#endif /* CPLUS_STRBUF_H */
