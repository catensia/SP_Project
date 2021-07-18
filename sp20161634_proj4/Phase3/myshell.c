/* $begin shellmain */
#include "myshell.h"
#include <stdbool.h>
#include <errno.h>
#define MAXARGS   128
#define MEMSET(string) memset(string, '\0', sizeof(string))

void print_jobs(){
    for(int i=0;i<total_jobs;i++){
        printf("[%d] %d Running     %s", i+1,jobs[i].pid, jobs[i].cmd);
    }
}

void add_job(struct job new_job){
    if(MAX_JOBS<=total_jobs+1){
        printf("job queue full.\n");
        return;
    }
    jobs[total_jobs++]=new_job;
}

void sigchld_ignore_job(int sig){
    int olderrno = errno;
    Sio_puts("brand new handler\n");
    cur_pid=waitpid(-1, NULL, 0);
    errno=olderrno;
}

void remove_job(pid_t pid){
    if(total_jobs==0) return;
    total_jobs--;
}

void sigint_handler(int sig){
    Sio_puts("CAUGHT SIGINT\n");

}


void sigchld_handler(int sig){
    int olderrno =errno;
    sigset_t lmask_all, lprev_all;
    pid_t pid;
    Sigfillset(&lmask_all);

    while((pid=waitpid(-1, NULL, 0))>0){
        Sigprocmask(SIG_BLOCK, &lmask_all, &lprev_all);
        remove_job(pid);
        Sio_puts("child terminate\n");
        Sigprocmask(SIG_SETMASK, &lprev_all, NULL);
    }
    Sio_puts("END OF ONE SIGNAL\n");
    if(errno!=ECHILD) Sio_error("waitpid error");
    errno=olderrno;
}

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    char *cwd; /* current working directory*/

    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);
    Signal(SIGCHLD, sigchld_handler); //installs signal handler - reaps child process, updates job queue
    //Signal(SIGINT, sigint_handler); //install signal handler that handles ctrl+c

    while (1) {
	    /* Read */
        setpgid(0,0);
        int status;
        printf("PROMPT: pid:%d pgid:%d\n", getpid(), getpgrp());
        cwd=getcwd(NULL, MAXLINE);
        printf("%s > ", cwd);                   
	    char *a = fgets(cmdline, MAXLINE, stdin); 
	    if (feof(stdin))
	        exit(0);

	    /* Evaluate */
	    eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    int fds[2];
    int argn;
    char carg[MAXLINE];
    MEMSET(carg);
    int job_fd[2], in_fd, out_fd;

    int p;
    if(p=pipe(job_fd)){
        printf("pipe error\n");
        exit(0);
    }

    if((p=pipe(fds))==-1){
        printf("pipe error\n");
        exit(0);
    }
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 

    argn=argnum(argv); //calculate argnum

    if (argv[0] == NULL) {
	    return;   /* Ignore empty lines */
    }



    if (!builtin_command(argv)) { 

        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one); //Blocks SIGCHLD
        
	    if (!bg){ 
	        int status;
            //Temporarily blocks child process signals since foreground jobs must end before receiving commands
            cur_pid=Fork();
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);
            switch(cur_pid){
                case -1: PERROR_EXIT("FORK ERROR\n");
                case 0: 
                    tcsetpgrp(0, getpgrp());
                    printf("NOTBG) pid:%d pgid:%d\n", getpid(), getpgrp());
                    pipe_execute(0, argv, STDIN_FILENO, argn, 0);
            } 

            wait(&status); //should wait for foreground process to terminate 
	    }
	    else{    
            
            cur_pid=Fork();
            Sigprocmask(SIG_SETMASK, &prev_one, NULL);
            setpgid(0,0);
            int pid;

            switch(cur_pid){
                case -1: PERROR_EXIT("FORK ERROR\n");
                case 0:
                    pid=getpid();
                    printf("FORK: %d\n", pid);
                    Write(job_fd[1], &pid, sizeof(pid));
                    pipe_execute(0, argv, STDIN_FILENO, argn, 0);
                    exit(0);
                
            }
            
        }
        Sigprocmask(SIG_BLOCK, &mask_all, NULL);
        int job_pid;
        Read(job_fd[0], &job_pid, sizeof(job_pid));
        struct job new_job;
        new_job.pid=job_pid;
        strcpy(new_job.cmd, cmdline);
        new_job.status=1;
        add_job(new_job);    
        close(job_fd[0]);
        close(job_fd[1]);
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);

    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if(!strcmp(argv[0], "jobs")){ /* jobs command */
        Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
        print_jobs();
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
        return 1;
    }

    if (!strcmp(argv[0], "quit")) /* quit command */
	    exit(0);  

    if(!strcmp(argv[0], "exit")) /* exit command */
        exit(0);

    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	    return 1;
    
    if (!strcmp(argv[0], "cd")){
        int chdir_ret;
        //only cd
        if(argv[1]==NULL){
            if((chdir_ret=chdir(getenv("HOME")))==-1){
                printf("cd home error\n");
            }
            return 1;
        }
        //cd ..     cd SP/proj4/
        if((chdir_ret=chdir(argv[1]))==-1){
            printf("cd error\n");
        }
        return 1;
    }

    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    if(argc>0){
        int lastlen=(int)strlen(argv[argc-1]);
        if(argc>=1 && argv[argc-1][lastlen-1]=='&'){
            bg=1;
            argv[argc-1][lastlen-1]='\0';
        }
    }
    return bg;
}
/* $end parseline */


int argnum(char **argv){
    for(int i=0;;i++){
        if(argv[i]=='\0'){
            return i;
        }
    }
}

int pipe_execute(int idx, char **argv, int fds, int argnum, int level){
    
    char t[MAXLINE];
    char *cmd[MAXARGS];
    int args=0;
    for(int i=0;i<argnum+1;i++) {
        cmd[i]=(char*)malloc(sizeof(char)*100);
        MEMSET(cmd[i]);
    }
    bool last=false;
    int ahead=idx;

    while(1){
        if(argv[ahead]==NULL){
            last=true; break;
        }
        if(!strcmp(argv[ahead], "|")) break;
        ahead++;
    }


    if(last){
        //terminating condition
        while(1){
            if(argv[idx]==NULL || argv[idx][0]=='\0') break;
            strcpy(cmd[args], argv[idx]);
            idx++; args++;
        }
        cmd[args]=NULL;


        if(!strcmp(cmd[0], "grep")){
            if(cmd[1]!=NULL && cmd[1][0]=='"'){

                if(cmd[args-1][strlen(cmd[args-1])-1]!='"') PERROR_EXIT("quotation error");

                for(int i=1;i<args;i++){
                    strcat(t, cmd[i]);
                    strcat(t, " ");
                }
                int l=(int)strlen(t);
                for(int i=0;i<l;i++){
                    t[i]=t[i+1];
                }
                t[l-3]='\0';
                strcpy(cmd[1], t);
                cmd[2]=NULL;
            }
        }
        
        if(fds!=STDIN_FILENO){
            Dup2(fds, STDIN_FILENO);
            Close(fds); 
        }
        
        if(execvp(cmd[0], cmd)<0){
            PERROR_EXIT("command not found\n");
        }
    }

    else {
        bool grep_flag=false;
        int temp_len=0;
        while (1) {
            if (argv[idx] == NULL || argv[idx][0] == '\0' || !strcmp(argv[idx], "|")) {
                break;
            }
            strcpy(cmd[args], argv[idx]);
            idx++; args++;
        }
        cmd[args]=NULL;
        idx++;
        if(!strcmp(cmd[0], "grep")){
            if(cmd[1]!=NULL && cmd[1][0]=='"'){
                
                if(cmd[args-1][strlen(cmd[args-1])-1]!='"') PERROR_EXIT("quotation error");

                for(int i=1;i<args;i++){
                    strcat(t, cmd[i]);
                    strcat(t, " ");
                }
                int l=(int)strlen(t);
                for(int i=0;i<l;i++){
                    t[i]=t[i+1];
                }
                t[l-3]='\0';

                strcpy(cmd[1], t);
                cmd[2]=NULL;
            }
        }


        int fd[2];
        if(pipe(fd)==-1) PERROR_EXIT("pipe error");
        int child_status;
        pid_t pid=Fork();
        
        if(pid==-1){
            PERROR_EXIT("fork error");
        }
        else if(pid==0){
            //spawn child process
            Close(fd[0]); //read from parameter fds, so close this 
            if(fds!=STDIN_FILENO){
                Dup2(fds, STDIN_FILENO); //read from fds
                Close(fds);
            }
            if(fd[1]!=STDOUT_FILENO){
                Dup2(fd[1], STDOUT_FILENO); //write to fd[1]
                Close(fd[1]);
            }
            if(execvp(cmd[0], cmd)<0){
                PERROR_EXIT("command not found\n");
            }
        }
        else{
            Close(fd[1]);
            Close(fds);
            int a = pipe_execute(idx, argv, fd[0], argnum, level+1);
        }

    }


}

static void PERROR_EXIT(const char *msg){
    perror(msg);
    exit(0);
}