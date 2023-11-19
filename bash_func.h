#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bash_func.c"


struct Job* createJob(pid_t pid, const char* command, int state);
void addJob(struct Job** jobList, struct Job* newJob);
int getJobCount(struct Job* jobList);
void removeFromJobList(struct Job** jobList, pid_t pid);
void printJobsList(struct Job* jobList);
void printJobs(struct Job* jobList);
void updateJobList(struct Job** jobList);


char* characterInput();
char** splitStringWithoutSpaces(char* str, int* wordCount);
struct Command* parseCommandsFromWords(char** words, int wordCount, int* firstOperatorFlag);
void printAllWords(struct Command* commands);


void executeAndOperator(struct Command* cmd);
void executeOrOperator(struct Command* cmd);
void executePipeline(struct Command* cmd);
void executeSeqOperator(struct Command* cmd);
void executeInBackground(struct Command* cmd, struct Job** jobList);


void outputInFile(struct Command* cmd, const char* filename);
void appendToFile(struct Command* cmd, const char* filename);
void inputFromFile(struct Command* cmd, const char* filename); 


void executeCommand(struct Command* cmd, int firstOperatorFlag, struct Job** joblist);
void pwd();
void echo(struct Command* cmd);
void help();
int isFile(const char* filename);
void removeFile(struct Command* cmd);
void touch(const char* filename);
void cat(const char* filename);
