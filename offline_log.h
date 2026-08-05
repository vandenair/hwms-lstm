#ifndef OFFLINE_LOG_H
#define OFFLINE_LOG_H

#include <Arduino.h>

void offlineLogSetup();
void offlineLogRecordIfDue(const String& jsonPayload);
void offlineLogSyncIfConnected();

#endif