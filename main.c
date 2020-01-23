#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define maxArgNumber 38

//-------------File flags for > type operations-------------
#define CREATE_FLAGS (O_WRONLY | O_TRUNC | O_CREAT )
//-------------File flags for >> type files--------------
#define CREATE_APPENDFLAGS (O_WRONLY | O_APPEND | O_CREAT )
//-------------File flags for input files-------------
#define CREATE_INPUTFLAGS (O_RDWR)
//-------------File flags for type mode--------------
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int argCount = 0;
char INPUT_FILE[20];
char OUTPUT_FILE[20];
char ERROR_FILE[20];

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

typedef struct historyStruct{//Struct for history command
    struct historyStruct *next;
    char args[maxArgNumber][20];
}HistoryStruct;

typedef struct path{//Struct for path command
    struct path *next;
    char path[15];
} PathStruct;

typedef struct backGroundProcessStruct{//Struct for background processes
    struct backGroundProcessStruct * next;
    pid_t id;
} BackgroundProcess;

HistoryStruct *historyHead = NULL;//Linked list headers
PathStruct *pathHead = NULL;
BackgroundProcess *bckHead = NULL;

int isForeground = 0;
pid_t foregroundID = 0;

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
            i,      /* loop index for accessing inputBuffer array */
            start,  /* index where beginning of next command parameter is */
            ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    //printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

//	for (i = 0; i <= ct; i++)
    //	printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

int commandRunner(char *args[]) {//For running commands with execv from enviroment paths

    const char* s = getenv("PATH");
    char  *token = strtok(s, ":");
    while(token != NULL) {
        char out[60];
        strcpy(out, token);
        strcat(out, "/");
        strcat(out, args[0]);
        execv(out,args);
        token = strtok(NULL, ":");
    }

    return -1;
}

int library(char *args[]){//For adding used commands to history  linked list

    HistoryStruct *newHead = malloc(sizeof(HistoryStruct));

    int i;
    for(i = 0; i < 38; i++) {

        if(args[i] == NULL)
            break;
        strcpy((newHead->args)[i],args[i]);

    }

    newHead->next = historyHead;
    historyHead = newHead;

}

void printHistory() {//for printing history

    int i;
    HistoryStruct *iter = historyHead;

    for(i = 0; i < 10; i++) {
        if(iter == NULL) break;

        printf("  %d ",i + 1);
        int j;
        for(j = 0; j < maxArgNumber;j++) {
            if(iter->args[j] == NULL) break;

            printf("%s ", iter->args[j]);
        }

        printf("\n");
        iter = iter->next;
    }
}

int historyRunner(int index) {//For running from history but it is not working
    printf("index : %d \n", index);
    int i;
    index = 1;
    HistoryStruct *iter;
    iter = historyHead;
    for(i = 1; i < index; i++)
        iter = iter->next;

    //printf("%s\n",iter->args[0]);
    char *args[MAX_LINE/2 + 1];
    /*for(i = 0;i < maxArgNumber; i++) {
        args[i] = iter->args[i];
    }*/

    /*for(i = 0; i < maxArgNumber; i++) {
        if (strcmp(args[i], "''"))
            printf("y\n");
        else printf("%s\n", args[i]);
    }*/
    printf("--%s--\n",(iter->args));
    commandRunner((iter->args));

    printf("olmadı");
    return -1;
}

static void zSigHandler(int signo) {//For handing ctrl-z signal
    if(isForeground == 1) {
        int status = kill(foregroundID, SIGKILL);
        if (status == -1)
            perror("Failed to send the SIGUSR1 signal to child");
        else {
            isForeground = 0;
        }
    }
}

void pathAdder(char path[]) {//For path + command to add new path to path linked list

    PathStruct *newHead = malloc(sizeof(PathStruct));

    strcpy(newHead->path, path);
    PathStruct *iter = pathHead;
    if(pathHead == NULL) {
        newHead->next = pathHead;
        pathHead = newHead;
    }
    else {
        while(iter != NULL) {
            if(iter->next == NULL) {
                iter->next = newHead;
                newHead->next = NULL;
                break;
            }
            else {
                iter = iter->next;
            }
        }
    }


}

void pathDelete(char path[]) {// For deleting path and all duplicates of it from path linked list

    PathStruct *before = pathHead;
    PathStruct *current = pathHead;

    while(current != NULL) {
        if(current == pathHead) {
            if(strcmp(current->path, path) == 0) {
                current = current->next;
                before = current;
                pathHead = current;
            }

            else {
                current = current->next;
            }
        }

        else if(current->next == NULL) {
            if(strcmp(current->path, path) == 0) {
                before->next = current->next;
                break;
            }

            else {
                before = current;
                current = current->next;
            }
        }

        else {
            if(strcmp(current->path, path) == 0) {
                before->next = current->next;
                current = before->next;
            }

            else {
                before = current;
                current = current->next;
            }
        }
    }

}

void pathPrinter() {// For printing whole path linked list with ":" in between every path

    PathStruct *head = pathHead;

    while(head != NULL) {
        printf("%s", head->path);
        head = head->next;
        if(head != NULL) printf(":");
    }
    printf("\n");

}

void backGroundAdder(pid_t id) {//For adding new background process to background linked list

    BackgroundProcess *newHead = malloc(sizeof(BackgroundProcess));
    newHead->id = id;
    newHead->next = bckHead;
    bckHead = newHead;
}

void backGroundRemover(pid_t id){//For removing from background linked list with given pid

    BackgroundProcess *before = bckHead;
    BackgroundProcess *current = bckHead;

    while(current != NULL) {

        if(current == bckHead) {
            if(current->id == id) {
                current = current->next;
                before = current;
                pathHead = current;
            }

            else {
                current = current->next;
            }
        }
        else if(current->next == NULL) {
            if(current->id == id) {
                before->next = current->next;
                break;
            }

            else {
                before = current;
                current = current->next;
            }
        }
        else {
            if(current->id == id) {
                before->next = current->next;
                current = before->next;
            }

            else {
                before = current;
                current = current->next;
            }
        }

    }
}

void backGroundSearcher(pid_t id) {//For fg command it moves background process to foreground

    BackgroundProcess *iter = bckHead;

    while(iter != NULL) {
        if(iter->id == id) {// checks if process is in background
            if(waitpid(id, NULL, WNOHANG) != 0) {//Checks if the process already died
                printf("This process is already died!\n");
                break;
            }

            else {
                if(kill(id, SIGSTOP) == -1) {//Try to stop it with signal
                    printf("Failed to stop backgroud process!\n");
                    break;
                }

                else {
                    if(kill(id, SIGCONT) == -1) {//Try to run it back on foreground
                        printf("Failed to run backgroud process in foreground!\n");
                        break;
                    }

                    else {
                        printf("Process caried to the foregroud!\n");
                        isForeground = 1;
                        foregroundID = id;
                        wait(id);//waits for it to die
                        break;
                    }
                }
            }
        }

        else iter = iter->next;
    }

    if(iter == NULL) {
        printf("There is no backgroud process with that pid!\n");
    }

}

int isThereOutput(char *args[]){//checks if we need to change standart output
    int i;
    for( i = 0; args[i]!=NULL ; i++){
        if( strcmp(args[i],">") == 0 ){
            strcpy(OUTPUT_FILE,args[i+1]);
            args[i]=NULL;//for not sending > and name of file as arg
            args[i+1]=NULL;
            return 1;
        }else if( strcmp(args[i],">>") == 0 ){//checks for appending
            strcpy(OUTPUT_FILE,args[i+1]);
            args[i]=NULL;//for not sending >> and name of file as arg
            args[i+1]=NULL;
            return 2;
        }
    }
    return 0;
}

int isThereInput(char *args[]){//does the same thing but looks for input
    int i;
    for( i = 0; args[i]!=NULL ; i++){
        if( strcmp(args[i],"<") == 0 ){
            strcpy(INPUT_FILE , args[i+1]);
            args[i]=NULL;//for not sending < and name of file as arg
            args[i+1]=NULL;
            return 1;
        }
    }
    return 0;
}

char isThereError(char *args[]){//checks if we need to change err output
    int i;
    for( i = 0; args[i]!=NULL ; i++){
        if( strcmp(args[i],"2>") == 0 ){
            strcpy(ERROR_FILE,args[i+1]);
            args[i]=NULL;//for not sending 2> and name of file as arg
            args[i+1]=NULL;
            return 1;
        }
    }
    return 0;
}

int main(void){

    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */

    struct sigaction act; /* to handle ^Z signal */
    act.sa_handler = zSigHandler;
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1) ||
        (sigaction(SIGTSTP, &act, NULL) == -1)) { perror("Failed to set SIGTSTP handler"); return 1;
    }

    while (1){
        isForeground = 0;
        background = 0;
        printf("myshell: ");
        fflush(NULL);// for clearing buffers
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);

        pid_t child = fork();//forking new child here

        if(background == 0) {//if process is going to work in foreground we update our global variables for ctrl z handling
            isForeground = 1;
            foregroundID = child;
        }

        if(child == -1) {
            perror("There is a problem and we can't fork a child!\n");
        }

        else if(child == 0) {//for child processes

            int i;
            for( i = 0; args[i]!=NULL ; i++)// we clear & from args
                if( strcmp(args[i],"&") == 0 )
                    args[i]=NULL;



            int out = isThereOutput(args);//flags for redirection
            int in = isThereInput(args);
            int error = isThereError(args);

            if( out!=0 || in !=0 || error != 0){//we check if we need to redirection I/O

                if(out==1){                                                  //Output part
                    int fd;
                    fd = open(OUTPUT_FILE,CREATE_FLAGS,CREATE_MODE);
                    if(fd == -1){
                        perror("Failed to open file");
                        return 1;
                    }
                    if(dup2(fd,STDOUT_FILENO) == -1){
                        perror("Failed to redirect standart output");
                        return 1;
                    }
                    if(close(fd) == -1){
                        perror("Failed to close the file");
                        return 1;
                    }
                }

                if(out==2){                                                  //Append output part
                    int fd;
                    fd = open(OUTPUT_FILE,CREATE_APPENDFLAGS,CREATE_MODE);
                    if(fd == -1){
                        perror("Failed to open file");
                        return 1;
                    }
                    if(dup2(fd,STDOUT_FILENO) == -1){
                        perror("Failed to redirect standart output");
                        return 1;
                    }
                    if(close(fd) == -1){
                        perror("Failed to close the file");
                        return 1;
                    }
                }

                if(in == 1){                                                  //Input part
                    int fd;
                    fd = open(INPUT_FILE,CREATE_INPUTFLAGS,CREATE_MODE);
                    if(fd == -1){
                        perror("Failed to open file");
                        return 1;
                    }
                    if(dup2(fd,STDIN_FILENO) == -1){
                        perror("Failed to redirect standart output");
                        return 1;
                    }
                    if(close(fd) == -1){
                        perror("Failed to close the file");
                        return 1;
                    }
                }

                if(error==1){                                                  //Error part
                    int fd;
                    fd = open(ERROR_FILE,CREATE_FLAGS,CREATE_MODE);
                    if(fd == -1){
                        perror("Failed to open file");
                        return 1;
                    }
                    if(dup2(fd,STDERR_FILENO) == -1){
                        perror("Failed to redirect standart output");
                        return 1;
                    }
                    if(close(fd) == -1){
                        perror("Failed to close the file");
                        return 1;
                    }
                }

            }

            if(strcmp(args[0], "history") == 0)  {// if command is history
                if(args[1] == NULL) {//without any arg we need the print history
                    printHistory();
                }

                else {
                    historyRunner(atoi(args[2]));//This function is for history -i but it is not working
                }
            }

            else if(strcmp(args[0], "path") == 0)  {// for path command

                if(args[1] != NULL) {
                    if(strcmp(args[1], "+") == 0) {//if second arg is + we add path to linked list
                        pathAdder(args[2]);

                    }

                    else if(strcmp(args[1], "-") == 0) {//if second arg is - we remove everey instance of it from linked list
                        pathDelete(args[2]);
                    }
                }

                else {//if path called without any arg we prit paths
                    pathPrinter();
                }
            }

            else if(strcmp(args[0],"exit") == 0) {//if commmand exit called, we exit from child and call this one from parent
                printf("Checking  if there is a background process!\n");
                exit(0);
            }

            else if(strcmp(args[0], "fg") == 0) {//if command fg called, we exit from child and call this one from parent
                printf("Searching for background with that pid!\n");
                exit(0);
            }

            else {//we run linux command
                commandRunner(args);
            }
        }

        else {// if process is parent
            if(background == 0)//waits for foreground process
                wait(child);
            else {//adds child to background and prints p_id of it
                printf("child id : %ld\n", child);
                backGroundAdder(child);
            }

            library(args);// adds command to library

            if(strcmp(args[0], "fg") == 0) {//carries selected process to foreground
                pid_t bckPid = atoi(args[1]);//for converting string to pid_t
                backGroundSearcher(bckPid);
            }

            if(strcmp(args[0], "exit") == 0) {//for exit command
                pid_t ret = waitpid(-1, NULL, WNOHANG);

                if(ret == 0) {//if there is background process we cannot close the system
                    printf("There might be background process, we cannot terminate the system!\n");
                }

                else {
                    printf("Terminating the system!\n");;
                    exit(0);
                }
            }
        }
        /** the steps are:
        (1) fork a child process using fork()
        (2) the child process will invoke execv()
        (3) if background == 0, the parent will wait,
        otherwise it will invoke the setup() function again. */
    }
}
