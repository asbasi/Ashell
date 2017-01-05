#include "command.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

void cd_command(char* path)
{
    int retVal;

    if(path == NULL)
    {
        path = getenv("HOME");
        if(path == NULL)
        {
            write(STDOUT_FILENO, "Error changing directory.", strlen("Error changing directory."));
            write(STDOUT_FILENO, "\r\n", 2);
            return;
        }
    }

    retVal = chdir(path);

    if(retVal != 0)
    {
        write(STDOUT_FILENO, "Error changing directory.", strlen("Error changing directory."));
        write(STDOUT_FILENO, "\r\n", 2);
    }
}

void execute_command(tokenizer* tok)
{
    char* currTok;
    char* arguments[tok->totalPossible + 1]; // Holds the arguments for a command.

    pid_t pid;

    int fd_in = 0;

    bool previousPipe = false;

    int inFile = 0;
    int outFile = 0;

    int argIndex = 0;
    int commandCount = 0;
    int status;

    resetCurrentToken(tok);

    int fds[2];

    currTok = nextToken(tok);
    while(currTok != NULL)
    {
        argIndex = 0;
        commandCount++;
        pipe(fds);
        if((pid = fork()) == 0) // Child
        {
            while(currTok != NULL && currTok[0] != '|') // Take care of redirections.
            {
                // Input redirection.
                if(currTok[0] == '<'  && (currTok = nextToken(tok)) != NULL && currTok[0] != '|')
                {
                    // currTok should now hold filename.
                    if(inFile != 0)
                    {
                        close(inFile);
                        inFile = 0;
                    }

                    inFile = open(currTok, O_RDONLY);
                    if(inFile == -1)
                    {
                        write(1, "File \"", strlen("File \""));
                        write(1, currTok, strlen(currTok));
                        write(1, "\" does not exist!", strlen("\" does not exist!"));
                        write(1, "\r\n", 2);

                        close(fds[0]);
                        close(fds[1]);
                        return;
                    }
                }
                // Output redirection.
                else if(currTok[0] == '>' && (currTok = nextToken(tok)) != NULL && currTok[0] != '|')
                {
                    // currTok should now hold filename.
                    if(outFile != 0)
                    {
                        close(outFile);
                        outFile = 0;
                    }
                    outFile = open(currTok, O_WRONLY | O_CREAT, S_IRUSR  | S_IRGRP | S_IWUSR | S_IWGRP);

                    if(outFile == -1)
                    {
                        outFile = 0;
                    }
                }
                else
                {
                    arguments[argIndex++] = currTok;
                }

                if(currTok != NULL && currTok[0] != '|')
                {
                    currTok = nextToken(tok);
                }
            }

            arguments[argIndex] = NULL;
            // At this point outFile and inFile have been set to their respective values.

            // Input handling.
            if(inFile != 0)
            {
                dup2(inFile, 0);
                close(inFile);
            }
            if(previousPipe)
            {
                dup2(fd_in, 0);
                close(fd_in);
            }
            close(fds[0]);

            // Output handling.
            if(outFile != 0)
            {
                dup2(outFile, 1);
                close(outFile);
            }
            if(currTok != NULL && currTok[0] == '|')
            {
                dup2(fds[1], 1);
            }
            close(fds[1]);

            // Now we just have to handle the command.
            handle_commands(arguments);
            return;
        }
        while(currTok != NULL && currTok[0] != '|')
            currTok = nextToken(tok);

        if(currTok != NULL && currTok[0] == '|')
        {
            previousPipe = true;
            currTok = nextToken(tok);
        }

        if(fd_in != 0)
            close(fd_in);

        fd_in = fds[0];
        close(fds[1]);
    }

    if(fd_in != 0)
        close(fd_in);

    // Parent

    // Wait for your children to get done.
    for(int i = 0; i < commandCount; i++)
    {
        wait(&status);
    }
}

void handle_commands(char** arguments)
{
    if(arguments[0] != NULL) // Some argument.
    {
        if(strcmp(arguments[0], "ls") == 0)
        {
            ls_command(arguments);
        }
        else if(strcmp(arguments[0], "pwd") == 0)
        {
            pwd_command();
        }
        else if(strcmp(arguments[0], "ff") == 0)
        {
            ff_command(arguments);
        }
        else
        {
           if(execvp(arguments[0], arguments) == -1)
           {
               write(1, "Failed to execute ", strlen("Failed to execute "));
               write(1, arguments[0], strlen(arguments[0]));
               write(1, "\r\n", strlen("\r\n"));
           }
        }
    }
}

void ls_command(char** args)
{
    DIR *dir;
    struct dirent* entry;
    struct stat eStat;

    int path_size = 1000;
    char* path = (char*)malloc((sizeof(char)) * path_size);
    char* relativePath;

    if(args[1] == NULL) // No directory provided.
    {
        // Get current directory, dynammically expand cwd as needed.
        while(getcwd(path, path_size) == NULL && errno == ERANGE)
        {
            path_size *= 2;
            path = (char*)realloc(path, path_size);
        }
    }
    else
    {
        free(path);
        path = args[1];
    }

    if((dir = opendir(path)) == NULL)
    {
        write(1, "Failed to open directory \"", strlen("Failed to open directory \""));
        write(1, path, strlen(path));
        write(1, "/\"\r\n", strlen("/\"\r\n"));
    }
    else
    {
        while((entry = readdir(dir)) != NULL)
        {
            relativePath = (char*)calloc((strlen(path) + strlen(entry->d_name) + strlen("/") + 1), sizeof(char));
            strcpy(relativePath, path);
            strcat(relativePath, "/");
            strcat(relativePath, entry->d_name);
            stat(relativePath, &eStat);
            free(relativePath);


            write(1, ((S_ISDIR(eStat.st_mode)) ? "d" : "-"), 1);
            write(1, ((eStat.st_mode & S_IRUSR) ? "r" : "-"), 1);
	    write(1, ((eStat.st_mode & S_IWUSR) ? "w" : "-"), 1);
            write(1, ((eStat.st_mode & S_IXUSR) ? "x" : "-"), 1);
            write(1, ((eStat.st_mode & S_IRGRP) ? "r" : "-"), 1);
            write(1, ((eStat.st_mode & S_IWGRP) ? "w" : "-"), 1);
            write(1, ((eStat.st_mode & S_IXGRP) ? "x" : "-"), 1);
            write(1, ((eStat.st_mode & S_IROTH) ? "r" : "-"), 1);
            write(1, ((eStat.st_mode & S_IWOTH) ? "w" : "-"), 1);
            write(1, ((eStat.st_mode & S_IXOTH) ? "x" : "-"), 1);
            write(1, " ", 1);
            write(1, entry->d_name, strlen(entry->d_name));
            write(1, "\r\n", 2);
        }
    }

    closedir(dir);

    if(args[1] == NULL)
        free(path);
}

void pwd_command()
{
    int cwd_size = 1000;
    char* cwd = (char*)malloc((sizeof(char)) * cwd_size);

    // Get current directory, dynammically expand cwd as needed.
    while(getcwd(cwd, cwd_size) == NULL && errno == ERANGE)
    {
        cwd_size *= 2;
        cwd = (char*)realloc(cwd, cwd_size);
    }

    write(1, cwd, strlen(cwd));
    write(1, "\r\n", 2);

    free(cwd);
}

void ff_command(char** args)
{
    int path_size = 1000;
    char* path;

    char* prefix;

    if(args[1] == NULL) // No filename given.
    {
        write(1, "ff command requires a filename!\r\n", strlen("ff command requires a filename!\r\n"));
        return;
    }
    else if(args[2] == NULL) // No directory given.
    {
        path = (char*)malloc((sizeof(char)) * path_size);

        // Get current directory, dynammically expand cwd as needed.
        while(getcwd(path, path_size) == NULL && errno == ERANGE)
        {
            path_size *= 2;
            path = (char*)realloc(path, path_size);
       	}

        prefix = (char*)malloc(sizeof(char) * strlen(".") + 1);
        strcpy(prefix, ".");
    }
    else
    {
        path = (char*)malloc(sizeof(char) * strlen(args[2]) + 1);
        strcpy(path, args[2]);

        prefix = (char*)malloc(sizeof(char) * strlen(path) + 1);
        strcpy(prefix, path);
    }

    ff_helper(prefix, path, args[1]);

    free(prefix);
    free(path);
}

void ff_helper(char* prefix, char* path, char* filename)
{
    DIR *dir;
    struct dirent* entry;
    struct stat eStat;

    char* newpath;
    char* newprefix;

    char* relativePath;

    if((dir = opendir(path)) == NULL)
    {
        write(1, "Failed to open directory \"", strlen("Failed to open directory \""));
        write(1, path, strlen(path));
        write(1, "/\"\r\n", strlen("/\"\r\n"));
    }
    else
    {
        while((entry = readdir(dir)) != NULL)
        {
            relativePath = (char*)calloc((strlen(path) + strlen(entry->d_name) + strlen("/") + 1), sizeof(char));
            strcpy(relativePath, path);
            strcat(relativePath, "/");
            strcat(relativePath, entry->d_name);
            stat(relativePath, &eStat);
            free(relativePath);

            if(S_ISDIR(eStat.st_mode) && strncmp(entry->d_name, ".", 1) != 0 && strcmp(entry->d_name, "..") != 0) // It's a directory.
            {
                // Update path & prefix.
                newpath = (char*)calloc((strlen(path) + strlen(entry->d_name) + strlen("/") + 1), sizeof(char));
                strcpy(newpath, path);
                strcat(newpath, "/");
                strcat(newpath, entry->d_name);

                newprefix = (char*)calloc((strlen(prefix) + strlen(entry->d_name) + strlen("/") + 1), sizeof(char));
                strcpy(newprefix, prefix);
                strcat(newprefix, "/");
                strcat(newprefix, entry->d_name);

                ff_helper(newprefix, newpath, filename);

                free(newpath);
                free(newprefix);
            }
            else // Regular file.
            {
                if(strcmp(filename, entry->d_name) == 0) // Found file, yay!
                {
                    write(1, prefix, strlen(prefix));
                    write(1, "/", 1);
		    write(1, filename, strlen(filename));
		    write(1, "\r\n", strlen("\r\n"));
                }
            }
        }
        closedir(dir);
    }
}
