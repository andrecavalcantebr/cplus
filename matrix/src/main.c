/*
	FILE: main.c
	DESCR: entry point of the program
	AUTHOR: Andre Cavalcante
	DATE: September, 2025
	LICENSE: CC BY-SA
*/

#include <stdio.h>
#include <stdlib.h>

#include "tgc.h"
#include "matrix.h"

static tgc_t gc;

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

    tgc_start(&gc, &argc);
    mat_init(&gc);
	
	printf("Matrix!\n");

    Matrix *m = mat_eye(3);
    mat_print("m", m);
	
	printf("Bye!\n");

    mat_deinit();
    tgc_stop(&gc);
	return EXIT_SUCCESS;
}
