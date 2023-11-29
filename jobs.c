#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>


// Structure Job
struct Job {
    pid_t pid;       // Process ID
    pid_t pgid;      // Process Group ID
    char* command;   // Command string
    int state;       // Process state (0 - running, 1 - stopped, 2 - killed)
    struct Job* next; // Next Job
};

// Function for creating a new Job
struct Job* createJob(pid_t pid, pid_t pgid, const char* command, int state) {
    struct Job* job = (struct Job*)malloc(sizeof(struct Job));
    if (job == NULL) {
        perror("Memory allocation");
        exit(1);
    }

    job->pid = pid;
    job->pgid = pgid;
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
            return;
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
    	int status;
        pid_t result = waitpid(current->pid, NULL, WNOHANG | WUNTRACED);

        if (result == -1) {
            perror("waitpid");
            exit(1);
        } else if (result == 0) {
            // Process is still running
            prev = current;
            current = current->next;
        } else {
        	if (WIFEXITED(status)) {
        		// Process has terminated
			printf("[%d]+  Done\t\t%s\n", getJobCount(*jobList), current->command);
		} else if (WIFSTOPPED(status)) {
			printf("[%d]+  Stopped\t\t%s\n", getJobCount(*jobList), current->command);
			}
			
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


// Function to bring a background job to the foreground by PID or command name
void bringToForeground(struct Job** jobList, char* identifier) {
    // Identifier as a pid or name of command
    if (identifier == NULL) {
        struct Job* current = *jobList;
        struct Job* lastJob = NULL;

        while (current != NULL) {
            lastJob = current;
            current = current->next;
        }

        if (lastJob != NULL) {
            pid_t pid = lastJob->pid;
            
            if (strcmp(lastJob->command, "cat") == 0 || strcmp(lastJob->command, "wc") == 0) {
            	kill(-pid, SIGINT);
            } else {
		    	// Bring the job to the foreground
		    	waitpid(pid, NULL, 0); // Wait for the process to finish
		    	printf("[%d]+  Done\t\t%s\n", getJobCount(*jobList), lastJob->command);
            }
            
            // Remove the job from the list
            removeFromJobList(jobList, pid);
		
            lastJob = NULL;
        } else {
            printf("No background jobs.\n");
        }
    } else {
        // Check if the identifier is a PID
        pid_t pid = atoi(identifier);

        // Find the job with the given PID or command name
        struct Job* current = *jobList;
        struct Job* prev = NULL;

        while (current != NULL) {
            if (current->pid == pid || (strcmp(current->command, identifier) == 0)) {
            	if (strcmp(current->command, "cat") == 0 || strcmp(current->command, "wc") == 0)  {
                	kill(-pid, SIGINT);	
                } else {
		            // Bring the job to the foreground
		            waitpid(current->pid, NULL, 0); // Wait for the process to finish
		            printf("[%d]+  Done\t\t%s\n", getJobCount(*jobList), current->command);
		        }

                // Remove the job from the list
                if (prev == NULL) {
                    *jobList = current->next;
                } else {
                    prev->next = current->next;
                }

                free(current);
                current = NULL;
                return;
            }

            prev = current;
            current = current->next;
        }

        printf("Job with identifier %s not found.\n", identifier);
    }
}
