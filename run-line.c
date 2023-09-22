#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "bash-functions.h" // my own library with written commands for bash


// Function for entering a string with character-by-character input
char* getInputString() {
    char* str = NULL;
    char c;
    int length = 0;

    while (1) {
        c = getchar();

        if (c == '\n') {
            break;
        }

        // Allocation memory for new symbol
        char* temp = (char*)realloc(str, (length + 1) * sizeof(char));
        if (temp == NULL) {
            perror("Memory allocation.");
            free(str);
            exit(1);
        }

        str = temp;
        str[length++] = c;
    }

    // Adding a trailing null character
    char* temp = (char*)realloc(str, (length + 1) * sizeof(char));
    if (temp == NULL) {
        perror("Memory allocation.");
        free(str);
        exit(1);
    }

    str = temp;
    str[length] = '\0';

    return str;
}

// A function for splitting a string into words and writing them into a triple array
void splitString(char* inputString, char*** words, int* wordCount) {
    int length = strlen(inputString);
    int start = 0;
    int end = 0;
    *wordCount = 0;

    // Dynamic allocation
    *words = (char**)malloc(sizeof(char*));

    for (int i = 0; i <= length; i++) {
        if (inputString[i] == ' ' || inputString[i] == '\0') {
            end = i;
            
            if (end > start) {
                // Allocating memory for a word and copying it
                (*words)[*wordCount] = (char*)malloc((end - start + 1) * sizeof(char));
                strncpy((*words)[*wordCount], inputString + start, end - start);
                (*words)[*wordCount][end - start] = '\0';
                (*wordCount)++;
                
                // Dynamic allocation (expansion) of memory
                *words = (char**)realloc(*words, (*wordCount + 1) * sizeof(char*));
            }
            
            start = end + 1;
        }
    }
}

// A function that takes array of words and executes the entered command(program)
void progexec(char** array_args) {
	pid_t pid = fork();
	
	if (pid == 0) {
		if (execvp(array_args[0], array_args) == -1) {
			perror("execvp error");
			exit(1);
		}
	} else if (pid > 0) {
		int status;
		wait(&status);
		if (WIFEXITED(status)) {
			printf("Child process exited with status %d\n", WEXITSTATUS(status));
		}
	} else {
		perror("fork error");
		exit(1);
	}
}


int main(int argc, char** argv) {
    
    while(1) {
    
		char* inputString = getInputString();
		char** words;
		int wordCount;

		splitString(inputString, &words, &wordCount);

		int doubleStickFlag = 0; // flag for checking on '||'
		int doubleAmpersandFlag = 0; // flag for checking on '&&'
		
		for (int i = 0; i < wordCount; i++) {
			if (strcmp(words[i], "||") == 1) {
				doubleStickFlag = 1;
				break;
			}
		}
		
		for (int i = 0; i < wordCount; i++) {
			if (strcmp(words[i], "&&") == 1) {
				doubleAmpersandFlag = 1;
				break;
			}
		}
		
		if (doubleStickFlag) {
			doubleStickExec(&words);
		
		} else if (doubleAmpersandFlag) {
			doubleAmperndExec(&words);
		
		} else {
			
			if (wordCount > 0) {
				progexec(words);
			}
		}

		for (int i = 0; i < wordCount; i++) {
			free(words[i]);
		}

		free(words); // Free memory for array of pointers
		free(inputString); // Free memory for input string

	}

    return 0;
}
