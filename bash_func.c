#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Structure Command
struct Command {
    char** words;         // Command words
    int flag;             // Operator flag (0 - default, 1 - '|', 2 - '&', 3 - '||', 4 - '&&', .....)
    struct Command* next; // Next Command
};


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
            cmd->flag == 5;
            currentFlag == 5;
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


/*
// Function for operator '&&'
void executeAndOperator(struct Command* cmd) {
	int status;
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
                return; 
            }
        }
} 

// Function for operator '||'
void executeOrOperator(struct Command* cmd) {
	int status;
	
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
			return;
		}
	}
} */

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



void executeCommand(struct Command* cmd, int firstOperatorFlag) {
    if (cmd == NULL) {
        return;
    }
    
    // Check the command's flag and execute accordingly
    if (firstOperatorFlag == 1 || cmd->flag == 1) { // Pipe ('|')
        executePipeline(cmd);
    } else if (firstOperatorFlag == 2 || cmd->flag == 2) { // Background ('&')
    
    } else if (firstOperatorFlag == 3 || cmd->flag == 3) { // Logical OR ('||')
        executeOrOperator(cmd);
    } else if (firstOperatorFlag == 4 || cmd->flag == 4) { // Logical AND ('&&')
        executeAndOperator(cmd);
    } else if (firstOperatorFlag == 5 || cmd->flag == 5) { // Sequential execution (';')
        executeSeqOperator(cmd);
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
    char path[1024]; // Создаем массив для хранения пути
    if (getcwd(path, sizeof(path)) != NULL) {
        printf("\033[1;34m%s\033[0m$ ", path);
    } else {
        perror("getcwd");
    }
}
