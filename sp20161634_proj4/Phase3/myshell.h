#include "csapp.h"

#define MAX_JOBS 30
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
int pipe_execute(int idx, char **argv, int fds, int argnum,  int level);
int argnum(char **argv);
int calc_pipe_num(char **argv);
static void PERROR_EXIT(const char *msg);

struct job{
    int id;
    int pgid;
    int pid;
    char cmd[50];
    int status;
};

volatile sig_atomic_t pid;
pid_t cur_pid;
sigset_t mask_all, mask_one, prev_one;
int total_jobs=0;
struct job jobs[MAX_JOBS];