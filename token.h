#ifndef TOKEN_H
#define TOKEN_H

typedef int bool;
#define true 1
#define false 0

#define INIT_NUM_POSSIBLE 100
#define INIT_TOK_SIZE 100

typedef struct
{
    int currentTok;
    int numStored;

    char** tokArr;

    int totalPossible;
    int tokSize;
} tokenizer;

tokenizer* createTokenizer(char* cmd);
void destroyTokenizer(tokenizer* tok);

char* nextToken(tokenizer* tok);

void expandTokenSize(tokenizer* tok, int sLength);
void expandTotalPossible(tokenizer* tok);

void resetCurrentToken(tokenizer* tok);

#endif
