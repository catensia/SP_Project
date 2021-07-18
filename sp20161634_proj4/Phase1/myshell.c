/* $begin shellmain */
#include "csapp.h"
#include <errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 

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
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL) {
	    return;   /* Ignore empty lines */
    }

    char newargv[MAXLINE]={"/bin/"};
    strcat(newargv, argv[0]);

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if((pid=Fork())==0){
            if (execve(newargv, argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
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


