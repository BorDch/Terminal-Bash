#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

// Structure Command
struct Command {
    char** words;         // Command words
    int flag;             // Operator flag (0 - default, 1 - '|', 2 - '&', 3 - '||', 4 - '&&', .....)
    struct Command* next; // Next Command
    char* filename;       // for several functions
};


// Structure Job
struct Job {
    pid_t pid;       // Process ID
    char* command;   // Command string
    int state;       // Process state (0 - running, 1 - stopped, 2 - killed)
    struct Job* next; // Next Job
};

// Function for creating a new Job
struct Job* createJob(pid_t pid, const char* command, int state) {
    struct Job* job = (struct Job*)malloc(sizeof(struct Job));
    if (job == NULL) {
        perror("Memory allocation");
        exit(1);
    }

    job->pid = pid;
    job->command = strdup(command);
    job->state = state;
    job->next = NULL;

    return job;
}

// Function for adding a Job to the list
void addJob(struct Job** jobList, struct Job* newJob) {
    if (*jobList == NULL) {
        *jobList = newJob;
    } else {
        struct Job* current = *jobList;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newJob;
    }
}

// get the number of tasks in the JobList
int getJobCount(struct Job* jobList) {
    int count = 0;
    struct Job* current = jobList;

    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;
}

// Function for removing a Job from the list
void removeFromJobList(struct Job** jobList, pid_t pid) {
    struct Job* current = *jobList;
    struct Job* prev = NULL;

    while (current != NULL) {
        if (current->pid == pid) {
            if (prev == NULL) {
                *jobList = current->next;
            } else {
                prev->next = current->next;
            }

            free(current);
            break;
        }

        prev = current;
        current = current->next;
    }
}

// Function for printing all Jobs in the list
void printJobsList(struct Job* jobList) {
    struct Job* current = jobList;
    while (current != NULL) {
        printf("[%d] %s\t%s\n", current->pid, (current->state == 0 ? "Running" : "Stopped"), current->command);
        current = current->next;
    }
}

// Function for printing all Jobs
void printJobs(struct Job* jobList) {
    if (jobList == NULL) {
        printf("No background jobs.\n");
    } else {
        printJobsList(jobList);
    }
}

// Check if a background job has terminated and update the job list
void updateJobList(struct Job** jobList) {
    struct Job* current = *jobList;
    struct Job* prev = NULL;

    while (current != NULL) {
        pid_t result = waitpid(current->pid, NULL, WNOHANG);

        if (result == -1) {
            perror("waitpid");
            exit(1);
        } else if (result == 0) {
            // Process is still running
            prev = current;
            current = current->next;
        } else {
            // Process has terminated
            printf("[%d]+  Done\t\t%s\n", getJobCount(*jobList), current->command);

            if (prev == NULL) {
                // Current job is the first in the list
                removeFromJobList(jobList, current->pid);
                current = *jobList;
            } else {
                // Current job is not the first in the list
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
        }
    }
}


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


// Parser Function
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag) {
    struct Command* head = NULL;  // Command head
    struct Command* current = NULL;
    struct Command* lastWordCommand = NULL; // last Command node 
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
        
        if (strcmp(words[i], "|") == 0) {
            cmd->flag = 1;
            currentFlag = 1;
        } else if (strcmp(words[i], "&") == 0) {
            cmd->flag = 2;
            currentFlag = 2;
        } else if (strcmp(words[i], "||") == 0) {
            cmd->flag = 3;
            currentFlag = 3;
        } else if (strcmp(words[i], "&&") == 0) {
            cmd->flag = 4;
            currentFlag = 4;
        } else if (strcmp(words[i], ";") == 0) {
            cmd->flag = 5;
            currentFlag = 5;
        } else if (strcmp(words[i], ">") == 0) {
        	cmd->flag = 6;
        	currentFlag = 6;
        } else if (strcmp(words[i], ">>") == 0) {
        	cmd->flag = 7;
        	currentFlag = 7;
        } else if (strcmp(words[i], "<") == 0) {
        	cmd->flag = 8;
        	currentFlag = 8;
        } else {
            int wordLength = strlen(words[i]);
            cmd->words = (char**)malloc(2 * sizeof(char*));
            if (cmd->words == NULL) {
                perror("Memory allocation");
                exit(1);
            }
            cmd->words[0] = (char*)malloc(wordLength + 1);
            if (cmd->words[0] == NULL) {
                perror("Memory allocation");
                exit(1);
            }
            strcpy(cmd->words[0], words[i]);
            cmd->words[1] = NULL;
            cmd->flag = currentFlag;
        
            // Connect last word with logical operator
            if (lastWordCommand != NULL) {
                lastWordCommand->next = cmd;
            }
            // Remember last node
            lastWordCommand = cmd;
            //printf("Parsed word: %s, Flag: %d\n", cmd->words[0], cmd->flag);
            
        }
    }
    
    return head;
}

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

// Pipe Function
void executePipeline(struct Command* cmd) {
    int fd[2];

    while (cmd != NULL) {
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(1);
            }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            if (cmd->next != NULL) {
                if (dup2(fd[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
            }

            if (cmd->next != NULL) {
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
            }

            close(fd[0]);
            close(fd[1]);

            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        }

        if (cmd->flag == 1) {
            close(fd[0]);
        }

        wait(NULL);
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

// Function for operator '&'
void executeInBackground(struct Command* cmd, struct Job** jobList) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Child process
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        printf("Process wth id [%d]\n", pid);
        addJob(jobList, createJob(pid, cmd->words[0], 0)); // Adding the job to the list
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


void executeCommand(struct Command* cmd, int firstOperatorFlag, struct Job** jobList) {
    if (cmd == NULL) {
        return;
    }
    
    // Check the command's flag and execute accordingly
    if (firstOperatorFlag == 1 || cmd->flag == 1) { // Pipe ('|')
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
		cmd = cmd->next;
	}
	printf("\n");
}

// help: manual page with bash commands
void help() {
	printf("Available commands:\n*****\n");
	printf("pwd [-LP] - prints the absolute path to the screen\n");
	printf("cd [dir] - changes directory\n");
	printf("exit - closes the terminal\n");
	printf("clear - makes the terminal window empty\n");
	printf("echo [arg ...] - prints anything to the screen\n");
	printf("rm [filename ...] - remove a file or files\n");
	printf("touch [filename ...] - create a file or files\n");
	printf("cat [filename ...] - prints the contents of a file or files\n");
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

// cat: content of the file
void cat(const char* filename) {
	if (filename == NULL) {
		signal(SIGINT, handler_interrupt);
		
		int c;
		while (!interrupt) {
			c = getchar();
			if (c == EOF) break;
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

