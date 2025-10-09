/*
	FILE: matrix.h
	DESCRIPTION: public definitions of ADT Matrix
	AUTHOR: Andre Cavalcante
	DATE: September, 2025
	LICENSE: CC BY-SA
*/

#ifndef MATRIX_H__
#define MATRIX_H__

#include <stdbool.h>
#include <stdlib.h>
#include "tgc.h"

typedef struct matrix Matrix;

// library initialization and finalization
void mat_init(tgc_t *gc);
void mat_deinit(void);

// constructors
Matrix *mat_new(size_t rows, size_t cols);
Matrix *mat_zeros(size_t rows, size_t cols);
Matrix *mat_ones(size_t rows, size_t cols);
Matrix *mat_eye(size_t n);

// basic ops
Matrix *mat_add(const Matrix *A, const Matrix *B);
Matrix *mat_sub(const Matrix *A, const Matrix *B);
Matrix *mat_mul(const Matrix *A, const Matrix *B);
Matrix *mat_transpose(const Matrix *A);

// utilities
void mat_fill(Matrix *M, double val);
bool mat_same_shape(const Matrix *A, const Matrix *B);
void mat_print(const char *name, const Matrix *A);

#endif //MATRIX_H__