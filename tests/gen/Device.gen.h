/*
	FILE: ./tests/gen/Device.gen.h
	DESCRIPTION: public definitions of Device
	AUTHOR: Andre Cavalcante
	DATE: August, 2025
	LICENSE: CC BY-SA
*/

#ifndef DEVICE_GEN_H
#define DEVICE_GEN_H


#include <stdbool.h>

#include <stdlib.h>

typedef struct Device {
    const struct Device__meta_s *meta;
    int voltage;
    bool status;
} Device;

void Device_power_on(Device *self);
void Device_power_off(Device *self);
void Device__sys_init(void);
void Device__sys_deinit(void);

#endif /* DEVICE_GEN_H */
