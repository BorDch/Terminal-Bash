#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "bash_func.h"


void clearJobs(struct Job** jobList); 

// Function for creating a new Job
struct Job* createJob(pid_t pid, pid_t pgid, char* command, int state, struct Command* commands) {
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
    job->commands = commands;
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


void executeInBackground(struct Command* cmd, struct Job** jobList) {
    if (cmd == NULL) {
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGKILL, SIG_DFL);
        setpgid(0, 0);
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
    	cmd->pid = pid;
    	printf("Process with id [%d]\n", pid);
    	    	
    	struct Job* job = createJob(pid, pid, cmd->words[0], 0, cmd);
    	
    	addJob(jobList, job);


        // Return control of the terminal to the parent process
       	tcsetpgrp(STDIN_FILENO, getpgrp());
    }
}


void executeDefault(struct Command* cmd, struct Job** jobList, struct History** historyList) {
    if (cmd == NULL) {
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        signal(SIGTSTP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGCONT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGKILL, SIG_DFL);
        cmd->pid = pid;
        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        freeCommand(&cmd);
        clearHistory(historyList);
        exit(EXIT_FAILURE);
    } else {
        int status;
        setpgid(0, 0);
        waitpid(pid, &status, WUNTRACED);

        if (WIFEXITED(status)) {
            // Process exited successfully
            
        } else if (WIFSTOPPED(status)) {
            // CTRL + Z pushed
            printf("\n[%d] %s Stopped\n", pid, cmd->words[0]);
            struct Job* job = createJob(pid, pid, cmd->words[0], 1, cmd);
            addJob(jobList, job);
        } else {
            printf("\n%s: execution error\n", cmd->words[0]);
        }
    }
}


void executePipeline(struct Command* cmd, struct Job** jobList) {
    int fd[2];
    int prev_fd = 0;

    pid_t first_cmd_pid = 0;

    while (cmd != NULL) {
        if (pipe(fd) == -1) {
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // Child process
            // First process PID in pipeline setting as GROUP PID
            if (getpid() == first_cmd_pid || first_cmd_pid == 0) {
                setpgid(0, 0);
            }

            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTERM, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGCONT, SIG_DFL);
            signal(SIGHUP, SIG_DFL);
            signal(SIGKILL, SIG_DFL);

            if (prev_fd != 0) {
                cmd->pid = pid;
                if (dup2(prev_fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(prev_fd);
            }

            if (cmd->next != NULL) {
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(fd[1]);
            }

            close(fd[0]);

            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else { // Parent process
            close(fd[1]);

            if (prev_fd != 0) {
                close(prev_fd);
            }

            if (first_cmd_pid == 0) {
                first_cmd_pid = pid;
            }

            int status;
            waitpid(pid, &status, WUNTRACED);

            if (WIFSTOPPED(status)) {
            	printf("First job-cmd pid: %d\n", first_cmd_pid);
                printf("\n[%d] %s Stopped\n", pid, cmd->words[0]);
                struct Job* job = createJob(pid, first_cmd_pid, cmd->words[0], 1, cmd);
                addJob(jobList, job);
            }

            prev_fd = fd[0];
        }

        cmd = cmd->next;
    }
}


// Pipeline for commands with background processes
void PipelineBackground(struct Command* cmd, struct Job** jobList) {
    int fd[2];
    int prev_fd = 0;
    pid_t last_cmd_pid;
    pid_t first_cmd_pid = -1;
    char* full_expression = NULL;
    struct Command* last_cmd = NULL;
    
    // Pipe for getting first command pid
    int pipe_pid[2];

    // Traverse the pipeline and set up redirections
    while (cmd->next != NULL) {
        if (pipe(fd) == -1) {
            perror("pipe");
            exit(1);
        }
        
	// Pipe for getting first command pid
        if (pipe(pipe_pid) == -1) {
    	   perror("pipe");
    	   exit(1);
        }

        if (full_expression == NULL) {
            full_expression = strdup(cmd->words[0]);
        } else {
            char* temp = strdup(full_expression);
            free(full_expression);
            full_expression = malloc(strlen(temp) + strlen(cmd->words[0]) + 4);
            strcpy(full_expression, temp);
            strcat(full_expression, " | ");
            strcat(full_expression, cmd->words[0]);
            free(temp);
        }

        pid_t pid = fork();
        int is_first_procces = 1;
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // Child process
            if (prev_fd != 0) {
                if (dup2(prev_fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }
                close(prev_fd);
            }
            
            if (first_cmd_pid == -1) {
            	first_cmd_pid = getpid();
            	
            	close(pipe_pid[0]);
            	write(pipe_pid[1], &first_cmd_pid, sizeof(first_cmd_pid));
            	close(pipe_pid[1]);
            } 

            if (dup2(fd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(fd[0]);
            close(fd[1]);

            execvp(cmd->words[0], cmd->words);
            perror("execvp");
            exit(1);
        } else { // Parent process
            close(fd[1]);

            if (prev_fd != 0) {
                close(prev_fd);
            }
            
            if (!is_first_procces) {
            	wait(NULL);
            } else {
		is_first_procces = 0;
	    }
		
            
            close(pipe_pid[1]);
            read(pipe_pid[0], &first_cmd_pid, sizeof(first_cmd_pid));
            close(pipe_pid[0]);

            prev_fd = fd[0];
            last_cmd = cmd;  // Запоминаем последнюю команду
        }
        
	
        cmd = cmd->next;
    }

    // Build the full expression
    if (full_expression == NULL) {
        full_expression = strdup(cmd->words[0]);
    } else {
        char* temp = strdup(full_expression);
        free(full_expression);
        full_expression = malloc(strlen(temp) + strlen(cmd->words[0]) + 4);
        strcpy(full_expression, temp);
        strcat(full_expression, " | ");
        strcat(full_expression, cmd->words[0]);
        free(temp);
    }

    // Execute the last command in the background
    last_cmd_pid = fork();
    if (last_cmd_pid == -1) {
        perror("fork");
        exit(1);
    } else if (last_cmd_pid == 0) { // Child process
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGCONT, SIG_DFL);
	signal(SIGHUP, SIG_DFL);
	signal(SIGKILL, SIG_DFL);
        if (prev_fd != 0) {
            if (dup2(prev_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            close(prev_fd);
        }

        execvp(cmd->words[0], cmd->words);
        perror("execvp");
        exit(1);
    } else { // Parent process
        // Create node only for last command
        printf("First job-cmd pid: %d\n", first_cmd_pid);
        setpgid(first_cmd_pid, first_cmd_pid);
        
        
        printf("Process with id [%d]\n", last_cmd_pid);
        addJob(jobList, createJob(last_cmd_pid, first_cmd_pid, full_expression, 0, last_cmd));
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }

    free(full_expression);
}


void executeCommand(struct Command* cmd, struct Job** jobList, struct History** historyList, int firstOperatorFlag, int secondFlag) {
    if (cmd == NULL) {
        return;
    }

    // Check if word is '#' then won't execute following commands
    if (strcmp(cmd->words[0], "#") == 0) {
        cmd->next = NULL;
        return;
    }

    // Check the command's flag and execute accordingly
    if ((firstOperatorFlag == 3 && secondFlag == 4) || (firstOperatorFlag == 4 && secondFlag == 3)) {
        executeCommandSequence(cmd);
    } else if (firstOperatorFlag == 1 && secondFlag == 2) { // Pipeline + Background ('&')
    	PipelineBackground(cmd, jobList);
    
    } else if (firstOperatorFlag == 1 || (cmd->flag == 1)) { // Pipe ('|')
        executePipeline(cmd, jobList);

    } else if (firstOperatorFlag == 2 || (cmd->flag == 2)) { // Background ('&')
    	executeInBackground(cmd, jobList);

    } else if (firstOperatorFlag == 3 || (cmd->flag == 3)) { // Logical OR ('')
        executeOrOperator(cmd);

    } else if (firstOperatorFlag == 4 || (cmd->flag == 4)) { // Logical AND ('&&')
        executeAndOperator(cmd);

    } else if (firstOperatorFlag == 5 || (cmd->flag == 5)) { // Sequential execution (';')
        executeSeqOperator(cmd);

    } else if (firstOperatorFlag == 6 || (cmd->flag == 6)) { // Redirect output to file '>'
        struct Command* next_cmd = cmd->next;
        if (next_cmd != NULL) {
            next_cmd->filename = next_cmd->words[0];
            outputInFile(cmd, next_cmd->filename);
        }

    } else if (firstOperatorFlag == 7 || (cmd->flag == 7)) { // Append data to the end of the file '>>'
        struct Command* next_cmd = cmd->next;
        if (next_cmd != NULL) {
            next_cmd->filename = next_cmd->words[0];
            appendToFile(cmd, next_cmd->filename);
        }

    } else if (firstOperatorFlag == 8 || (cmd->flag == 8)) { // Output file contents to the stream '<'
        struct Command* next_cmd = cmd->next;
        if (next_cmd != NULL) {
            next_cmd->filename = next_cmd->words[0];
            inputFromFile(cmd, next_cmd->filename);
        }

    } else { // Default, no operator
        executeDefault(cmd, jobList, historyList);
    }
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
        if (current != NULL || current->command != NULL) {
        	printf("Group ID: [%d] ---- Process ID: [%d] %s\t%s\n", current->pgid, current->pid, status, current->command);
        }
        current = current->next;
    }
}

// Function for printing all Jobs
void printJobs(struct Job* jobList) {
    if (jobList == NULL) {
        printf("No jobs.\n");
    } else {
        printJobsList(jobList);
    }
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


// Function to find a job by group PID
struct Job* findJobByPGID(struct Job* jobList, pid_t pgid) {
    struct Job* currentJob = jobList;
    while (currentJob != NULL) {
        if (currentJob->pgid == pgid) {
            return currentJob;
        }
        currentJob = currentJob->next;
    }
    return NULL; // If Node was not found
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
	    //current->commands = NULL;	
	
            return;
        }

        prev = current;
        current = current->next;
    }
}


void updateJobList(struct Job** jobList) {
    struct Job* current = *jobList;
    struct Job* prev = NULL;

    while (current != NULL) {
        int all_jobs_done = 1; // Checking flag for all jobes' status in the Node
        struct Job* temp = current; // Temporary pointer for every job in the Node

        while (temp != NULL) {
            int status;
            pid_t result = waitpid(temp->pid, &status, WNOHANG | WUNTRACED);
            
            /* If there is unexpected error happened, for example: job's order execution error. 
               Then clear the Job List and recreate the right order. 
               
               !!! All jobs should be executed in correct order !!! */
            if (result == -1) {
                perror("waitpid");
                
                struct Job* temp = *jobList;
                while (temp != NULL) {
                	kill(temp->pid, SIGKILL);
                	temp = temp->next;
                }
                clearJobs(jobList);
                return;
            } else if (result == 0 || (WIFSTOPPED(status) && temp->state)) {
                // If some job in JobNode is still running or it was stopped, then change the flag's value and break from check cycle 
                all_jobs_done = 0;
                break;
            } else if (WIFEXITED(status)) printf("[%d]+  Done\t%s\n", temp->pid, temp->command);
            else if (WIFSIGNALED(status) && (temp->state == 2 || temp->state == 3 || temp->state == 4 || temp->state == 5 || temp->state == 6)) {
                // If job got the signal, the print description about it
                if (temp->state == 2) printf("[%d]+  Terminated\t%s\n", temp->pid, temp->command);
                else if (temp->state == 3) printf("[%d]+  Killed\t%s\n", temp->pid, temp->command);
                else if (temp->state == 4) printf("[%d]+  Interrupted\t%s\n", temp->pid, temp->command);
                else if (temp->state == 5) printf("[%d]+  Hangup\t%s\n", temp->pid, temp->command);
                else if (temp->state == 6) printf("[%d]+  Quited\t%s\n", temp->pid, temp->command);
                
                break;
            }

            temp = temp->next;
        }

        if (all_jobs_done) {
        // All jobs in the Node are done or got signal, then delete the jobNode
           if (prev == NULL) { 
		   // Current job is the first in the list
		   struct Job* nextNode = current->next;
		   *jobList = current->next;
		   free(current->command);
	   	   free(current);
	   	   *jobList = nextNode;
		   current = nextNode;
	   } else {
	   	prev->next = current->next;
	   	free(current->command);
	   	free(current);
	   	current = prev->next;
	   }	
        } else {
            // Not All jobs in the Node are done or got signal, then get to the next node
            prev = current;
            current = current->next;
        }
    }
}


// Function to bring a background job to the foreground by PID or command name
void bringToForeground(struct Job** jobList, char* identifier) {

    if (identifier == NULL) {
        // If identifier is NULL, bring the last job to the foreground
        struct Job* lastJob = getLastJob(*jobList);
        if (lastJob != NULL) {
	    tcsetpgrp(STDIN_FILENO, lastJob->pid);
            kill(lastJob->pid, SIGCONT);
            int status;
            waitpid(lastJob->pid, &status, WUNTRACED);

            if (WIFEXITED(status)) {
            	printf("\n[%d]+    Done\t\t%s\n", lastJob->pid, lastJob->command);
            	removeFromJobList(jobList, lastJob->pid);
            } else if(WIFSIGNALED(status)) {
            	printf("\n[%d]+    Terminated\t\t%s\n", lastJob->pid, lastJob->command);
            	removeFromJobList(jobList, lastJob->pid);
            
            } else if (WIFSTOPPED(status)) {
		    // CTRL + Z pushed
		    printf("\nStopped\n");
		    lastJob->state = 1;
            } else {
            	printf("\n%s: execution error\n", lastJob->commands->words[0]);
            }
        } else {
            printf("No background jobs to bring to foreground\n");
        }
        tcsetpgrp(STDIN_FILENO, getpgrp());
        return;
    }

    struct Job* current = *jobList;

    // Find the job by PID or command name
    while (current != NULL) {
        if (current->pid == atoi(identifier) || strcmp(current->command, identifier) == 0) {
            tcsetpgrp(STDIN_FILENO, current->pid);
	    kill(current->pid, SIGCONT);
            int status;
            waitpid(current->pid, &status, WUNTRACED);
            
            if (WIFEXITED(status)) {
            	printf("\n[%d]+    Done\t\t%s\n", current->pid, current->command);
            	removeFromJobList(jobList, current->pid);
            } else if(WIFSIGNALED(status)) {
            	printf("\n[%d]+    Terminated\t\t%s\n", current->pid, current->command);
            	removeFromJobList(jobList, current->pid);
            
            } else if (WIFSTOPPED(status)) {
		    // CTRL + Z pushed
		    printf("\nStopped\n");
		   current->state = 1;
            } else {
            	printf("\n%s: execution error\n", current->commands->words[0]);
            }
	    tcsetpgrp(STDIN_FILENO, getpgrp());
            return;
        }

        current = current->next;
    }

    printf("Job with PID or command name '%s' not found\n", identifier);
}

// Function to move a job to the background by PID or command name
void resumeInBackground(struct Job** jobList, char* identifier) {
    if (identifier == NULL) {
        struct Job* lastJob = getLastJob(*jobList);
        if (lastJob != NULL) {
            pid_t pid = lastJob->pid;
            //tcsetpgrp(STDIN_FILENO, getpgrp());
            kill(pid, SIGCONT);
            lastJob->state = 0;
            printf("[%d]+  Running\t%s\n", getJobCount(*jobList), lastJob->command);
        } else {
            printf("No background jobs found.\n");
        }
        return;
    }

    pid_t pid = atoi(identifier);

    struct Job* current = findJobByPid(*jobList, identifier);

    if (current != NULL) {
       if (current->pid == pid) {
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
    }

    printf("Job with identifier %s not found or not stopped.\n", identifier);
}


// Function to kill a process by identifier (PID or command name)
void killProcessByIdentifier(struct Command* commands, struct Job** jobList, char** identifierArray) {
    char* identifier = identifierArray[0];
    char* group_pid = identifierArray[1];

    if (strcmp(identifier, "-l") == 0) {
        // List of the signals
        printf("List of signals:\n");
        printf("1) SIGHUP\t2) SIGINT\t3) SIGQUIT\t4) SIGILL\t5) SIGTRAP\n");
        printf("6) SIGABRT\t7) SIGBUS\t8) SIGFPE\t9) SIGKILL\t10) SIGUSR1\n");
        printf("11) SIGSEGV\t12) SIGUSR2\t13) SIGPIPE\t14) SIGALRM\t15) SIGTERM\n");
        printf("16) SIGSTKFLT\t17) SIGCHLD\t18) SIGCONT\t19) SIGSTOP\t20) SIGTSTP\n");
        return;
    } else if (strcmp(identifier, "-g") == 0) {
	printf("'-g': kill signal to the job group\n");
	int pgid = atoi(identifierArray[1]);
	struct Job* jobNode = findJobByPGID(*jobList, pgid);
	
	if (jobNode == NULL) {
		printf("bash: kill: no such job\n");
    		return;
    	} else {
    		struct Job* currentJob = jobNode;
		while (currentJob != NULL) {
		    if (currentJob->pid != 0) {
		   	if (currentJob->state != 1) {
		        	kill(currentJob->pid, SIGTERM);
		        	currentJob->state = 2;
		        } else {
		       		kill(currentJob->pid, SIGCONT);
		       		kill(currentJob->pid, SIGTERM);
		       		currentJob->state = 2;
		       	}
		        printf("Sent SIGTERM signal to process with PID %d\n", currentJob->pid);
		    }
		    currentJob = currentJob->next;
		}
	}
    		
	return;
    } else if (*group_pid == '-') {
    	printf("'-pid': kill signal to the job group\n");
	int pgid = atoi(identifierArray[1]);
	int sig_num = atoi(identifier);
	int state;
	pgid = abs(pgid);
	struct Job* jobNode = findJobByPGID(*jobList, pgid);
	
	
	if (sig_num == 1) state = 5;
        else if (sig_num == 2) state = 4;
        else if (sig_num == 3) state = 6;
        else if (sig_num == 9) state = 3;
        else if (sig_num == 15) state = 2;
        else if (sig_num == 18) state = 0;
        else if (sig_num == 19 || sig_num == 20) state = 1;
	
	if (jobNode == NULL) {
		printf("bash: kill: no such job\n");
    		return;
    	} else {
    		struct Job* currentJob = jobNode;
		while (currentJob != NULL) {
		    if (currentJob->pid != 0) {
		   	if (currentJob->state != 1) {
		        	kill(currentJob->pid, sig_num);
		        	currentJob->state = state;
		        } else {
		       		kill(currentJob->pid, SIGCONT);
		       		kill(currentJob->pid, sig_num);
		       		currentJob->state = state;
		       	}

		        printf("Sent %s signal to process with PID %d\n", strsignal(sig_num), currentJob->pid);
		    }
		    currentJob = currentJob->next;
		}
	}
    	return;
    }
    	
    
    int sig_num = atoi(identifierArray[0]);
    int pid = atoi(identifierArray[1]);
    int state;
    
    if (sig_num == 1) state = 5;
    else if (sig_num == 2) state = 4;
    else if (sig_num == 3) state = 6;
    else if (sig_num == 9) state = 3;
    else if (sig_num == 15) state = 2;
    else if (sig_num == 18) state = 0;
    else if (sig_num == 19 || sig_num == 20) state = 1;
    
        
    struct Job* job = findJobByPid(*jobList, identifierArray[1]);
    
    if (job == NULL) {
    	printf("bash: kill: no such job\n");
    	return;
    } else {
    	if (kill(job->pid, sig_num) == -1) {
    		perror("kill");
    	}
    	job->state = state;
    }
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

