#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "history.h"
#include "token.h"
#include "command.h"

#define INIT_CWD_SIZE 1000
#define MAX_PATH_DISPLAYED 16
#define PREPEND_STRING "/..."

bool getCommand(char** cmd, int* length, int* cmd_size, historyTable* hist);
void clearLine(int length);
void printCorrectCWD(char* cwd);
void ResetCanonicalMode(int fd, struct termios *savedattributes);
void SetNonCanonicalMode(int fd, struct termios *savedattributes);

int main()
{
    struct termios SavedTermAttributes;

    char** cmd = (char**)malloc(sizeof(char*));
    (*cmd) = (char*)malloc(sizeof(char) * INIT_COMMAND_SIZE);

    int cmd_len = 0;
    int cmd_size = INIT_COMMAND_SIZE;

    char* cwd = (char*)calloc(INIT_CWD_SIZE, sizeof(char));
    int cwd_size = INIT_CWD_SIZE;

    historyTable* hist = createHistoryTable();

    tokenizer* tokens;
    char* tok;

    const pid_t parentPID = getpid(); // Get process id of the parent.

    SetNonCanonicalMode(STDIN_FILENO, &SavedTermAttributes); // From Nitta's code

    while(getpid() == parentPID) // Check that we're the parent process.
    {
        // Get current directory, dynammically expand cwd as needed.
        while(getcwd(cwd, cwd_size) == NULL && errno == ERANGE)
        {
            cwd_size *= 2;
            cwd = (char*)realloc(cwd, cwd_size);
        }

        printCorrectCWD(cwd);

        // Obtains/Prints user's command.
        if(getCommand(cmd, &cmd_len, &cmd_size, hist))
        {
            write(STDOUT_FILENO, "\r\n", 2);

            tokens = createTokenizer(*cmd);

            if((tok = nextToken(tokens)) != NULL)
            {
                if(strcmp(tok, "exit") == 0) // Exits shell.
                {
                    destroyTokenizer(tokens);
                    break;
                }
                else if(strcmp(tok, "cd") == 0) // Changes current working directory.
                {
                    cd_command(nextToken(tokens));
                }
                else // Execute command.
                {
                    execute_command(tokens);
                }
           }

           destroyTokenizer(tokens);
        }
        else // The user pressed the C-d key so end program.
        {
            break;
        }
    }

    if(getpid() == parentPID)
        ResetCanonicalMode(STDIN_FILENO, &SavedTermAttributes); // From Nitta's code

    destroyHistoryTable(hist);

    free(cwd);

    free(*cmd);
    free(cmd);

    return 0;
}

// Get the entire command from the user.
bool getCommand(char** cmd, int* length, int* cmd_size, historyTable* hist)
{
    bool vFlag = true; // Used to make sure the command is valid (i.e. not C-d).
    char curr;
    int index = 0;

    char* temp = (char*)malloc(sizeof(char) * (*cmd_size));

    while(1)
    {
        // Need to expand the cmd & history table.
        if(index == *cmd_size)
        {
            (*cmd_size) *= 2; // Double the size.

            (*cmd) = (char*)realloc(*cmd, *cmd_size); // Expand cmd.
            temp = (char*)realloc(temp, *cmd_size);

            expandTable(hist, *cmd_size); // Expand history table.
        }

        read(STDIN_FILENO, &curr, 1);
        if(curr == 0x09) // Tab pressed
        {
            continue;
        }
        else if(curr == '\n') // Enter key pressed.
        {
            resetCurrentIndex(hist);
            (*cmd)[index] = '\0';

            // Only add entry if we actually got some input.
            if(index != 0)
                newEntry(hist, *cmd);
            break;
        }
        else if(curr == 0x04) // C-d pressed
        {
            vFlag = false;
            break;
        }
        else if(curr == 0x1B) // Esc character so it's potential Arrow key.
        {
            // Note that the arrow keys are a sequence of characters
            // in the form 0x1B (esc), 0x58 ('['), and either 'A' or 'B'.
            // 'A' is Up, 'B' is Down.

            read(STDIN_FILENO, &curr, 1);
            if(curr == 0x5B) // '[' character
            {
                read(STDIN_FILENO, &curr, 1);
                if(curr == 'A') // Up arrow.
                {
		    strcpy(temp, scrollUp(hist));
                    if(temp[0] != '\0')
                    {
                        clearLine(index);
                    	strcpy(*cmd, temp);
                    	index = strlen(*cmd);
                    	write(STDOUT_FILENO, *cmd, strlen(*cmd));
                    }
                }
                else if(curr == 'B') // Down arrow
                {
                    strcpy(temp, scrollDown(hist, &index));
                    if(temp[0] != '\0')
                    {
                        clearLine(index);
                        strcpy(*cmd, temp);
                        index = strlen(*cmd);
                        write(STDOUT_FILENO, *cmd, strlen(*cmd));
                    }

                }
                else if(curr == '3') // potentially delete key
                {
                    read(STDIN_FILENO, &curr, 1);
                    if(curr == '~') // delete key
                    {
                        if(index > 0) // Valid index.
                        {
                            write(STDOUT_FILENO, "\b \b", strlen("\b \b"));
                            index--;
                        }
                        else // delete when no characters exist.
                        {
                            // output audible bell character.
                            write(STDOUT_FILENO, "\a", 1);
                        }
                    }
                }
            }

        }
        else if(curr == 0x7F || curr == 0x08 || curr == '\b') // Backspace key.
        {
            if(index > 0) // Valid index.
            {
                write(STDOUT_FILENO, "\b \b", strlen("\b \b"));
                index--;
            }
            else // Backspace when no characters exist.
            {
                // output audible bell character.
                write(STDOUT_FILENO, "\a", 1);
            }
        }
        else
        {
            write(STDOUT_FILENO, &curr, 1);
            (*cmd)[index++] = curr;
        }

    }

    free(temp);
    *length = strlen(*cmd);
    return vFlag;
}

// Clears the line.
void clearLine(int length)
{
    for(int i = 0; i < length; i++)
    {
        write(STDOUT_FILENO, "\b \b", strlen("\b \b"));
    }
}

// Prints the correct path, shortened as necessary.
void printCorrectCWD(char* cwd)
{
    int sLength = strlen(cwd);

    if(sLength > MAX_PATH_DISPLAYED) // Longer than 16 characters.
    {
        while(cwd[sLength] != '/')
            sLength--;

        write(STDOUT_FILENO, PREPEND_STRING, strlen(PREPEND_STRING));
        write(STDOUT_FILENO, &(cwd[sLength]), strlen(&(cwd[sLength])));
    }
    else
    {
        write(STDOUT_FILENO, cwd, strlen(cwd));
    }
    write(STDOUT_FILENO, "% ", 2);
}

// NOTE: This is from Prof. Nitta's example code (called "noncanmode.c")
void ResetCanonicalMode(int fd, struct termios *savedattributes)
{
    tcsetattr(fd, TCSANOW, savedattributes);
}

// NOTE: This is from Prof. Nitta's example code (called "noncanmode.c")
void SetNonCanonicalMode(int fd, struct termios *savedattributes)
{
    struct termios TermAttributes;

    // Make sure stdin is a terminal.
    if(!isatty(fd))
    {
        write(STDERR_FILENO, "Not a terminal.\r\n", strlen("Not a terminal.\r\n"));
        exit(0);
    }

    // Save the terminal attributes so we can restore them later.
    tcgetattr(fd, savedattributes);

    // Set the funny terminal modes.
    tcgetattr (fd, &TermAttributes);
    TermAttributes.c_lflag &= ~(ICANON | ECHO); // Clear ICANON and ECHO.
    TermAttributes.c_cc[VMIN] = 1;
    TermAttributes.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &TermAttributes);
}
