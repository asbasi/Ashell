#ifndef HISTORY_H
#define HISTORY_H

#define INIT_COMMAND_SIZE 100
#define MAX_NUM_ENTRIES 10

typedef struct
{
    char** previousCommands;

    int latestIndex;
    int lastIndex;

    int currentIndex;
} historyTable;

historyTable* createHistoryTable();
void destroyHistoryTable(historyTable* hist);

char* scrollUp(historyTable* hist);
char* scrollDown(historyTable* hist, int* position);

void newEntry(historyTable* hist, char* currentCommand);
void resetCurrentIndex(historyTable* hist);

void printHistoryTable(historyTable* hist);

void expandTable(historyTable* hist, int size);

#endif
