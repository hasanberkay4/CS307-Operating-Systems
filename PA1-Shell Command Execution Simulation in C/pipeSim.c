#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(){
    printf("I'm SHELL process, with PID: %d - Main command is: man ping | grep \"\\\\-v\" -A 2\n", getpid());

    int fileDesc[2];
    pipe(fileDesc);

    int manFork =  fork();

    if(manFork < 0){    // Meaning fork failed just exit
        fprintf(stderr, "First fork failed, Exiting...\n");
        exit(1);
    }

    else if(manFork == 0){    // It is the child  where man command will be executed
        printf("I'm MAN process, with PID: %d - My command is: man ping\n", getpid());

        int grepFork = fork();    // Grandchild that will conduct grep operation

        if(grepFork < 0){    // Same fork check for grandchild
            fprintf(stderr, "Second fork failed, Exiting...\n");
            exit(1);
        }

        else if(grepFork == 0){    // Grandchild goes this path
            printf("I'm GREP process, with PID: %d - My command is: grep \"\\\\-v\" -A 2\n", getpid());
            dup2(fileDesc[0], STDIN_FILENO);    // Grep should read from anon file where man output is written

            // Grep should output to output.txt file

            close(fileDesc[1]); // Closing this will make process to write stdout which is file at the end

            int myFile = open("output.txt", O_CREAT | O_APPEND | O_WRONLY, 0666);
            dup2(myFile, STDOUT_FILENO);    // directed output to opened file

            char* argumentList[] = {"grep", "\\-v", "-A", "2", NULL};    // to escape special character double escape is required. (Searched from stackoverflow
            execvp("grep", argumentList);
        }

        else{    //   First child goes this path and executes man execution
            dup2(fileDesc[1], STDOUT_FILENO);    // Move  man output to anon file
	    char* argumentList[] = {"man", "ping", NULL};
            execvp("man", argumentList);
        }
    }

    else{    // Again came back to shell process
        wait(NULL);    // First and grandchild should be done before coming back to SHELL
        printf("I'm SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", getpid());
    }

    return 0;
}
