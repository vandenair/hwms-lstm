#ifndef HISTORY_BUFFER_H
#define HISTORY_BUFFER_H

#define HISTORY_SIZE 72

struct HistoryEntry {
    float temperature_c;
    float humidity_pct;
    float hour_sin;
    float hour_cos;
};

void historyBufferSetup();
void historyBufferLoop();
int historyBufferCount();
bool historyBufferIsFull();
HistoryEntry historyBufferGet(int chronologicalIndex);

#endif