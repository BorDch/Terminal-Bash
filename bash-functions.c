#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void doubleStickExec(char*** words) {
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execvp(words[0][0], words[0]);
        perror("execvp");
        _exit(1);
    }
    
    else if (pid1 == -1) {
        perror("fork");
        _exit(1);
    }

    int status;
    wait(&status);
    if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            execvp(words[1][0], words[1]);
            perror("execvp");
            _exit(1);
        }
        
        else if (pid2 == -1) {
            perror("fork");
            _exit(1);
        }

        wait(NULL);
    }
}

void doubleAmperndExec(char*** words) {
    pid_t pid1 = fork();
    if (pid1 == 0) {
        execvp(words[0][0], words[0]);
        perror("execvp");
        _exit(1);
    }
    
    else if (pid1 == -1) {
        perror("fork");
        _exit(1);
    }

    int status;
    wait(&status);
    if (WIFEXITED(status) == 1 && WEXITSTATUS(status) == 0) {
        pid_t pid2 = fork();
        if (pid2 == 0) {
       	    execvp(words[1][0], words[1]);
            perror("execvp");
            _exit(1);
        }
        
        else if (pid2 == -1) {
            perror("fork");
            _exit(1);
        }

        wait(NULL);
    }
}
