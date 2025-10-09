/*
	FILE: matrix.c
	DESCRIPTION: implementation of ADT Matrix
	AUTHOR: Andre Cavalcante
	DATE: September, 2025
	LICENSE: CC BY-SA
*/

#include <stdio.h>
#include "matrix.h"
#include "tgc.h"

struct matrix {
    size_t rows;
    size_t cols;
    double *vals;
};

static tgc_t *gc;

#define VALUE(m, i, j) ((m)->vals[i * (m)->cols + j])

// library initialization and finalization
void mat_init(tgc_t *_gc) {
    gc = _gc;
}

void mat_deinit(void) {
    //nothing for now
}

// destructor
static void mat_dtor(void *p) {
    Matrix *m  = (Matrix *)p;
    if(m && m->vals) {
        free(m->vals);
    }
}

// alloc
static Matrix *mat_alloc(size_t rows, size_t cols) {
    Matrix *m = tgc_alloc_opt(gc, sizeof(*m), 0, mat_dtor);
    if(m) {
        m->vals = tgc_alloc(gc, rows*cols*sizeof(m->vals[0]));
        if((m->vals)) {
            m->rows = rows;
            m->cols = cols;
        } else {
            free(m);
            m = NULL;
        }
    }
    return m;
}

// constructors
Matrix *mat_new(size_t rows, size_t cols) {
    return mat_alloc(rows, cols);
}

Matrix *mat_zeros(size_t rows, size_t cols) {
    Matrix *m = mat_alloc(rows, cols);
    if(m) mat_fill(m, 0.0);
    return m;
}

Matrix *mat_ones(size_t rows, size_t cols) {
    Matrix *m = mat_alloc(rows, cols);
    if(m) mat_fill(m, 1.0);
    return m;
}

Matrix *mat_eye(size_t n) {
    Matrix *m = mat_alloc(n, n);
    if(m) {
        for(size_t i=0; i < m->rows; i++)
            for(size_t j=0; j < m->cols; j++)
                VALUE(m, i, j) = ((i==j) ? 1.0 : 0.0);
    }
    return m;
}

// basic ops
Matrix *mat_add(const Matrix *A, const Matrix *B) {
    if(!mat_same_shape(A,B)) {
        fprintf(stderr, "Shape mismatch in add\n");
        return NULL;
    }
    Matrix *m = mat_alloc(A->rows, A->cols);
    if(m)
        for(size_t i=0; i < (m->rows * m->cols); i++)
            m[i].vals[i] = A->vals[i] + B->vals[i];
    return m;
}

Matrix *mat_sub(const Matrix *A, const Matrix *B) {
    if(!mat_same_shape(A,B)) {
        fprintf(stderr, "Shape mismatch in sub\n");
        return NULL;
    }
    Matrix *m = mat_alloc(A->rows, A->cols);
    if(m)
        for(size_t i=0; i < (m->rows * m->cols); i++)
            m[i].vals[i] = A->vals[i] - B->vals[i];
    return m;
}

Matrix *mat_mul(const Matrix *A, const Matrix *B) {
    if(A->cols != B->rows) {
        fprintf(stderr, "Shape mismatch in mul\n");
        return NULL;
    }
    Matrix *m = mat_alloc(A->rows, A->cols);
    if(m)
        for(size_t i=0; i < (m->rows * m->cols); i++)
            m[i].vals[i] = A->vals[i] + B->vals[i];
    return m;

}

Matrix *mat_transpose(const Matrix *A) {
    Matrix *m = mat_alloc(A->cols, A->rows);
    if(m) {
        for(size_t i=0; i < m->rows; i++)
            for(size_t j=0; j < m->cols; j++)
                VALUE(m, i, j) = VALUE(A, j, i);
    }
    return m;
}

// utilities
void mat_fill(Matrix *m, double val) {
    if(m) {
        for(size_t i=0; i < m->rows; i++)
            for(size_t j=0; j < m->cols; j++)
                VALUE(m, i, j) = val;
    }
}

bool mat_same_shape(const Matrix *A, const Matrix *B) {
    return ((A->rows == B->rows) && (A->cols == B->cols));
}

void mat_print(const char *name, const Matrix *A) {
    printf("%s(%ld,%ld) = [\n", name, A->rows, A->cols);
    for(size_t i=0; i < A->rows; i++) {
        for(size_t j=0; j < A->cols; j++)
            printf("\t%f%s", VALUE(A, i, j), (j < A->cols-1) ? " " : "");
        printf("\n");
    }
    printf("]\n");
}

