/*
	FILE: IStartable.h
	DESCRIPTION: public definitions of IStartable
	AUTHOR: Andre Cavalcante
	DATE: August, 2025
	LICENSE: CC BY-SA
*/

#ifndef ISTARTABLE_H
#define ISTARTABLE_H


#include <stdbool.h>

typedef struct IStartable_vtable {
    void (*start)(void *self);
    void (*stop)(void *self);
} IStartable_vtable;

#endif /* ISTARTABLE_H */
