#ifndef RTC_TIME_H
#define RTC_TIME_H

#include <Arduino.h>

void rtcTimeSetup();
void rtcTimeLoop();
unsigned long rtcGetEpochUtc();
int rtcGetHourLocal();
float rtcGetHourSin();
float rtcGetHourCos();

#endif