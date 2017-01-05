#include "history.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Creates a new history table.
historyTable* createHistoryTable()
{
    // Allocate space for the table.
    historyTable* hist = (historyTable*)malloc(sizeof(historyTable));

    // Allocate space for the the array of previous commands.
    hist->previousCommands = (char**)malloc(sizeof(char*) * MAX_NUM_ENTRIES);

    // Go through and allocate space for each individual index in array of commands.
    for(int i = 0; i < MAX_NUM_ENTRIES; i++)
    {
        hist->previousCommands[i] = (char*)calloc(INIT_COMMAND_SIZE, sizeof(char));
    }

    hist->currentIndex = -1;
    hist->latestIndex = -1;
    hist->lastIndex = -1;

    return hist;
}

// Deallocates the history table.
void destroyHistoryTable(historyTable* hist)
{
    for(int i = 0; i < MAX_NUM_ENTRIES; i++)
    {
        free(hist->previousCommands[i]);
    }

    free(hist->previousCommands);

    free(hist);
}

// Scrolls up and gets the entry at that index.
char* scrollUp(historyTable* hist)
{
    if(hist->currentIndex == hist->lastIndex)
    {
        // output audible bell character.
        write(STDOUT_FILENO, "\a", 1);

        return NULL;
    }

    if(hist->currentIndex == -1)
    {
        hist->currentIndex = hist->latestIndex;
    }
    else
    {
        hist->currentIndex = (hist->currentIndex - 1 + MAX_NUM_ENTRIES) % MAX_NUM_ENTRIES;
    }

    return hist->previousCommands[hist->currentIndex];
}

// Scrolls down and gets the entry at that index.
char* scrollDown(historyTable* hist, int* position)
{
    if(hist->currentIndex == -1)
    {
        // output audible bell character.
       	write(STDOUT_FILENO, "\a", 1);

       	return NULL;
    }

    if(hist->currentIndex == hist->latestIndex)
    {
        hist->currentIndex = -1;

        for(int i = 0; i < *position; i++)
            write(STDOUT_FILENO, "\b \b", strlen("\b \b"));

        *position = 0;

        return NULL;
    }

    hist->currentIndex = (hist->currentIndex + 1) % MAX_NUM_ENTRIES;

    return hist->previousCommands[hist->currentIndex];
}

void newEntry(historyTable* hist, char* currentCommand)
{
    if(hist->latestIndex == -1 && hist->lastIndex == -1)
    {
        hist->latestIndex = 0;
        hist->lastIndex = 0;
        strcpy(hist->previousCommands[0], currentCommand);
    }
    else
    {
        hist->latestIndex = (hist->latestIndex + 1) % MAX_NUM_ENTRIES;
        strcpy(hist->previousCommands[hist->latestIndex], currentCommand);

        if(hist->latestIndex == hist->lastIndex)
        {
           hist->lastIndex = (hist->lastIndex + 1) % MAX_NUM_ENTRIES;
        }
    }
}

void resetCurrentIndex(historyTable* hist)
{
    hist->currentIndex = -1;
}

// Prints the entire history table (Used for debugging)
void printHistoryTable(historyTable* hist)
{
    for(int i = 0; i < MAX_NUM_ENTRIES; i++)
    {
        write(STDOUT_FILENO, hist->previousCommands[i], strlen(hist->previousCommands[i]));
        write(STDOUT_FILENO, "\n", 1);
    }
}

// Expands the history table to accomodate larger commands.
void expandTable(historyTable* hist, int size)
{
    for(int i = 0; i < MAX_NUM_ENTRIES; i++)
    {
        hist->previousCommands[i] = (char*)realloc(hist->previousCommands[i], size);
    }
}

