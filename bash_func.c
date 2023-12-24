#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "jobs.c"
#include "history.c"

// Structure Command
struct Command {
    char** words;         // Command words
    int flag;             // Operator flag (0 - default, 1 - '|', 2 - '&', 3 - '||', 4 - '&&', .....)
    struct Command* next; // Next Command
    char* filename;       // for several functions
};


// Parser Section 


// Function for string input
char* characterInput() {
    int size = 10; // original size for string
    int length = 0; // current string length
    char* input = (char*)malloc(size * sizeof(char));
    
    if (input == NULL) {
        perror("Memory allocation");
        exit(1);
    }
    
    int c;
    while (1) {
        c = getchar();
        
        if (c == EOF) {
       		printf("EOF\n");
       		char* eof = "EOF";
       		free(input);
       		return eof;
        }

        if (c == '\n') {
            break;
        }

        input[length++] = (char)c;

        if (length >= size) {
            size *= 2; // expand size of string
            input = (char*)realloc(input, size * sizeof(char));
            if (input == NULL) {
                perror("Memory overlocation");
                exit(1);
            }
        }
    }

    input[length] = '\0';
    return input;
}


// Function for splitting a string and writing it to an array
char** splitStringWithoutSpaces(char* str, int* wordCount) {
    int wordBufferSize = 10;
    *wordCount = 0;
    char** words = (char**)malloc(wordBufferSize * sizeof(char*));
    
    if (words == NULL) {
        perror("Memory allocation");
        exit(1);
    }

    int i = 0;
    int start = -1;
    int inWord = 0;

    while (str[i] != '\0') {
        if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
            if (inWord) {
                // Find the space and start new word if it is word yet
                int length = i - start;
                if (*wordCount >= wordBufferSize) {
                    wordBufferSize *= 2;
                    words = (char**)realloc(words, wordBufferSize * sizeof(char*));
                    if (words == NULL) {
                        perror("Memory overlocation");
                        exit(1);
                    }
                }

                words[(*wordCount)] = (char*)malloc(length + 1);
                if (words[(*wordCount)] == NULL) {
                    perror("Memory allocation");
                    exit(1);
                }
                strncpy(words[(*wordCount)++], str + start, length);
                words[*wordCount - 1][length] = '\0';

                inWord = 0;
            }
        } else {
            if (!inWord) {
                // If it is not word, start new word
                start = i;
                inWord = 1;
            }
        }
        i++;
    }

    // Last word  handling
    if (inWord) {
        int length = i - start;
        if (*wordCount >= wordBufferSize) {
            wordBufferSize *= 2;
            words = (char**)realloc(words, wordBufferSize * sizeof(char*));
            if (words == NULL) {
                perror("Memory overlocation");
                exit(1);
            }
        }

        words[(*wordCount)] = (char*)malloc(length + 1);
        if (words[(*wordCount)] == NULL) {
            perror("Memory allocation");
            exit(1);
        }
        strncpy(words[(*wordCount)++], str + start, length);
        words[*wordCount - 1][length] = '\0';
    }

    return words;
}


void freeCommand(struct Command** cmd);

// Parser Function
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag, int* secondOperatorFlag) {
    struct Command* head = NULL;  // Command head
    struct Command* current = NULL;
    struct Command* lastWordCommand = NULL;
    int currentFlag = 0; // Current flag for operators
    
    for (int i = 0; i < wordCount; i++) {
        if (strcmp(words[i], "|") == 0) {
            *firstOperatorFlag = 1;
            break;
        } else if (strcmp(words[i], "&") == 0) {
            *firstOperatorFlag = 2;
            break;
        } else if (strcmp(words[i], "||") == 0) {
            *firstOperatorFlag = 3;
            break;
        } else if (strcmp(words[i], "&&") == 0) {
            *firstOperatorFlag = 4;
            break;
        } else if (strcmp(words[i], ";") == 0) {
            *firstOperatorFlag = 5;
            break;
        } else if (strcmp(words[i], ">") == 0) {
       	    *firstOperatorFlag = 6;
            break;
       	} else if (strcmp(words[i], ">>") == 0) {
       	    *firstOperatorFlag = 7;
            break;
        } else if (strcmp(words[i], "<") == 0) {
       	    *firstOperatorFlag = 8;
            break;
        }
    }

    for (int i = 0; i < wordCount; i++) {
        
        if (strcmp(words[i], "|") == 0) {
            currentFlag = 1;
            continue;
        } else if (strcmp(words[i], "&") == 0) {
            currentFlag = 2;
            continue;
        } else if (strcmp(words[i], "||") == 0) {
            currentFlag = 3;
          
            if (*firstOperatorFlag == 4 && *secondOperatorFlag == 0) {
            	*secondOperatorFlag = 3;
            }
            continue;
        } else if (strcmp(words[i], "&&") == 0) {
            currentFlag = 4;
            
            if (*firstOperatorFlag == 3 && *secondOperatorFlag == 0) {
            	*secondOperatorFlag = 4;
         	}
         	
         	continue;
        } else if (strcmp(words[i], ";") == 0) {
            currentFlag = 5;
			continue;
        } else if (strcmp(words[i], ">") == 0) {
        	currentFlag = 6;
        	continue;
        } else if (strcmp(words[i], ">>") == 0) {
        	currentFlag = 7;
        	continue;
        } else if (strcmp(words[i], "<") == 0) {
        	currentFlag = 8;
        	continue;
        }
        
        
        struct Command* cmd = (struct Command*)malloc(sizeof(struct Command));
        cmd->words = NULL;
        cmd->flag = 0;
        cmd->next = NULL;
        
        if (head == NULL) {
            head = cmd;
            current = cmd;
        } else {
            current->next = cmd;
            current = cmd;
        }
      
        int wordLength = strlen(words[i]);
        cmd->words = (char**)malloc(2 * sizeof(char*));
        if (cmd->words == NULL) {
            perror("Segmentation falut");
            freeCommand(&head);
          	return NULL;
        }
        cmd->words[0] = (char*)malloc(wordLength + 1);
        if (cmd->words[0] == NULL) {
            perror("Segmentation fault");
            freeCommand(&head);
          	return NULL;
        }
        strcpy(cmd->words[0], words[i]);
        cmd->words[1] = NULL;
        cmd->flag = currentFlag;
    
        // Connect last word with logical operator
        if (lastWordCommand != NULL) {
            lastWordCommand->next = cmd;
            lastWordCommand->flag = currentFlag;
        }
        // Remember last node
        lastWordCommand = cmd;
        lastWordCommand->flag = currentFlag;
        //printf("Last word: %s\n", lastWordCommand->words[0]);
        //printf("Parsed word: %s, Flag: %d\n", cmd->words[0], cmd->flag);
    } 
  
    return head;
}


// Two functions for cleaning memory of commands and a list of commands  
void freeCmd(struct Command* cmd) {
    struct Command* current = cmd;
    
    if (current != NULL) {
    	for (int i = 0; current->words[i] != NULL; i++) {
    		free(current->words[i]);
    	}
    	free(current->words);
    	free(current);
    }
}


void freeCommand(struct Command** cmd) {
    struct Command* current = *cmd;
    struct Command* next;

    while (current != NULL) {
        next = current->next;
        for (int i = 0; current->words[i] != NULL; i++) {
            free(current->words[i]);
        }
        free(current->words);
        free(current);

        current = next;
    }

    *cmd = NULL;
}
    

// To check the operation of the parser
void printAllWords(struct Command* commands) {
    struct Command* cmd = commands;

    while (cmd != NULL) {
        if (cmd->words != NULL) {
            for (int i = 0; cmd->words[i] != NULL; i++) {
                printf("%s ", cmd->words[i]);
            }
            printf("\n");
        }
        if (cmd->flag == 0) {
            printf("Flag: None\n");
        } else if (cmd->flag == 1) {
            printf("Flag: Pipe ('|')\n");
        } else if (cmd->flag == 2) {
            printf("Flag: Background ('&')\n");
        } else if (cmd->flag == 3) {
            printf("Flag: Logical OR ('||')\n");
        } else if (cmd->flag == 4) {
            printf("Flag: Logical AND ('&&')\n");
        } else if (cmd->flag == 5) {
            printf("Flag: (';')\n");
        }

        cmd = cmd->next;
    }
}


// Other Bash functions

// Function for operator '&&'
void executeAndOperator(struct Command* cmd) {
    int status = 0;
    while (cmd != NULL) { // Check for '&&'
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else {
            wait(&status);
            if (status != 0) {
                break;
            }
        }
        cmd = cmd->next;
    }
}

// Function for operator '||'
void executeOrOperator(struct Command* cmd) {
    int status = 0;
    while (cmd != NULL) { // Check for '||'
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else {
            wait(&status);
            if (status == 0) {
                break;
            }
        }
        cmd = cmd->next;
    }
}


// Function to execute a command sequence with '&&' and '||' operators
void executeCommandSequence(struct Command* cmd) {
    int status = 0;
    int success = 1;  // To track success of the previous command in the sequence

    while (cmd != NULL) {
        if (cmd->flag == 4) {  // '&&'
            if (success) {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(1);
                } else if (pid == 0) {
                    execvp(cmd->words[0], cmd->words);
                    perror("execvp");
                    exit(1);
                } else {
                    wait(&status);
                    success = (status == 0);
                }
            }
        } else if (cmd->flag == 3) {  // '||'
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                execvp(cmd->words[0], cmd->words);
                perror("execvp");
                exit(1);
            } else {
                wait(&status);
                if (status == 0) {
                    // Break out of the loop if '||' command succeeded
                    break;
                }
            }
        } else {  // Default, no operator
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                execvp(cmd->words[0], cmd->words);
                perror("execvp");
                exit(1);
            } else {
                wait(NULL);
            }
        }

        cmd = cmd->next;
    }
}


// Pipe Function
void executePipeline(struct Command* cmd) {
    int fd[2];
    int prev_fd = -1;

    while (cmd != NULL) {
        if (pipe(fd) == -1) {
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // Child process
            if (prev_fd != -1) {
                if (dup2(prev_fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(prev_fd);
            }

            if (cmd->next != NULL) {
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(fd[1]);
            }

            close(fd[0]);

            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else { // Parent process
            close(fd[1]);

            if (prev_fd != -1) {
                close(prev_fd);
            }

            wait(NULL);

            prev_fd = fd[0];
        }

        cmd = cmd->next;
    }
}


// Function for operator ';'
void executeSeqOperator(struct Command* cmd) {
    int status;
    while (cmd != NULL) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else {
            wait(&status);
        }
        cmd = cmd->next;
    }
}

/* Extra modifications
// Function for operator '&'
void executeInBackground(struct Command* cmd, struct Job** jobList) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Child process
        setpgid(0, 0);
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        
        pid_t pgid = getpgid(pid);
        printf("Process wth id [%d]\n", pid);
        addJob(jobList, createJob(pid, pgid, cmd->words[0], 0)); // Adding the job to the list
    }
}

*/

void executeInBackground(struct Command* cmd, struct Job** jobList) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Child process
        setpgid(0, 0);
        tcsetpgrp(STDIN_FILENO, getpgrp()); // Capture the terminal
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        pid_t pgid = getpgid(pid);
        printf("Process with id [%d]\n", pid);
        addJob(jobList, createJob(pid, pgid, cmd->words[0], 0)); // Adding the job to the list
        tcsetpgrp(STDIN_FILENO, getpgrp()); // Return access to the parent (bash)
    }
}


void signalHandler(int sig) { 
}

void executeDefault(struct Command* cmd, struct Job** jobList, struct History* historyList) { 
    signal(SIGTSTP, signalHandler);
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        freeCommand(&cmd);
        freeHistoryNode(historyList);
        exit(EXIT_FAILURE);
    } else if (pid == 0) { 
    	signal(SIGTSTP, signalHandler);
        //tcsetpgrp(STDIN_FILENO, getpgrp()); // Capture the terminal
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        freeCommand(&cmd);
        freeHistoryNode(historyList);
        exit(EXIT_FAILURE);
    } else {
        int status;
        setpgid(0, 0);
        
        waitpid(pid, &status, WUNTRACED);

        //tcsetpgrp(STDIN_FILENO, getpgrp()); // Return access to the parent (bash)

        if (WIFEXITED(status)) {
            // Process exited successfully
        } else if (WIFSTOPPED(status)) {
            // CTRL + Z pushed
            printf("\n[%d] %s Stopped\n", pid, cmd->words[0]);
            addJob(jobList, createJob(pid, pid, cmd->words[0], 1));
        } else {
            printf("\n%s: execution error\n", cmd->words[0]);
        }
    }
}

/* Extra modifications
void executeDefault(struct Command* cmd, struct Job** jobList) {
    signal(SIGTSTP, signalHandler);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        signal(SIGTSTP, signalHandler);
 
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
        
        //tcsetpgrp(STDIN_FILENO, getpgrp()); // return access to parent(bash)

        if (WIFEXITED(status)) {
        
        } else if (WIFSTOPPED(status)) {
            setsid(); // create new process group
            
            //tcsetpgrp(STDIN_FILENO, getpid());
            // CTRL + Z pushed
            printf("\n[%d] %s Stopped\n", pid, cmd->words[0]);
            addJob(jobList, createJob(pid, pid, cmd->words[0], 1));
        } else {
            printf("\n%s: execution error\n", cmd->words[0]);
        }
    }
} */

// operator '>'
void outputInFile(struct Command* cmd, const char* filename) {
    int fd[2];
    pipe(fd);
    
   
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(fd[0]);

        if (dup2(fd[1], STDOUT_FILENO) == -1) {
        	perror("dup2");
        	exit(EXIT_FAILURE);
        } 
        

        execvp(cmd->words[0], cmd->words); 

        close(fd[1]); 
        exit(EXIT_SUCCESS);
    } else { 
        close(fd[1]); 

        FILE *file = fopen(filename, "w");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer[1024];
        int bytes_read;

        while ((bytes_read = read(fd[0], buffer, sizeof(buffer))) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                fputc(buffer[i], file);
            }
        }

        fclose(file);
        close(fd[0]);
        
        wait(NULL);
    }
}

// operator '>>'
void appendToFile(struct Command* cmd, const char* filename) {
    int fd[2];
    pipe(fd);
    
   
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(fd[0]);

        dup2(fd[1], STDOUT_FILENO);

        execvp(cmd->words[0], cmd->words); 

        close(fd[1]); 
        exit(EXIT_SUCCESS);
    } else { 
        close(fd[1]); 

        FILE *file = fopen(filename, "a");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        char buffer[1024];
        int bytes_read;
        
        while ((bytes_read = read(fd[0], buffer, sizeof(buffer))) > 0) {
            for (int i = 0; i < bytes_read; i++) {
                fputc(buffer[i], file);
            }
        }

        fclose(file);
        close(fd[0]);
        
        wait(NULL);
    }
}

void cat(const char* filename);

// operator '<'
void inputFromFile(struct Command* cmd, const char* filename) {
   // if (strcmp(cmd->words[0], "cat") == 0) {
     //   cat(filename);
    if (1) {// else {
        int fd[2];
        pipe(fd);

        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {

            close(fd[1]);  
            
            dup2(fd[0], STDIN_FILENO);
            
            execvp(cmd->words[0], cmd->words);

            close(fd[0]);

            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            close(fd[0]);

            FILE *file = fopen(filename, "r");
            if (file == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }

            char buffer[1024];
            size_t bytesRead;

            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                write(fd[1], buffer, bytesRead);
            }

            fclose(file);
            close(fd[1]);

            wait(NULL);
        }
    }
}


void executeCommand(struct Command* cmd, struct Job** jobList, int firstOperatorFlag, int secondFlag, struct History* historyList) {
    if (cmd == NULL) {
        return;
    } 
    
    // Check if word is '#' then won't execute following commands
    if (strcmp(cmd->words[0], "#") == 0) {
    	cmd->next = NULL;
    	return;	
    }
    // Check the command's flag and execute accordingly
    if ((firstOperatorFlag == 3 && secondFlag == 4) || (firstOperatorFlag == 4 && secondFlag == 3)) {
    	executeCommandSequence(cmd);
    } else if (firstOperatorFlag == 1 || cmd->flag == 1) { // Pipe ('|')
        executePipeline(cmd);
        
    } else if (firstOperatorFlag == 2 || cmd->flag == 2) { // Background ('&')
    	executeInBackground(cmd, jobList);
    			
    } else if (firstOperatorFlag == 3 || cmd->flag == 3) { // Logical OR ('||')
        executeOrOperator(cmd);
        
    } else if (firstOperatorFlag == 4 || cmd->flag == 4) { // Logical AND ('&&')
        executeAndOperator(cmd);
        
    } else if (firstOperatorFlag == 5 || cmd->flag == 5) { // Sequential execution (';')
        executeSeqOperator(cmd);
        
    } else if (firstOperatorFlag == 6 || cmd->flag == 6) { // redirect output in file '>'
    	struct Command* next_cmd = cmd->next;
    	next_cmd->filename = next_cmd->words[0];
    	outputInFile(cmd, next_cmd->filename);
    	
    } else if (firstOperatorFlag == 7 || cmd->flag == 7) { // append data to the end of the file '>>'
    	struct Command* next_cmd = cmd->next; 
    	next_cmd->filename = next_cmd->words[0];
    	appendToFile(cmd, next_cmd->filename);
    	
    } else if (firstOperatorFlag == 8 || cmd->flag == 8) { // output of file contents to the stream '<'
    	struct Command* next_cmd = cmd->next; 
    	next_cmd->filename = next_cmd->words[0];
    	inputFromFile(cmd, next_cmd->filename);
    	 
    } else { // Default, no operator
    	executeDefault(cmd, jobList, historyList);
    }
}


// pwd: path
void pwd() {
    char path[1024]; // Array for path
    if (getcwd(path, sizeof(path)) != NULL) {
        printf("\033[1;34m%s\033[0m$ ", path);
    } else {
        perror("getcwd");
    }
}

// cd: change directory
void cd(const char* path) {
	if (path == NULL) {
		const char* home = "/home";
		if (chdir(home) != 0) {
			perror("bash: cd");
		}
	} else if (path == "..") {
		if (chdir(path) != 0) {
			perror("bash: cd");
		}
	} else {
		if (chdir(path) != 0) {
			perror("bash: cd");
		}
	}
}

// echo: print on screen
void echo(struct Command* cmd) {
	while (cmd != NULL && cmd->words[0] != NULL) {
		printf("%s ", cmd->words[0]);
		freeCmd(cmd);
		cmd = cmd->next;
	}
	printf("\n");
}

// help: manual page with bash commands
void help() {
	printf("Available commands:\n*****\n");
	printf("pwd [-LP] - Prints the absolute path to the screen.\n");
	printf("ls [-LP-flags...] - Lists the current directory's content.\n");
	printf("cd [dir] - Changes directory.\n");
	printf("exit - Closes the terminal.\n");
	printf("clear - Makes the terminal window empty.\n");
	printf("echo [arg ...] - Prints anything to the screen.\n");
	printf("rm [filename ...] - Remove a file or files.\n");
	printf("touch [filename ...] - Create a file or files.\n");
	printf("cat [filename ...] - Prints the contents of a file or files.\n____\n");
	printf("jobs [args] - Lists the active jobs. Default: the status of all active jobs is displayed.\n");
	printf("bg [job(pid or name)] - Transfer a job in the background mode.\n");
	printf("fg [job(pid or name)] - Transfer a job in the foreground(active) mode.\n");
	printf("kill [signal name(flags: '-l'...)] [job pid or job name] - Sends the signals to job\n");
	printf("wait [job(pid or name)] - Wait for job completion\n____\n");
	printf("history [-c] - Default: Display the history list with line numbers. With option [-c] it clears the history list.\n");
	printf("sleep [seconds] - Pauses the terminal for the entered seconds.\n");
}

// check the existence of the file
int isFile(const char* filename) {
	FILE* file = fopen(filename, "r");
	
	if (file) {
		fclose(file);
		return 1;
	} else {
		return 0;
	}
}

// rm: remove a file
void removeFile(struct Command* cmd) {
	while(cmd != NULL) {
		if (isFile(cmd->words[0])) {
			remove(cmd->words[0]); 
		} else {
			perror("rm");
		}
		cmd = cmd->next;
	}
}

// touch: create a new file
void touch(const char* filename) {
	FILE* file = fopen(filename, "a");
	if (file) {
		fclose(file);
	} 
}

// for function cat: hanlder for ignoring CTRL-C
int interrupt = 0;

void handler_interrupt(int sig) {
	interrupt = 1;
}

// cat: content of the files
void cat(const char* filename) {
	if (filename == NULL) {
		signal(SIGINT, handler_interrupt);
		
		int c;
		while (!interrupt) {
			c = getchar();
			if (c == EOF) break; // If CTRL+D was pushed
			else putchar(c);
		}
		interrupt = 0;
		return;	
	} else {
	
		FILE* file = fopen(filename, "r");
		if (!file) {
			perror("cat");
			return;
		}
		
		int c;
		while ((c = fgetc(file)) != EOF) {
			putchar(c);
		}
		
		fclose(file);
	}
}

void sleepCustom(int seconds, int flag, struct Job** jobList) {
    pid_t pid;

    if ((pid = fork()) < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        sleep(seconds);
        exit(0);
    } else {
        if (flag == 0) {
            // If not in background, wait for the child to finish
            waitpid(pid, NULL, 0);
        } else {
            printf("[1] %d\n", pid);
            addJob(jobList, createJob(pid, pid, "sleep", 0)); // Assuming jobList
            printf("Background job added: sleep %d seconds\n", seconds);
        }
    }
}
