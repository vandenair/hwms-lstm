#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <Arduino.h>

void actuatorSetup();
void actuatorLoop();
void actuatorHandleCommand(byte* payload, unsigned int length);

#endif