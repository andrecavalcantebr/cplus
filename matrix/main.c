/*****************************************************************************/
/**** Programação de Tempo Real                                           ****/
/**** Prof. André Cavalcante, 2015(c)                                     ****/
/*****************************************************************************/
/*** Para compilar:						                                   ***/
/***    $ make								                               ***/
/*****************************************************************************/

// Includes

#include <stdio.h>
#include "matrix.h"


/* Main */

int main(int argc, char * args[])
{
	Matrix A,B;
/*	int k;*/

	matrix_zeros(&A);		// valor inicial
	matrix_identity(&B);	// valor inicial
	matrix_display(A, 2, 3);
	matrix_display(B, 3, 3);

	return 0;
}

