#ifndef COMMAND_H
#define COMMAND_H

#include "token.h"

void cd_command(char* path);
void ls_command(char** arguments);
void pwd_command();
void ff_command(char** arguments);
void execute_command(tokenizer* tok);
void handle_commands(char** arguments);
void ff_helper(char* prefix, char* path, char* filename);

#endif
