/*****************************************************************************/
/**** Programação de Tempo Real                                           ****/
/**** Prof. André Cavalcante, 2015(c)                                     ****/
/**** Descrição: Biblioteca de funções para manipulacao matricial         ****/
/****			 Arquivo de cabeçalho									  ****/
/*****************************************************************************/

#ifndef __MATRIX_H__
#define __MATRIX_H__

/* Defines */

#define N_MAX	6
#define M_MAX	6

/* Typedefs */

typedef double Matrix[N_MAX][M_MAX];

/* Prototypes */

extern void matrix_display(Matrix m, int N, int M);
extern void matrix_zeros(Matrix * m);
extern void matrix_ones(Matrix * m);
extern void matrix_identity(Matrix * m);
extern void matrix_add(Matrix * sol, Matrix a, Matrix b, int N, int M);
extern void matrix_mult_c(Matrix * sol, double c, Matrix m, int N, int M);
extern void matrix_mult(Matrix * sol, Matrix a, Matrix b, int N, int P, int M);
extern void matrix_transp(Matrix * sol, Matrix m, int N, int M);
extern void matrix_exterm(Matrix * sol, Matrix m, int i, int j, int N, int M);
extern double matrix_det(Matrix m, int N);
extern void matrix_cof(Matrix * sol, Matrix m, int N);
extern void matrix_inv(Matrix * sol, Matrix m, int N);

#endif

