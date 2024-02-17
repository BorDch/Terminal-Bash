#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "bash_func.h"


int main() {
	struct Command* commands = NULL;
	struct Job* jobList = NULL;
	struct History* historyList = NULL;
	
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

    while (1) {
    	    updateJobList(&jobList);
    	    pwd();
	    
	    char* input = characterInput();
	    if (feof(stdin)) {
	    	printf("CTRL+D handled\n");
		free(input);
	    	break;

	    }
	    
	    if (input[0] == '\0') { // If press Enter, skip and continue new iteration
	    	free(input);
	    	continue;
	    } else if (strcmp(input, "history") == 0) {
	    	printHistory(historyList);
	    	free(input);
	    	continue;
	    } else if (strcmp(input, "history -c") == 0) {
	    	clearHistory(&historyList);
	    	free(input);
	    	continue;
	    } else if (strcmp(input, "exit") == 0) {
	    	free(input);
	    	break;
	    }
	    
	    addToHistory(&historyList, input);
	
	    int wordCount;
	    char** words = splitStringWithoutSpaces(input, &wordCount);  
            
	    int firstOperatorFlag = 0;
	    int secondOperatorFlag = 0;
	    
	    commands = parseCommandsFromWords(words, wordCount, &firstOperatorFlag, &secondOperatorFlag);
	    
	    
	    if (strcmp(commands->words[0], "cd") == 0) {
	    	if (wordCount == 1) {
	    		cd(NULL);
	   
	    	} else cd(commands->words[1]);
	    	
	    } else if (strcmp(commands->words[0], "echo") == 0) {
	    	echo(commands->words[1]);
	    
	    } else if (strcmp(commands->words[0], "help") == 0) {
	    	help();
	    
	    } else if (strcmp(commands->words[0], "jobs") == 0) {
	    	if (jobList != NULL) {
	    		printJobs(jobList);
	    	} else {
	    		printf("No jobs\n");
	    	}
	    } else if (strcmp(commands->words[0], "fg") == 0) {
	    	char* identifier = NULL;
	    	if (commands->words[1] != NULL) {
	    		identifier = commands->words[1];
	    	}
	    	
	    	bringToForeground(&jobList, identifier);
	    
	    } else if (strcmp(commands->words[0], "bg") == 0) {
	    	if (commands->words[1] == NULL) {
	    		resumeInBackground(&jobList, NULL);
	    	} else {
	    		resumeInBackground(&jobList, commands->words[1]);
	    	}
	    } else if (strcmp(commands->words[0], "kill") == 0) {
	    	char* identifier[2] = {commands->words[1], commands->words[2]};
		killProcessByIdentifier(commands, &jobList, identifier);
		
		freeCommand(&commands);
	    } else if (strcmp(commands->words[0], "wait") == 0) {
	    	if (commands->words[1] != NULL) {
	    		pid_t waitPid = atoi(commands->words[1]);
				
			waitProcess(&jobList, waitPid);
		} else {
			perror("Invalid wait command. Please provide a valid PID.");
		}
	    
	    } else {
	    	executeCommand(commands, &jobList, &historyList, firstOperatorFlag, secondOperatorFlag);
	    }
	    
	    freeCommand(&commands);
	    free(input);
	    for (int i = 0; i < wordCount; i++) {
	    	free(words[i]);
	    }
	   	
	    free(words);
    }
    
    while (historyList != NULL) {
    	struct History* temp = historyList;
    	historyList = historyList->next;
     
    	free(temp->command);
    	free(temp);
    }
    
    if (jobList != NULL) {
    	clearJobs(&jobList);
    }
    
    if (commands != NULL && jobList != NULL) {
    	freeCommand(&commands);
    	clearJobs(&jobList);
    }
    freeHistory(historyList);

    return 0;
}
