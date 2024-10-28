#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	MAX_SIZE_INPUT	256
#define	MAX_SIZE_ARGV	20


char *argv[MAX_SIZE_ARGV];
char stream[MAX_SIZE_INPUT];
int tokenSize;
pid_t pid;
int HISTORYCAPACITY = 1;
int HISTORYSIZE = 0;
char** history;

void shell();
void run_batch();
void get_input ();
void tokenize_input();
void atomicCommand();
void handle_operators();
void andOp(int);
void redirection(char**);
int isRedirection(char**, int);
void addToHistory(char*);
void printHistory(int);
int max(int, int);


int main(){
    history = (char**)malloc(sizeof(char*) * HISTORYCAPACITY);
	shell();
	return 0;
}


void shell() {
    while (1){
        get_input();
        if(!strcmp("quit", stream)) break;
        addToHistory(stream);
        tokenize_input();
        if(isRedirection(argv, tokenSize) == 1)
        {
            redirection(argv);
        }
        else if(tokenSize == 2 && !(strcmp("batch_mode", argv[0])))
            run_batch();
        else if(tokenSize == 2 && !strcmp("print_history", argv[0])){
            int topCommands = atoi(argv[1]);
            printHistory(topCommands);
        }
        else {
            handle_operators();
        }
    }
}


void get_input () {
   char cwd[256];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("%s:%% ", cwd);
   } else {
       perror("getcwd() error");
   }
   fgets(stream, MAX_SIZE_INPUT, stdin);
   if ((strlen(stream) > 0) && (stream[strlen (stream) - 1] == '\n'))
        	stream[strlen (stream) - 1] = '\0';
}


void tokenize_input() {
	char *token;
	int i = 0;
	token = strtok(stream, " ");
	while(token != NULL){
		argv[i] = token;
		i++;
		token = strtok(NULL, " ");}
    argv[i] = NULL;
    tokenSize = i;

}


void andOp(int i){
    int status;
    argv[i] = NULL;
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        exit(1);
    } else {
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) != 0) {
            return;
        }
        pid = fork();
        if (pid == 0) {
            execvp(argv[i + 1], &argv[i + 1]);
        } else {
            waitpid(pid, &status, 0);
        }
    }
}


void orOp(int i){
    int status;
    argv[i] = NULL;
            pid_t pid = fork();
            if (pid == 0) {
                execvp(argv[0], argv);
                exit(1);
            } else {
                waitpid(pid, &status, 0);
                if (WEXITSTATUS(status) == 0) {
                    return;
                }
                pid = fork();
                if (pid == 0) {
                    execvp(argv[i + 1], &argv[i + 1]);
                } else {
                    waitpid(pid, &status, 0);
                }
            }
            return;
        }


void semiCommaOp(int i){

argv[i] = NULL;
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
    } else {
        wait(NULL);
        pid = fork();
        if (pid == 0) {
            execvp(argv[i + 1], &argv[i + 1]);
        } else {
            wait(NULL);
        }
    }}


void atomicCommand(){
    pid = fork();

    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "Error: The command does not exist or cannot be executed\n");

    } else if (pid > 0) {
        waitpid(pid, NULL, 0);

    } else {
        printf("Failed");
    }
}


void run_batch(){

    FILE* file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stderr, "Error: File %s does not exist \n", argv[1]);
    }
    while (fgets(stream, MAX_SIZE_INPUT, file)) {
        if ((strlen(stream) > 0) && (stream[strlen (stream) - 1] == '\n'))
            stream[strlen (stream) - 1] = '\0';
        addToHistory(stream);
        if(!strcmp("quit", stream)) break;
        tokenize_input();
        printf("Command: %s\n", stream);
        handle_operators();
    }
    fclose(file);
}


void redirection(char** args){

    char *cp;
    char *ifile = NULL, *ofile = NULL;
    int i = 0, j = 0;
    char** args2 = malloc(MAX_SIZE_ARGV * sizeof(char *));

    while (1) {
        cp = args[i++];
        if (cp == NULL)
            break;

        switch (*cp) {
        case '<':

            if (cp[1] == 0)
                cp = args[i++];
            else
                ++cp;

            ifile = cp;

            if (cp == NULL)
                return;
            else if (cp[0] == 0)
                return;

            break;

        case '>':
            if (cp[1] == 0)
                cp = args[i++];
            else
                ++cp;

            ofile = cp;

            if (cp == NULL)
                return;
            else if (cp[0] == 0)
                return;
            break;

        default:
            args2[j++] = cp;
            break;
        }
    }

    args2[j] = NULL;

    if (j == 0)
        return;

    switch (pid = fork()) {
    case 0:
        if (ifile != NULL) {
            int fd = open(ifile, O_RDONLY);

            if (dup2(fd, STDIN_FILENO) == -1) {
                fprintf(stderr, "dup2 failed");
            }
            close(fd);
        }

        if (ofile != NULL) {
            int fd2;
            if ((fd2 = open(ofile, O_WRONLY | O_CREAT, 0644)) < 0) {
                perror("couldn't open output file.");
                exit(0);
            }
            dup2(fd2, STDOUT_FILENO);
            close(fd2);
        }
        execvp(args2[0], args2);
        signal(SIGINT, SIG_DFL);
        fprintf(stderr, "Error: The command does not exist or cannot be executed\n");
        exit(1);
        break;

    case -1:
        fprintf(stderr, "ERROR can't create child process!\n");
        break;

    default:
        wait(NULL);
    }

}


int isRedirection(char** args, int size){
    int i;
    for(i = 0;i < size;i++)
        if (!strcmp( args[i] , ">")|| !strcmp(args[i] , "<"))
            return 1;
    return 0;
}


void addToHistory(char* commandToSave){
    if(HISTORYSIZE >= HISTORYCAPACITY){
        HISTORYCAPACITY *= 2;
        char** tmp = (char**)malloc(sizeof(char*) * HISTORYCAPACITY);
        int i = 0;
        for(;i < HISTORYSIZE; i++){
            tmp[i] = malloc((MAX_SIZE_INPUT) * sizeof(char));
            tmp[i] = history[i];
        }
        free(history);
        history = tmp;
    }
    history[HISTORYSIZE] = malloc((MAX_SIZE_INPUT) * sizeof(char));
    strcpy(history[HISTORYSIZE],commandToSave);
    HISTORYSIZE++;
}


void printHistory(int top){
    int i = HISTORYSIZE - 1;
    printf("------------ start of history ------------\n");
    for(; i >= max(HISTORYSIZE - top, 0);i--)
        printf("%s\n",history[i]);
    printf("------------ end of history ------------\n");
}


void handle_operators() {
    int i;
    int status;
    for (i = 0; i < tokenSize; i++) {
        if (!strcmp("&&", argv[i])) {
            andOp(i);
            return;
        } else if (!strcmp("|", argv[i])) {
            int fd[2];
            pid_t pid;
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // child process
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                argv[i] = NULL;
                execvp(argv[0], argv);
                exit(EXIT_FAILURE);
            } else {
                // parent process
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                execvp(argv[i + 1], &argv[i + 1]);
                exit(EXIT_FAILURE);
            }
        } else if (!strcmp(";", argv[i])) {
            semiCommaOp(i);
            return;
        } else if (!strcmp("||", argv[i])) {
            orOp(i);
            return;
        }
    }
    pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
    } else {
        wait(NULL);
    }
}


int max(int a, int b){
     return (a > b) ? a : b;}
