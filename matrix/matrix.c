/*****************************************************************************/
/**** Programação de Tempo Real                                           ****/
/**** Prof. André Cavalcante, 2015(c)                                     ****/
/**** Descrição: Biblioteca de funções para manipulacao matricial         ****/
/*****************************************************************************/

#include <stdio.h>
#include "matrix.h"

/*
	Mostra os valores de uma matriz
*/
void matrix_display(Matrix m, int N, int M)
{
	int i,j;
	
	for(i=0; i<N; i++)
	{
		for(j=0; j<M; j++)
		{
			printf("\t%6.4f", m[i][j]);
		}
		printf("\r\n");
	}
}


/*
	Inicializa uma matriz com zeros
*/
void matrix_zeros(Matrix * m)
{
	int i,j;

	for(i=0; i<N_MAX; i++)
	{
		for(j=0; j<M_MAX; j++)
		{
			(*m)[i][j] = 0;
		}
	}
}


/*
	Inicializa uma matriz com uns
*/
void matrix_ones(Matrix * m)
{
	int i,j;

	for(i=0; i<N_MAX; i++)
	{
		for(j=0; j<M_MAX; j++)
		{
			(*m)[i][j] = 1;
		}
	}
}


/*
	Inicializa uma matriz identidade
*/
void matrix_identity(Matrix * m)
{
	int i,j;
	
	for(i=0; i<N_MAX; i++)
	{
		for(j=0; j<M_MAX; j++)
		{
			(*m)[i][j] = (i==j)?1:0;
		}
	}
}


/*
	Calcula o valor da soma de duas matrizes NxM
*/
void matrix_add(Matrix * sol, Matrix a, Matrix b, int N, int M)
{
	int i,j;
	
	for(i=0; i<N; i++)
	{
		for(j=0; j<M; j++)
		{
			(*sol)[i][j] = a[i][j] + b[i][j];
		}
	}
}


/*
	Calcula o valor da multiplicacao de um escalar por uma matriz
*/
void matrix_mult_c(Matrix * sol, double c, Matrix m, int N, int M)
{
	int i,j;
	
	for(i=0; i<N; i++)
	{
		for(j=0; j<M; j++)
		{
			(*sol)[i][j] = (m)[i][j] * c;
		}
	}
}


/*
	Calcula o valor da multiplicacao de duas matrizes: A:NxP e B:PxM 
*/
void matrix_mult(Matrix * sol, Matrix a, Matrix b, int N, int P, int M)
{
	int i,j,k;
	
	for(i=0; i<N; i++)
	{
		for(j=0; j<M; j++)
		{
			(*sol)[i][j] = 0;
			for(k=0; k<P; k++)
			{
				(*sol)[i][j] += (a[i][k] * b[k][j]);
			}
		}
	}
}

/*
	Faz a transposta da matriz dada
*/
void matrix_transp(Matrix * sol, Matrix m, int N, int M)
{
	int i,j;
	
	for(i=0; i<N; i++)
	{
		for(j=0; j<M; j++)
		{
			(*sol)[j][i] = m[i][j];
		}
	}
}

/*
	Retorna uma matriz de ordem -1 da matriz dada com a linha
	e a coluna suprimidas
*/
void matrix_exterm(Matrix * sol, Matrix m, int i, int j, int N, int M)
{
	int a, b, c, d;
	
	for(a=c=0; a<i; a++)
	{
		for(b=d=0; b<M; b++)
		{
			if(b != j)
			{
				(*sol)[c][d] = m[a][b];
				d++;
			}
		}
		c++;
	}
	
	for(a=i+1; a<N; a++)
	{
		for(b=d=0; b<M; b++)
		{
			if(b != j)
			{
				(*sol)[c][d] = m[a][b];
				d++;
			}
		}
		c++;
	}
}

/*
	Retorna o determinante da matriz dada
*/

double matrix_det(Matrix m, int N)
{
	int i,j;
	Matrix aux;
	double result;
	
	switch(N){
		case 0:  return 0;
		case 1:  return m[0][0];
		case 2:  return m[0][0]*m[1][1] - m[1][0]*m[0][1];
		default:
			i=0;		/*i-esima linha*/
			result = 0;
			for(j=0; j<N; j++)
			{
				matrix_exterm(&aux, m, i, j, N, N);
				if((i+j)%2 == 0)
					result += m[i][j]*matrix_det(aux,N-1);
				else
					result -= m[i][j]*matrix_det(aux,N-1);
			}
			return result;
	}
}


/*
	Calcula a matriz de cofatores da matriz dada
*/
void matrix_cof(Matrix * sol, Matrix m, int N)
{
	Matrix aux;
	int i,j;

	for(i=0; i<N; i++)
	{
		for(j=0; j<N; j++)
		{
			matrix_exterm(&aux, m, i, j, N, N);
			if((i+j)%2 == 0)
				(*sol)[i][j] = matrix_det(aux, N-1);
			else
				(*sol)[i][j] = -matrix_det(aux, N-1);
		}
	}
}


/*
	Calcula a matriz inversa da matriz dada
*/
void matrix_inv(Matrix * sol, Matrix m, int N)
{
	double aux, det;
	Matrix t1,t2;
	
	matrix_zeros(sol);
	det = matrix_det(m, N);
	if(det == 0) return;
	aux = 1 / det;
	matrix_cof(&t1, m, N);
	matrix_transp(&t2, t1, N, N);
	matrix_mult_c(sol, aux, t2, N, N);
}


