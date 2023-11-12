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

	    int firstOperatorFlag = 0;
	    struct Command* commands = parseCommandsFromWords(words, wordCount, &firstOperatorFlag);

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
