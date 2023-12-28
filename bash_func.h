#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bash_func.c"

// For Jobs
struct Job* createJob(pid_t pid, pid_t pgid, char* command, int state);
void addJob(struct Job** jobList, struct Job* newJob);
int getJobCount(struct Job* jobList);
struct Job* getLastJob(struct Job* jobList);
struct Job* findJobByPid(struct Job* jobList, char* identifier);
void removeFromJobList(struct Job** jobList, pid_t pid);
void printJobsList(struct Job* jobList);
void printJobs(struct Job* jobList);
void updateJobList(struct Job** jobList);
void bringToForeground(struct Job** jobList, char* identifier);
void kill_process(pid_t pid);
void killProcessByIdentifier(struct Job** jobList, char** identifierArray);
void resumeInBackground(struct Job** jobList, char* identifier);
void waitProcess(struct Job** jobList, pid_t pid);


// Command processing
char* characterInput();
char** splitStringWithoutSpaces(char* str, int* wordCount);
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag, int* secondOperatorFlag);
void printAllWords(struct Command* commands);


// Command Operators
void executeAndOperator(struct Command* cmd);
void executeOrOperator(struct Command* cmd);
void executeCommandSequence(struct Command* cmd);
void executePipeline(struct Command* cmd);
void executeSeqOperator(struct Command* cmd);
void executeInBackground(struct Command* cmd, struct Job** jobList);
void executeDefault(struct Command* cmd, struct Job** jobList, struct History** historyList);


// Redirection input and output
void outputInFile(struct Command* cmd, const char* filename);
void appendToFile(struct Command* cmd, const char* filename);
void inputFromFile(struct Command* cmd, const char* filename); 


// Execute Commands
void executeCommand(struct Command* cmd, struct Job** joblist, int firstOperatorFlag, int secondFlag, struct History** historyList);
//void executeUntilGrid(struct Command* cmd, struct Job** jobList);
void pwd();
void echo(struct Command* cmd);
void help();
int isFile(const char* filename);
void removeFile(struct Command* cmd);
void touch(const char* filename);
void cat(const char* filename);
void sleepCustom(int seconds, int flag, struct Job** jobList);


// For Bash History
struct History* createHistoryNode(char* command);
void addToHistory(struct History** historyList, char* command);
void printHistory(const struct History* historyList);
void freeHistory(struct History* historyList);
void clearHistory(struct History** historyList);
//struct History* findCommandInHistory(struct History* historyList, char* command);
//void removeFromHistory(struct History** historyList, struct History* nodeToRemove);


// Free memory 
void freeCmd(struct Command* cmd);
void freeCommand(struct Command** cmd); 
void freeJob(struct Job* job);
void clearJobs(struct Job** jobList);
