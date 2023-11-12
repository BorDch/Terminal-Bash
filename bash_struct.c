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
	    } else if (strcmp(commands->words[0], "cd") == 0) {
	    	if (wordCount == 1) {
	    		cd(NULL);
	    	} else {
	    		commands = commands->next;
	    		cd(commands->words[0]);
	    	}
	    } else if (strcmp(commands->words[0], "echo") == 0) {
	    	commands = commands->next;
	    	echo(commands);
	    } else if (strcmp(commands->words[0], "help") == 0) {
	    	help();
	    } else {
	    	executeCommand(commands, firstOperatorFlag);
	    }
	     
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
