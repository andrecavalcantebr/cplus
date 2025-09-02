/*
	FILE: Motor.gen.h
	DESCRIPTION: public definitions of Motor
	AUTHOR: Andre Cavalcante
	DATE: August, 2025
	LICENSE: CC BY-SA
*/

#ifndef MOTOR_GEN_H
#define MOTOR_GEN_H


#include <stdbool.h>

#include <stdlib.h>

typedef struct Motor {
    const struct Motor__meta_s *meta;
    int current_rpm;
    bool enabled;
} Motor;

void Motor_init(Motor *self);
void Motor_deinit(Motor *self);
void Motor_start(Motor *self);
void Motor_stop(Motor *self);
void Motor_set_speed(Motor *self, int  rpm);
void Motor__sys_init(void);
void Motor__sys_deinit(void);

#endif /* MOTOR_GEN_H */
