#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "bash_func.h"


void ignore_handler(int sig) {

}


int main() {	
	struct Command* commands = NULL;     
	struct Job* jobList = NULL;
	struct History* historyList = NULL;
	struct NodeTracker* nodeTracker = NULL;	
	
	
	signal(SIGINT, ignore_handler);
	signal(SIGTSTP, ignore_handler);
	signal(SIGTERM, ignore_handler);
	signal(SIGQUIT, ignore_handler);
	signal(SIGSEGV, ignore_handler);
	signal(SIGTTIN, ignore_handler);
	
    while(1) {    
   		updateJobList(&jobList);
     	
     	pwd();
	    char* input = characterInput();
	    addToHistory(&historyList, input);
	    
	    if (input == "EOF") {
	    	clearerr(stdin);
	    	printf("CTRL+D handled\n");
	    	continue;
	    }
	    
	    if (input[0] == '\0') { // If press Enter, skip and continue new iteration
	    	free(input);
	    	continue;
	    }  else if (strcmp(input, "history") == 0) {
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

	    int wordCount;
	    char** words = splitStringWithoutSpaces(input, &wordCount);

	    int firstOperatorFlag = 0;
	    int secondOperatorFlag = 0;
	    commands = parseCommandsFromWords(words, wordCount, &firstOperatorFlag, &secondOperatorFlag);
	    
	    free(input);
		
		for (int i = 0; i < wordCount; i++) {
			free(words[i]);
		}
		
		free(words);

	    //printf("First operator's flag: %d\n", firstOperatorFlag);
	    //printf("Second operator's flag: %d\n", secondOperatorFlag);
	    
	    if (strcmp(commands->words[0], "cd") == 0) {
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
		} else if (strcmp(commands->words[0], "kill") == 0) {
			if (commands->next != NULL) {
				struct Command* tmp = commands; // create temporary node for first word: 'kill'
				commands = commands->next;
				freeCmd(tmp);
				
				if (strcmp(commands->words[0], "-SIGKILL") == 0 && commands->next != NULL) {
					char* identifier[2] = {commands->words[0], commands->next->words[0]};
					killProcessByIdentifier(&jobList, identifier);
				} else if (strcmp(commands->words[0], "-SIGSTOP") == 0 && commands->next != NULL) {
					char* identifier[2] = {commands->words[0], commands->next->words[0]};
					killProcessByIdentifier(&jobList, identifier); 
				} else if (strcmp(commands->words[0], "-SIGCONT") == 0 && commands->next != NULL) {
					char* identifier[2] = {commands->words[0], commands->next->words[0]};
					killProcessByIdentifier(&jobList, identifier); 
				} else if (strcmp(commands->words[0], "-SIGINT") == 0 && commands->next != NULL) {
					char* identifier[2] = {commands->words[0], commands->next->words[0]};
					killProcessByIdentifier(&jobList, identifier);
				} else if (strcmp(commands->words[0], "-SIGTERM") == 0 && commands->next != NULL) {
					char* identifier[2] = {commands->words[0], commands->next->words[0]};
					killProcessByIdentifier(&jobList, identifier);
				} else if (strcmp(commands->words[0], "-SIGHUP") == 0 && commands->next != NULL) {
					char* identifier[2] = {commands->words[0], commands->next->words[0]};
					killProcessByIdentifier(&jobList, identifier);
				} else {
					char* identifier[1] = {commands->words[0]};
					killProcessByIdentifier(&jobList, identifier);
				}
				
				freeCommand(&commands);  // clear full Command list: 'kill -flag job_pid'		
			} else {
				perror("kill");
			}
		 } else if (strcmp(commands->words[0], "jobs") == 0) { 
			printJobs(jobList);
		} else if (strcmp(commands->words[0], "fg") == 0) {
			if (commands->next != NULL) bringToForeground(&jobList, commands->next->words[0]); 
			else bringToForeground(&jobList, NULL);
		} else if (strcmp(commands->words[0], "bg") == 0) {
			if (commands->next != NULL) resumeInBackground(&jobList, commands->next->words[0]);
			else resumeInBackground(&jobList, NULL);
		} else if (strcmp(commands->words[0], "sleep") == 0) {
			//printf("cmd is %s\n", commands->next->words[0]);
			commands = commands->next;
			//printf("cmd flag is %d\n", commands->flag);
			int isBackground = (commands->next != NULL && commands->flag == 2);
			//printf("Back is %d\n", isBackground);
			
			if (commands != NULL && commands->words[0] != NULL) {
				if (isBackground) {
					//printf("background\n");
					sleepCustom(atoi(commands->words[0]), commands->flag, &jobList);  // Background
				} else {
					sleepCustom(atoi(commands->words[0]), 0, &jobList); // Foreground
				}
			} else {
				perror("Invalid sleep command");
			}
		} else if (strcmp(commands->words[0], "wait") == 0) {
			if (commands->next != NULL && commands->next->words[0] != NULL) {
				pid_t waitPid = atoi(commands->next->words[0]);
				
				waitProcess(&jobList, waitPid);
			} else {
				perror("Invalid wait command. Please provide a valid PID.");
			}
		} else {
			executeCommand(commands, &jobList, firstOperatorFlag, secondOperatorFlag, historyList);
			
		}
		
		freeCommand(&commands);
	}
    
    while (historyList != NULL) {
    	struct History* temp = historyList;
    	historyList = historyList->next;
    	
    	free(temp->command);
    	free(temp);
    }
  
   clearJobs(&jobList);
   freeCommand(&commands);
   freeHistory(historyList);
				

    return 0;
}

