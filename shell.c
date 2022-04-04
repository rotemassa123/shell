//
// Created by student on 15/11/2021.
//
#include <signal.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "unistd.h"
#include <wait.h>
#include <errno.h>
#include <fcntl.h>

void sigal_handle(int sig) {
    //if the errno is equal to 10 we don't want to crash because its missing child Error.
    if(waitpid(-1,NULL,WNOHANG) < 0 && errno != 10) {
        fprintf(stderr, "Error in sig process %s", strerror(errno));
    }
}

int prepare(void){
    // Take care of zombies
    struct sigaction sigal;
    memset(&sigal,0,sizeof(sigal));
    sigal.sa_handler = sigal_handle;
    sigal.sa_flags = SA_RESTART;
    sigaction(SIGCHLD,&sigal,NULL);
    signal(SIGINT,SIG_IGN);
    return 0;
}


//process and run two commands simultaneously
int pipe_handler(int count,int index, char** arglist){
    int error_index = 0;
    char** com1 = arglist + index + 1;
    int piping[2];
    if (pipe(piping) == -1){
        fprintf(stderr,"Error in piping %s",strerror(errno));
        return 0;
    }
    int pid1 = fork();

    if (pid1 < 0) {
        fprintf(stderr,"Error in piping %s",strerror(errno));
        return 0;
    }
        //first son
    else if (pid1 == 0){
        fprintf(stderr,"A");
        signal (SIGINT,SIG_DFL);
        error_index = close(piping[0]);
        if (error_index < 0) {
            fprintf(stderr,"Error in closing pipe first son %s",strerror(errno));
            exit(1);
        }
        error_index = dup2(piping[1],1);
        if (error_index < 0) {
            fprintf(stderr,"Error in dup2 %s",strerror(errno));
            exit(1);
        }
        error_index = close(piping[1]);
        if (error_index < 0) {
            fprintf(stderr,"Error in closing pipe %s",strerror(errno));
            exit(1);
        }
        error_index = execvp(arglist[0], arglist);
        if (error_index < 0) {
            fprintf(stderr,"Error in command execution %s",strerror(errno));
            exit(1);
        }
    }
        //in father
    else{


        //Splitting from the father to the second son
        int pid2 = fork();
        if (pid2 == -1) {
            fprintf(stderr,"Error in second Fork %s",strerror(errno));
            return 0;
        }
            //second son
        else if (pid2 == 0){
            fprintf(stderr,"B");
            signal (SIGINT,SIG_DFL);
            error_index = dup2(piping[0], 0);
            if (error_index < 0) {
                fprintf(stderr,"Error in dup2 %s",strerror(errno));
                exit(1);
            }
            error_index = close(piping[1]);
            if (error_index < 0) {
                fprintf(stderr,"Error in closing pipe %s",strerror(errno));
                exit(1);
            }
            error_index = close(piping[0]);
            if (error_index < 0) {
                fprintf(stderr,"Error in closing pipe %s",strerror(errno));
                exit(1);
            }
            error_index = execvp(com1[0], com1);
            if (error_index < 0) {
                fprintf(stderr,"Error in command execution %s",strerror(errno));
                exit(1);
            }
        }
            //waits until the both of the sons will finnish
        else{
            fprintf(stderr,"C");

            error_index = waitpid(pid1, NULL, WNOHANG);
            if (error_index < 0) {
                fprintf(stderr,"Error in first waitpid %s",strerror(errno));
                return 0;
            }
            fprintf(stderr,"D");
            error_index = waitpid(pid2, NULL, WNOHANG);
            if (error_index < 0) {
                fprintf(stderr,"Error in second waitpid %s",strerror(errno));
                return 0;
            }
            fprintf(stderr,"E");
            error_index = close(piping[0]);
            if (error_index < 0) {
                fprintf(stderr,"Error in closing pipe %s",strerror(errno));
                return 0;
            }
            error_index = close(piping[1]);
            if (error_index < 0) {
                fprintf(stderr,"Error in closing pipe %s",strerror(errno));
                return 0;
            }

        }
    }
    return 1;


}
//creating
int output_redirect(int count,int index, char** arglist){
    int error_index;
    int pid = fork(); //split
    if (pid < 0){
        printf("Error in Fork creation");
        return 0;
    }
    if(pid == 0){
        signal (SIGINT,SIG_DFL);
        int op = open(arglist[index+1],O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (op == -1){
            fprintf(stderr,"Error in open source file %s",strerror(errno));
            exit(1);
        }
        error_index = dup2(op,1);
        if(error_index < 0){
            fprintf(stderr,"Error in dup %s",strerror(errno));
            exit(1);
        }
        error_index = execvp(arglist[0], arglist);
        if(error_index == -1){
            fprintf(stderr,"Error in execvp process %s",strerror(errno));
            exit(1);
        }
    }
    else{
        error_index = waitpid(pid, NULL, WNOHANG);
        if(error_index <  0){
            fprintf(stderr,"Error in waitpid process %s",strerror(errno));
            return 0;
        }
    }
    return 0;
}

//handle and deals with commands. in case of & proccecing the command in the background.
int standard_handler(int count,char** arglist){
    int error_index;
    int amper_check = strcmp(arglist[count - 1], "&");
    if(amper_check == 0){
        arglist[count - 1] = NULL;
    }
    int pid = fork(); //split
    if (pid < 0){
        fprintf(stderr,"Error in Fork process %s",strerror(errno));
        return 0;
    }
    else if (pid == 0){
        if(amper_check != 0){
            signal (SIGINT,SIG_DFL);
        }
        error_index = execvp(arglist[0], arglist);
        if(error_index == -1){
            fprintf(stderr,"Error in execvp process stan %s",strerror(errno));
            exit(1);
        }
    }
    if(amper_check != 0){
        error_index = wait(0);
        if (error_index < 0) {
            fprintf(stderr,"Error in waitpid %s",strerror(errno));
            return 0;
        }
    }
    return 1;
}



int process_arglist(int count, char** arglist) {
    int i = 0;
    while(i<count-1){
        if (strcmp(arglist[i], "|") == 0) {
            arglist[i] = NULL;
            pipe_handler(count,i, arglist);
            i = count;
        }
        i++;
    }
    if(count > 1 && strcmp(arglist[count - 2], ">") == 0){
        arglist[count - 2] = NULL;
        output_redirect(count,count - 2, arglist);
        i = count;
    }
    else if(i != count){
        standard_handler(count,arglist);
    }

    return 1;
}

int finalize(){
    return 0;
}

