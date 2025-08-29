/* demo.cplus — Exemplo para testar parser Cplus */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "demo.cp.h"

//public will be ignored
public void Device_power_on(class Device *self) {
  self->status = true;
}

// assumes same modifier of the class
void Device_power_off(class Device *self) {
  self->status = false;
}

void Motor_init(Motor *self) {
  self->enabled = true;
  self->current_rpm = 0;
}

void Motor_deinit(Motor *self) {
  self->enabled = false;
}

void Motor_start(void *self) {
  //write code what does the motor start
  Motor *m = (Motor *)self;
  printf("Motor started\n");
}

void Motor_stop(void *self) {
  //write code what does the motor stop 
  Motor *m = (Motor *)self;
  printf("Motor stopped\n");
}

void Motor_set_speed(class Motor *self, int rpm) {
  const int factor = 20;
  self->voltage = rpm * factor; 
}

