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
    int state;       // Process state (0 - running, 1 - stopped, 2 - terminated, ...)
    struct Job* next; // Next Job
};

void clearJobs(struct Job** jobList);

// Function for creating a new Job
struct Job* createJob(pid_t pid, pid_t pgid, char* command, int state) {
    struct Job* job = (struct Job*)malloc(sizeof(struct Job));
    if (job == NULL) {
        perror("Memory allocation");
        clearJobs(&job);
        return NULL;
    }

    job->pid = pid;
    job->pgid = pgid;
    job->command = strdup(command);
    job->state = state;
    job->next = NULL;

    return job;
}

// Functions for cleaning Jobs
void freeJob(struct Job* job) {
	if (job == NULL) {
		return;
	}
	
	printf("Job address: %p\n", job);
	
	if (job->command != NULL) {
		printf("Job Command: %s\n", job->command);
		free(job->command);
	}
	
	if (job->next != NULL) {
		free(job->next);
	}
	
	free(job);
}

void clearJobs(struct Job** jobList) {
	struct Job* current = *jobList;
	struct Job* next;
	
	while (current != NULL) {
		next = current->next;
		free(current->command);
		free(current);
		current = next;
	}
	
	*jobList = NULL;
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


// Function to get the last job in the job list
struct Job* getLastJob(struct Job* jobList) {
    if (jobList == NULL) {
        return NULL;
    }

    struct Job* current = jobList;
    while (current->next != NULL) {
        current = current->next;
    }

    return current;
}


// Function to find a job by PID or command name
struct Job* findJobByPid(struct Job* jobList, char* identifier) {
    struct Job* current = jobList;

    while (current != NULL) {
        // Check if the identifier matches either the PID or command name
        if (current->pid == atoi(identifier) || strcmp(current->command, identifier) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;  // Job not found
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
	     
	    	free(current->command);
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
        const char* status;
        switch (current->state) {
            case 0:
                status = "Running";
                break;
            case 1:
                status = "Stopped";
                break;
            case 2:
                status = "Terminated";
                break;
            case 3:
                status = "Killed";
                break;
            case 4:
            	status = "Interrupted";
            	break;
            case 5:
				status = "Hangup";
				break;
			case 6:
				status = "Quited";
				break;
            default:
                status = "Unknown";
        }
        printf("[%d] %s\t%s\n", current->pid, status, current->command);
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
        pid_t result = waitpid(current->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        if (result == -1) {
            perror("waitpid");
            return;
        } else if (result == 0) {
            // Process is still running
            prev = current;
            current = current->next;
        } else {
         if (WIFEXITED(status)) {
          // Process has terminated
          printf("[%d]+  Done\t%s\n", getJobCount(*jobList), current->command);
		 } else if (WIFSTOPPED(status) && current->state == 1) {
			printf("[%d]+  Stopped\t%s\n", getJobCount(*jobList), current->command);
		 } else if (WIFSIGNALED(status) && current->state == 2) {
			printf("[%d]+  Terminated\t%s\n", getJobCount(*jobList), current->command);
		 } else if (WIFSIGNALED(status) && current->state == 4) {
			printf("[%d]+  Interrupted\t%s\n", getJobCount(*jobList), current->command);
		 } else if (WIFSIGNALED(status) && current->state == 5) {
			printf("[%d]+  Hangup\t%s\n", getJobCount(*jobList), current->command);
		 } else if (WIFCONTINUED(status)) {
		 	printf("[%d]+ Running\t%s\n", getJobCount(*jobList), current->command);
		 	addJob(jobList, createJob(current->pid, current->pgid, current->command, current->state));
		 }
   
		 if (prev == NULL) {
			// Current job is the first in the list
			removeFromJobList(jobList, current->pid);
			current = *jobList;
		 } else {
			// Current job is not the first in the list
			prev->next = current->next;
			free(current->command);
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
            	kill(pid, SIGINT);
            } else {
		    	// Bring the job to the foreground
		    	kill(pid, SIGCONT);
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
                	kill(pid, SIGINT);	
                } else {
		            // Bring the job to the foreground
		            kill(pid, SIGCONT);
		            waitpid(current->pid, NULL, 0); // Wait for the process to finish
		            printf("[%d]+  Done\t\t%s\n", getJobCount(*jobList), current->command);
		        }

                // Remove the job from the list
                if (prev == NULL) {
                    *jobList = current->next;
                } else {
                    prev->next = current->next;
                }
				
				free(current->command);
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


// Extra option to function: 'killProcessByIdentifier'
void kill_process(pid_t pid) {
	if (kill(pid, SIGKILL) == -1) {
		perror("kill");
	}
}


// Function to kill a process by identifier (PID or command name)
void killProcessByIdentifier(struct Job** jobList, char** identifierArray) {
    char* identifier = identifierArray[0];

    if (strcmp(identifier, "-l") == 0) {
        // List of the signals
        printf("List of signals:\n");
        printf("1) SIGHUP\t2) SIGINT\t3) SIGQUIT\t4) SIGILL\t5) SIGTRAP\n");
        printf("6) SIGABRT\t7) SIGBUS\t8) SIGFPE\t9) SIGKILL\t10) SIGUSR1\n");
        printf("11) SIGSEGV\t12) SIGUSR2\t13) SIGPIPE\t14) SIGALRM\t15) SIGTERM\n");
        printf("16) SIGSTKFLT\t17) SIGCHLD\t18) SIGCONT\t19) SIGSTOP\t20) SIGTSTP\n");
        return;
    }

    // Check if the identifier is a PID or signal
    pid_t pid;
    int signalFlag = 0;

    // Check if the identifier starts with "-SIGKILL", "-SIGSTOP", or "-SIGCONT"
    if (strcmp(identifier, "-SIGKILL") == 0 || strcmp(identifier, "-SIGSTOP") == 0 || strcmp(identifier, "-SIGTSTP") == 0 ||  strcmp(identifier, "-SIGCONT") == 0 || strcmp(identifier, "-SIGINT") == 0 || strcmp(identifier, "-SIGTERM") == 0 || strcmp(identifier, "-SIGQUIT") == 0 || strcmp(identifier, "-SIGHUP") == 0) {
        identifier = identifierArray[1];
        signalFlag = 1;

        pid = atoi(identifier);
        printf("current pid: %d\n", pid);

        if (signalFlag) {
            // Handle the case where user inputs "-SIGKILL", "-SIGSTOP", or "-SIGCONT", "-SIGINT" with PID
            struct Job* current = findJobByPid(*jobList, identifier);

            if (current != NULL) {
                if (strcmp(identifierArray[0], "-SIGCONT") == 0) {
                    if (kill(current->pid, SIGCONT) == 0) {
		            	addJob(jobList, createJob(current->pid, current->pgid, identifier, 0));
		            	removeFromJobList(jobList, current->pid);
		    		}
                } else if (strcmp(identifierArray[0], "-SIGSTOP") == 0 || strcmp(identifierArray[0], "-SIGTSTP") == 0) {
                    if (kill(current->pid, SIGSTOP) == 0) {
                    	addJob(jobList, createJob(current->pid, current->pgid, identifier, 1));
                    }
                } else if (strcmp(identifierArray[0], "-SIGTERM") == 0) {
                    if (kill(current->pid, SIGTERM) == 0) {
                    	addJob(jobList, createJob(current->pid, current->pgid, identifier, 2));
                    	removeFromJobList(jobList, current->pid);
                    }
  				} else if (strcmp(identifierArray[0], "-SIGKILL") == 0) {
                    if (kill(current->pid, SIGKILL) == 0) {
                    	addJob(jobList, createJob(current->pid, current->pgid, identifier, 3));
                    	removeFromJobList(jobList, current->pid);
                    }
                } else if (strcmp(identifierArray[0], "-SIGINT") == 0) {
                	if (kill(current->pid, SIGINT) == 0) {
                		addJob(jobList, createJob(current->pid, current->pgid, identifier, 4));
                		removeFromJobList(jobList, current->pid);
                	} 
                } else if (strcmp(identifierArray[0], "-SIGHUP") == 0) {
                	if (kill(current->pid, SIGHUP) == 0) {
                		addJob(jobList, createJob(current->pid, current->pgid, identifier, 5));
                		removeFromJobList(jobList, current->pid);
                	} 
             	} else if (strcmp(identifierArray[0], "-SIGQUIT") == 0) {
                    if (kill(current->pid, SIGQUIT) == 0) {
                    	addJob(jobList, createJob(current->pid, current->pgid, identifier, 6));
                    	removeFromJobList(jobList, current->pid);
                    }
             	
             	} else {
                    	perror("kill");
                }
            } else {
                printf("bash: kill: %s: no such job\n", identifierArray[1]);
            }

            return;
      	}
    } else {
    	pid = atoi(identifier);
    }

    struct Job* current = findJobByPid(*jobList, identifier);

    if (current != NULL) {
        kill_process(current->pid);
        addJob(jobList, createJob(current->pid, current->pgid, identifier, 2));
        removeFromJobList(jobList, current->pid);
    } else {
        printf("bash: kill: %s: no such job\n", identifier);
    }
}


// Function to resume a background job by PID or command name
void resumeInBackground(struct Job** jobList, char* identifier) {
    if (identifier == NULL) {
        // If identifier is NULL, attempt to resume the most recent background job
        struct Job* lastJob = getLastJob(*jobList);
        if (lastJob != NULL) {
            pid_t pid = lastJob->pid;
            // Resume the last background job
            kill(pid, SIGCONT);
            lastJob->state = 0;  // Update job state to running
            printf("[%d]+  Running\t%s\n", getJobCount(*jobList), lastJob->command);
        } else {
            printf("No background jobs found.\n");
        }
        return;
    }
   	
    // Check if the identifier is a PID
    pid_t pid = atoi(identifier);
    //printf("1current pid: %d\n", pid);

    // Find the job with the given PID or command name
    struct Job* current = findJobByPid(*jobList, identifier);

    while (current != NULL) {
        if (current->pid == pid || (strcmp(current->command, identifier) == 0)) {
            printf("2Process pid: %d\n", current->pid);
            if (current->state == 1) {
                // Job is currently stopped, resume it in the background
                kill(pid, SIGCONT);
                current->state = 0;  // Update job state to running
                printf("[%d]+  Running\t%s\n", getJobCount(*jobList), current->command);
            } else {
                printf("[%d]+  Already running\t%s\n", getJobCount(*jobList), current->command);
            }
            return;
        }
        current = current->next;
    }

    printf("Job with identifier %s not found or not stopped.\n", identifier);
}


// Function to wait for a specific process to finish
void waitProcess(struct Job** jobList, pid_t pid) {    
    int status;
    
    waitpid(pid, &status, WUNTRACED);
    
    if (WIFEXITED(status)) {
        printf("[%d]+	Done\t%s\n", pid, (*jobList)->command);
    } else if (WIFSIGNALED(status)) {
        printf("[%d]+	Terminated\t%s\n", pid, (*jobList)->command);
    }
    
    removeFromJobList(jobList, pid);
}
