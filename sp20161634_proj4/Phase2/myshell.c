/* $begin shellmain */
#include "csapp.h"
#include <stdbool.h>
#include <errno.h>
#define MAXARGS   128
#define MEMSET(string) memset(string, '\0', sizeof(string))


/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
int pipe_execute(int idx, char **argv, int fds, int argnum,  int level);
int argnum(char **argv);
int calc_pipe_num(char **argv);
static void PERROR_EXIT(const char *msg);

int pipe_num;



int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    char *cwd; /* current working directory*/
    int child_status;

    while (1) {
	    /* Read */
        cwd=getcwd(NULL, MAXLINE);
        printf("%s > ", cwd);                   
	    char *a = fgets(cmdline, MAXLINE, stdin); 
	    if (feof(stdin))
	        exit(0);

	    /* Evaluate */
	    eval(cmdline);
        wait(&child_status);
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
    pid_t pid;           /* Process id */
    int fds[2];          // file descriptors
    int argn;            // temporary buffer to parse
    char carg[MAXLINE];

    MEMSET(carg);
    
    int p;
    if((p=pipe(fds))==-1){
        printf("pipe error\n");
        exit(0);
    }

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 

    pipe_num=calc_pipe_num(argv);
    argn=argnum(argv); //calculate argnum


    if (argv[0] == NULL) {
	    return;   /* Ignore empty lines */
    }

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run

        pid=Fork();
        if(pid==-1) PERROR_EXIT("fork error");
        else if(pid==0){
            //child process
            int temp  = pipe_execute(0, argv, STDIN_FILENO, argn, 0);
        }
	    

        /* Parent waits for foreground job to terminate */
	    if (!bg){ 
	        int status;
	    }
	    else//when there is backgrount process!
	        printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
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

    return bg;
}
/* $end parseline */

int calc_pipe_num(char **argv){
    int res=0;
    for(int i=0;;i++){
        if(argv[i]=='\0') return res;
        if(!strcmp(argv[i], "|")) res++;
    }
    return res;
}

//finds argumnet numbers
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
    //create arguments set for each pipe to execute
    int args=0;
    for(int i=0;i<argnum+1;i++) {
        cmd[i]=(char*)malloc(sizeof(char)*100);
        MEMSET(cmd[i]);
    }
    bool last=false;
    int ahead=idx;

    //check whether current funtion is the last to process
    //EX: sort -r when command is [cat filename | grep "abc" | sort -r]
    while(1){
        if(argv[ahead]==NULL){
            last=true; break;
        }
        if(!strcmp(argv[ahead], "|")) break;
        ahead++;
    }


    if(last){
        //grab arguments until end of command line
        while(1){
            if(argv[idx]==NULL || argv[idx][0]=='\0') break;
            strcpy(cmd[args], argv[idx]);
            idx++; args++;
        }
        cmd[args]=NULL;

        //if argument is a grep, deal with quotation marks
        if(!strcmp(cmd[0], "grep")){
            if(cmd[1]!=NULL && cmd[1][0]=='"'){
                
                //if only one quotation mark at the front, mark as error
                if(cmd[args-1][strlen(cmd[args-1])-1]!='"') PERROR_EXIT("quotation error");

                //keep grabbing arguments until end of quotation mark
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
        
        execvp(cmd[0], cmd);
    }

    //if not final process 
    else {
        bool grep_flag=false;
        int temp_len=0;
        while (1) {
            //keep grabbing arguments until it meets a pipe
            if (argv[idx] == NULL || argv[idx][0] == '\0' || !strcmp(argv[idx], "|")) {
                break;
            }
            strcpy(cmd[args], argv[idx]);
            idx++; args++;
        }
        cmd[args]=NULL;
        idx++;
        //again, check for quotation marks (same with above code block)
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
            execvp(cmd[0], cmd);
        }
        else{
            Close(fd[1]);
            Close(fds);
            //send file descriptor as argument for recursive function
            int a = pipe_execute(idx, argv, fd[0], argnum, level+1);
        }

    }


}

static void PERROR_EXIT(const char *msg){
    perror(msg);
    exit(0);
}