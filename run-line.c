#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

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


int main(int argc, char** argv) {
    char* inputString = getInputString();
    char** words;
    int wordCount;

    splitString(inputString, &words, &wordCount);

/*  Words output

    for (int i = 0;  i < wordCount; i++) {
        printf("Word %d: %s\n", i + 1, words[i]);
        free(words[i]); // Free memory for word
    }
*/
    free(words); // Free memory for array of pointers
    free(inputString); // Free memory for input string

    return 0;
}
