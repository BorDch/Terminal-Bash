#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY_SIZE 256

// Structure for command history
struct History {
    char* command;
    struct History* next;
};

// Function to create a new history node
struct History* createHistoryNode(char* command) {
    struct History* newNode = (struct History*)malloc(sizeof(struct History));
    if (newNode == NULL) {
        perror("Memory allocation");
        exit(1);
    }

    newNode->command = strdup(command);
    newNode->next = NULL;

    return newNode;
}

// Function to add a command to history
void addToHistory(struct History** historyList, char* command) {
    struct History* newNode = createHistoryNode(command);
    newNode->next = *historyList;
    *historyList = newNode;
}

// Function to print command history
void printHistory(const struct History* historyList) {
    int count = 1;
    const struct History* current = historyList;
    const struct History* historyArray[MAX_HISTORY_SIZE];
    
    int index = 0;
    while (current != NULL) {
        historyArray[index] = current;
        current = current->next;
        index++;
    }
    
    for (int i = index - 1; i >= 0; i--) {
    	printf("%d: %s\n", count, historyArray[i]->command);
    	count++;
    }
}


// Function to free memory allocated for history
void freeHistory(struct History* historyList) {
    struct History* current = historyList;
    struct History* next;

    while (current != NULL) {
        next = current->next;
        free(current->command);
        free(current);
        current = next;
    }
}

void freeHistoryNode(struct History** historyNode) {
	if (*historyNode != NULL) {
		struct History* temp = *historyNode;
		free(temp->command);
		free(temp);
	}
	
	*historyNode = NULL;
}

// Function to clear command history
void clearHistory(struct History** historyList) {
    freeHistory(*historyList);
    *historyList = NULL;
}
