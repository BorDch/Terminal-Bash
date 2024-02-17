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
    pid_t pid;
    struct Command* next; // Next Command
    char* filename;       // for several functions
};


// Structure Job
struct Job {
    pid_t pid;       // Process ID
    pid_t pgid;      // Process Group ID
    char* command;   // Command string
    int state;       // Process state (0 - running, 1 - stopped, 2 - terminated, ...)
    struct Command* commands; // List of commands
    struct Job* next; // Next Job
};


// Structure for command history
struct History {
    char* command;
    struct History* next;
};


// For Jobs
struct Job* createJob(pid_t pid, pid_t pgid, char* command, int state, struct Command* commands);
void addJob(struct Job** jobList, struct Job* newJob);
int getJobCount(struct Job* jobList);
struct Job* getLastJob(struct Job* jobList);
struct Job* findJobByPid(struct Job* jobList, char* identifier);
void removeFromJobList(struct Job** jobList, pid_t pid);
void updateJobList(struct Job** jobList);
void bringToForeground(struct Job** jobList, char* identifier);
void killProcessByIdentifier(struct Command* commands, struct Job** jobList, char** identifierArray);
void resumeInBackground(struct Job** jobList, char* identifier);
void waitProcess(struct Job** jobList, pid_t pid);
void printJobs(struct Job* job);
void printJobsWithCommands(struct Job* jobList);


// Command processing
char* characterInput();
char** splitStringWithoutSpaces(char* str, int* wordCount);
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag, int* secondOperatorFlag);
void printCommand(struct Command* head);


// Command Operators
void executeCommand(struct Command* cmd, struct Job** jobList, struct History** historyList, int firstOperatorFlag, int secondFlag);

void executePipeline(struct Command* cmd, struct Job** jobList);
void executeDefault(struct Command* cmd, struct Job** jobList, struct History** historyList);
void executeInBackground(struct Command* cmd, struct Job** jobList);
void executeSeqOperator(struct Command* cmd);
void executeCommandSequence(struct Command* cmd);
void executeOrOperator(struct Command* cmd);
void executeAndOperator(struct Command* cmd);


// Redirection input and output
void outputInFile(struct Command* cmd, const char* filename);
void appendToFile(struct Command* cmd, const char* filename);
void inputFromFile(struct Command* cmd, const char* filename); 



// Other Bash commands
void pwd();
void cd(const char* path);
void echo(const char* str);
void help();
int isFile(const char* filename);
void removeFile(struct Command* cmd);
void touch(const char* filename);


// For Bash History
struct History* createHistoryNode(char* command);
void addToHistory(struct History** historyList, char* command);
void printHistory(const struct History* historyList);
void freeHistory(struct History* historyList);
void clearHistory(struct History** historyList);

// Free memory 
void freeCommand(struct Command** cmd);
void freeCmd(struct Command* cmd);
void clearJobs(struct Job** jobList); 
