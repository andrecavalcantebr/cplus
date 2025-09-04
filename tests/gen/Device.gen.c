/*
	FILE: ./tests/gen/Device.gen.c
	DESCRIPTION: public definitions of Device
	AUTHOR: Andre Cavalcante
	DATE: August, 2025
	LICENSE: CC BY-SA
*/

#include <stdio.h>
#include <stdlib.h>

#include "Device.gen.h"

struct Device__meta_s {
    const char *name;
    size_t size;
};

static const struct Device__meta_s Device__meta = { "Device", sizeof(Device) };

void Device_power_on(Device *self)
{
    /* TODO: generated body */
    return;
}

void Device_power_off(Device *self)
{
    /* TODO: generated body */
    return;
}

void Device__sys_init(void) {
    (void)Device__meta; /* TODO: set up vtables and meta */
}

void Device__sys_deinit(void) {
    /* TODO: tear down */
}
