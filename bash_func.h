#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bash_func.c"


char* characterInput();
char** splitStringWithoutSpaces(char* str, int* wordCount);
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag);

void printAllWords(struct Command* commands);


void executeAndOperator(struct Command* cmd);
void executeOrOperator(struct Command* cmd);
void executePipeline(struct Command* cmd);
void executeSeqOperator(struct Command* cmd);
void executeCommand(struct Command* cmd, int firstOperatorFlag);
void pwd();
