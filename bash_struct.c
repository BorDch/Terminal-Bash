#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bash_func.h"

int main() {	     
	struct Job* jobList = NULL;
	
     while(1) {	    
     	updateJobList(&jobList);
     
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
	    } else if (strcmp(commands->words[0], "rm") == 0) {
	    	commands = commands->next;
	    	removeFile(commands);
	    } else if (strcmp(commands->words[0], "touch") == 0) {
	    	struct Command* temp = commands;
	    	
	    	do {
	    		temp = temp->next;
	    		if (temp != NULL) {
	    			temp->filename = temp->words[0];
	    			touch(temp->filename);
	    		}
	    	} while (temp != NULL); 
	    } else if(strcmp(commands->words[0], "cat") == 0 && commands->flag == 0 && firstOperatorFlag == 0) {
	    	struct Command* temp = commands;
			temp = temp->next;

			if (temp == NULL) {
				cat(NULL);
			} else {
				while (temp != NULL) {
					temp->filename = temp->words[0];
					cat(temp->filename);
					temp = temp->next;
				}
			}
		} else if (strcmp(commands->words[0], "jobs") == 0) { 
			printJobs(jobList);
		} else {
			executeCommand(commands, firstOperatorFlag, &jobList);
		} 
	     
	    free(input);
	    free(words);
	    struct Command* cmd = commands;
	    while (cmd != NULL) {
		struct Command* tmp = cmd;
		cmd = cmd->next;
		free(tmp);
	    }
    }

    return 0;
}

