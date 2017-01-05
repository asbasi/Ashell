#include "token.h"
#include <string.h>
#include <stdlib.h>

tokenizer* createTokenizer(char* cmd)
{
    tokenizer* tok = (tokenizer*)malloc(sizeof(tokenizer));

    tok->tokArr = (char**)malloc(sizeof(char*) * INIT_NUM_POSSIBLE);

    for(int i = 0; i < INIT_NUM_POSSIBLE; i++)
    {
        tok->tokArr[i] = (char*)malloc(sizeof(char) * INIT_TOK_SIZE);
    }

    tok->tokSize = INIT_TOK_SIZE;
    tok->totalPossible = INIT_NUM_POSSIBLE;

    tok->currentTok = 0;
    tok->numStored = 0;

    int length = strlen(cmd);
    bool takeNext = false;
    bool initialSpace = true;
    int tokLength = 0;

    for(int i = 0; i < length; i++)
    {
        if(takeNext)
        {
            tok->tokArr[tok->numStored][tokLength++] = cmd[i];
            expandTokenSize(tok, tokLength); // Expand if needed.
            takeNext = false;
        }
        else if(cmd[i] == ' ' || cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|')
        {
            if(initialSpace && cmd[i] == ' ')
            {
                continue;
            }
            else
            {
                if(tokLength > 0)
                {
                    tok->tokArr[tok->numStored][tokLength] = '\0';
                    tokLength = 0;
                    (tok->numStored)++;
                    expandTotalPossible(tok); // Expand if needed.
                }

                initialSpace = true;

                if(cmd[i] == '<' || cmd[i] == '>' || cmd[i] == '|')
                {
                    tok->tokArr[tok->numStored][tokLength++] = cmd[i];
                    expandTokenSize(tok, tokLength); // Expand if needed.

                    tok->tokArr[tok->numStored][tokLength] = '\0';

                    (tok->numStored)++;
                    tokLength = 0;

                    expandTotalPossible(tok);
                }
            }
        }
        else if(cmd[i] == '\\')
        {
            initialSpace = false;
            takeNext = true;
        }
        else
        {
            initialSpace = false;
            tok->tokArr[tok->numStored][tokLength++] = cmd[i];
            expandTokenSize(tok, tokLength); // Expands if needed.
        }
    }

    if(tokLength > 0)
    {
        tok->tokArr[tok->numStored][tokLength] = '\0';
        (tok->numStored)++;
    }

    return tok;
}

void destroyTokenizer(tokenizer* tok)
{
    for(int i = 0; i < tok->totalPossible; i++)
    {
        free(tok->tokArr[i]);
    }

    free(tok->tokArr);

    free(tok);
}

char* nextToken(tokenizer* tok)
{
    if(tok->currentTok < tok->numStored)
    {
        (tok->currentTok)++;
        return tok->tokArr[tok->currentTok - 1];
    }
    else
    {
        return NULL;
    }
}

void expandTokenSize(tokenizer* tok, int sLength)
{
    if(sLength == tok->tokSize - 1)
    {
        tok->tokSize *= 2;
        for(int i = 0; i < tok->totalPossible; i++)
        {
            tok->tokArr[i] = (char*)realloc(tok->tokArr[i], tok->tokSize);
        }
    }
}

void expandTotalPossible(tokenizer* tok)
{
    if(tok->numStored == tok->totalPossible - 1)
    {
        tok->totalPossible *= 2;
        tok->tokArr = (char**)realloc(tok->tokArr, tok->totalPossible);
        for(int i = tok->totalPossible / 2; i < tok->totalPossible; i++)
        {
             tok->tokArr[i] = (char*)malloc(sizeof(char) *  tok->tokSize);
        }
    }
}

void resetCurrentToken(tokenizer* tok)
{
    tok->currentTok = 0;
}
