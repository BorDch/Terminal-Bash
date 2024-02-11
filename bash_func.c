#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "history.c"

// Structure Command
struct Command {
    char** words;         // Command words
    int flag;             // Operator flag (0 - default, 1 - '|', 2 - '&', 3 - '||', 4 - '&&', .....)
    pid_t pid;
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
       		return input;
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
    char** words = NULL;
    words = (char**)malloc(wordBufferSize * sizeof(char*));
    
    if (words == NULL) {
        perror("Memory allocation");
        exit(1);
    }
    
    for (int k = 0; k < wordBufferSize; k++) {
    	words[k] = NULL;
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


int isOperatorFlag(char** words, int currentFlag, int i, int* firstOperatorFlag, int* secondOperatorFlag) { // for commands
	if (strcmp(words[i], "|") == 0) {
            currentFlag = 1;
        } else if (strcmp(words[i], "&") == 0) {
            currentFlag = 2;
        } else if (strcmp(words[i], "||") == 0) {
            currentFlag = 3;
            if (*firstOperatorFlag == 4 && *secondOperatorFlag == 0) {
            	*secondOperatorFlag = 3;
            }
        } else if (strcmp(words[i], "&&") == 0) {
            currentFlag = 4;
            
            if (*firstOperatorFlag == 3 && *secondOperatorFlag == 0) {
            	*secondOperatorFlag = 4;
            }
        } else if (strcmp(words[i], ";") == 0) {
            currentFlag = 5;
        } else if (strcmp(words[i], ">") == 0) {
        	currentFlag = 6;
        } else if (strcmp(words[i], ">>") == 0) {
        	currentFlag = 7;
        	
        } else if (strcmp(words[i], "<") == 0) {
        	currentFlag = 8;
        }
        
        return currentFlag;
}


int wordOpFlag(char** words, int currentFlag, int i) { // for words
	if (strcmp(words[i], "|") == 0) {
            currentFlag = 1;
        } else if (strcmp(words[i], "&") == 0) {
            currentFlag = 2;
        } else if (strcmp(words[i], "||") == 0) {
            currentFlag = 3;
        } else if (strcmp(words[i], "&&") == 0) {
            currentFlag = 4;
            
        } else if (strcmp(words[i], ";") == 0) {
            currentFlag = 5;
        } else if (strcmp(words[i], ">") == 0) {
        	currentFlag = 6;
        } else if (strcmp(words[i], ">>") == 0) {
        	currentFlag = 7;
        	
        } else if (strcmp(words[i], "<") == 0) {
        	currentFlag = 8;
        }
        
        return currentFlag;
}

int isOperator(char** words, int i) {
	if ((strcmp(words[i], "|") == 0) || (strcmp(words[i], "&") == 0) ||
		(strcmp(words[i], "||") == 0) || (strcmp(words[i], "&&") == 0) ||
		(strcmp(words[i], ";") == 0) || (strcmp(words[i], ">") == 0) ||
		(strcmp(words[i], ">>") == 0) || (strcmp(words[i], "<") == 0)) {
		return 1;
	}
	return 0;		
}

void freeCommand(struct Command** cmd);


// Function to parse an array of words into a linked list of Command structures
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag, int* secondOperatorFlag) {
    struct Command* head = NULL;  // Command head
    struct Command* current = NULL;
    struct Command* lastWordCommand = NULL;
    int currentFlag = 0; // Current flag for operators

    for (int i = 0; i < wordCount; i++) {
        if (strcmp(words[i], "|") == 0 || strcmp(words[i], "&") == 0 ||
            strcmp(words[i], "||") == 0 || strcmp(words[i], "&&") == 0 ||
            strcmp(words[i], ";") == 0 || strcmp(words[i], ">") == 0 ||
            strcmp(words[i], ">>") == 0 || strcmp(words[i], "<") == 0) {
            *firstOperatorFlag = isOperatorFlag(words, currentFlag, i, firstOperatorFlag, secondOperatorFlag);
            break;
        }
    }

    if (strcmp(words[wordCount - 1], "&") == 0) {
        *secondOperatorFlag = 2;
    }

    for (int i = 0; i < wordCount; i++) {
        struct Command* cmd = (struct Command*)malloc(sizeof(struct Command));
        if (cmd == NULL) {
            perror("Memory allocation");
            freeCommand(&head);
            return NULL;
        }
        cmd->words = NULL;
        cmd->flag = 0;
        cmd->filename = NULL;
        cmd->next = NULL;

        if (head == NULL) {
            head = cmd;
            current = cmd;
        } else {
            current->next = cmd;
            current = cmd;
        }

        int wordLength = strlen(words[i]);
        cmd->words = (char**)malloc(10 * sizeof(char*));
        if (cmd->words == NULL) {
            perror("Memory allocation");
            freeCommand(&head);
            return NULL;
        }
        
        for (int k = 0; k < 10; k++) {
        	cmd->words[k] = NULL;
        }

        int j = 0;
        int temp_flag = 0;
        currentFlag = temp_flag;
        while (words[i] != NULL) {
            if (!isOperator(words, i)) {
                cmd->words[j] = strdup(words[i]);
                cmd->flag = currentFlag; // currentFlag's default value is 0
            } else if (isOperator(words, i)) {
                currentFlag = isOperatorFlag(words, temp_flag, i, firstOperatorFlag, secondOperatorFlag);
                cmd->flag = currentFlag;
                free(cmd->words[j]);
                cmd->words[j] = NULL;
            }

	    if (currentFlag != 0) { 
	   	break;
	    }
		
	    i++;
            j++;
        }

        // Connect last word with a logical operator
        if (lastWordCommand != NULL) {
            lastWordCommand->next = cmd;
        }
        // Remember the last node
        lastWordCommand = cmd;
    }

    return head;
}

// Function to free the memory allocated for Command structures
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


// Function to print the Command list
void printCommands(struct Command* head) {
    struct Command* current = head;
    while (current != NULL) {
        printf("Command words: \nFlag: %d\n", current->flag);
        for (int i = 0; current->words != NULL && current->words[i] != NULL; ++i) {
            printf("%s ", current->words[i]);
        }
        printf("\n");

        // Move to the next Command
        current = current->next;
    }
}


void executeAndOperator(struct Command* cmd) {
    int status = 0;
    
    while (cmd != NULL) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            cmd->pid = getpid();
            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else {
        	cmd->pid = pid;
            wait(&status);
        }
        cmd = cmd->next;
    }
}

void executeOrOperator(struct Command* cmd) {
    int status = 0;
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
        	cmd->pid = pid;
            wait(&status);
            if (status == 0) {
                break;
            }
        }
        cmd = cmd->next;
    }
}

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
                	cmd->pid = pid;
                    wait(&status);
                    success = (status == 0);
                }
            }
        } else if (cmd->flag == 3) {  // ''
            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(1);
            } else if (pid == 0) {
                execvp(cmd->words[0], cmd->words);
                perror("execvp");
                exit(1);
            } else {
            	cmd->pid = pid;
                wait(&status);
                if (status == 0) {
                    // Break out of the loop if '' command succeeded
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
            	cmd->pid = pid;
                wait(NULL);
            }
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
        	cmd->pid = pid;
            wait(&status);
        }
        cmd = cmd->next;
    }
}


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

// operator '<'
void inputFromFile(struct Command* cmd, const char* filename) {
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
void echo(const char* str) {
	if (str == NULL) printf("\n");
	else printf("%s\n", str);
}

// help: manual page with bash commands
void help() {
    printf("\033[1;31mAvailable commands\033[0m:\n\n*****\n");
    printf("\033[1;32m____\033[0m\n\n"); // Green underline
    printf("\033[1;31mpwd\033[0m [-LP] - Prints the absolute path to the screen.\n");
    printf("\033[1;31mls\033[0m [-LP-flags...] - Lists the current directory's content.\n");
    printf("\033[1;31mcd\033[0m [dir] - Changes directory.\n");
    printf("\033[1;31mexit\033[0m - Closes the terminal.\n");
    printf("\033[1;31mclear\033[0m - Makes the terminal window empty.\n");
    printf("\033[1;31mecho\033[0m [arg ...] - Prints anything to the screen.\n");
    printf("\033[1;31mrm\033[0m [filename ...] - Remove a file or files.\n");
    printf("\033[1;31mtouch\033[0m [filename ...] - Create a file or files.\n");
    printf("\033[1;31mcat\033[0m [filename ...] - Prints the contents of a file or files.\n");
    printf("\033[1;32m____\033[0m\n\n"); // Green underline
    printf("\033[1;31mjobs\033[0m [args] - Lists the active jobs. Default: the status of all active jobs is displayed.\n");
    printf("\033[1;31mbg\033[0m [job(pid or name)] - Transfer a job in the background mode.\n");
    printf("\033[1;31mfg\033[0m [job(pid or name)] - Transfer a job in the foreground(active) mode.\n");
    printf("\033[1;31mkill\033[0m [signal name(flags: '-l'...)] [job pid or job name] - Sends the signals to job\n");
    printf("\033[1;31mwait\033[0m [job(pid or name)] - Wait for job completion\n");
    printf("\033[1;32m____\033[0m\n\n"); // Green underline
    printf("\033[1;31mhistory\033[0m [-c] - Default: Display the history list with line numbers. With option [-c] it clears the history list.\n");
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
