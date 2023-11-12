#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bash_func.h"


int main() {
     while(1) {
     	    pwd();
	    char* input = characterInput();

	    int wordCount;
	    char** words = splitStringWithoutSpaces(input, &wordCount);

	    int firstOperatorFlag = 0; // Переменная для хранения флага первого оператора
	    struct Command* commands = parseCommandsFromWords(words, wordCount, &firstOperatorFlag);

	    // Выводим флаг первого оператора
	    //printf("First operator's flag: %d\n", firstOperatorFlag);
	    
	    
	    
	    if (strcmp(commands->words[0], "exit") == 0) {
	    	free(input);
	    	free(words);
	    	struct Command* cmd = commands;
	    	while (cmd != NULL) {
	        	struct Command* temp = cmd;
	        	cmd = cmd->next;
	        	free(temp);
	    	}
	    	break;
	    } 
	    
	    executeCommand(commands, firstOperatorFlag);
	     
	    free(input);
	    free(words);
	    struct Command* cmd = commands;
	    while (cmd != NULL) {
		struct Command* temp = cmd;
		cmd = cmd->next;
		free(temp);
	    }
    }

    return 0;
}
