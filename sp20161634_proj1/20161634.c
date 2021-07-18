#include "20161634.h"
#include "shell_cmd.h"
#include "mem_cmd.h"
#include "opcode.h"
#include "defines.h"
#define MAX_HASH 20

//head of linked list for history cmd
struct NODE *head=NULL;

//hash table pointer
struct NODE *hash[MAX_HASH];

//works as 1MB virtual memory 1byte * 2^20
unsigned char m[2<<20];
int dump_adr=0;


int main(void){
    //resets virtual memory with value
    memset(m, 0, sizeof(m));
    //reads from opcode.txt and initiates hash table
    init_hash(hash);
    while(1){
        start();
    }
    return 0;
}

/*---------------------------------------------------------------------*/
/*Function: hex*/
/*Purpose: reads a hexadecimal string and returns positive decimal value*/
/*Return value: if invalid -1 / if valid positive decimal value*/
/*---------------------------------------------------------------------*/
int hex(const char* num){
    if(strlen(num)==0) return -1;
    for(int i=0;i<strlen(num);i++){
        if(!((num[i]>='0' && num[i]<='9') || (num[i]>='a' && num[i]<='f') || (num[i]>='A' && num[i]<='F'))){
            return -1;
        }
    }
    int ret;
    sscanf(num, "%x", &ret);
    return ret;
}

/*---------------------------------------------------------------------*/
/*Function: argnum*/
/*Purpose: parses multi-word commands and calls appropriate functions depending on input*/
/*Return value: if invalid -1 / if valid 0 */
/*---------------------------------------------------------------------*/
int argnum(char* cmd){

    char what[100];
    //variables for start end value address
    int s,e,val, adr;
    //variable to check arguments
    int args=1;
    //checks validity of input
    int space=0, comma=0;

    //sets flag according to command
    bool editflag=false, dumpflag=false, fillflag=false;
    bool spaceflag=false;
    s=e=val=adr=-1;
    //string to add to history list
    char history_string[100];

    char op[100], mne[100], trash[100]; trash[0]='\0';
    int words=sscanf(cmd, "%s %s %s", op, mne, trash);

    if(words==2 && trash[0]=='\0' && !strcmp(op, "opcode")){
        //ex: opcode add
        //special case : need to check whether correct opcode, so add history inside function
        op_cmd(mne, hash, &head); return 0; 
    }

    //checks comma numbers
    for(int i=0;i<strlen(cmd);i++){
        if(!spaceflag && isspace(cmd[i])){
            space=i; spaceflag=true;
        }
        if(cmd[i]==',') comma++;
    }
    sscanf(cmd, "%s", what);
    char *tokenize=cmd;
    tokenize+=space;
    char *ptr;
    //decides command flags
    if(!strcmp("e", what)||!strcmp("edit", what)) editflag=true;
    if(!strcmp("du", what)||!strcmp("dump", what)) dumpflag=true;
    if(!strcmp("f", what)||!strcmp("fill", what)) fillflag=true;

    char hexStart[10], hexEnd[10],hexAddress[10], hexValue[10];

    //splits command by comma, and does boundary check and argument checks
    while((ptr=strsep(&tokenize,","))){
        int tempVal=hex(trim(ptr));
        //if invalid hexadecimal error
        if(tempVal==-1) return -1;
        if(editflag){
            //edit : check second argument range 00~FF
            //edit : first argument should be 00~FFFFF
            if(args==1 && 0<=tempVal && tempVal<=0xFFFFF){
                adr=tempVal; args++;
                strcpy(hexAddress, trim(ptr));
            }
            else if(args==2 && 0<=tempVal && tempVal<=0xFF){
                val=tempVal; args++;
                strcpy(hexValue, trim(ptr));
            }
            else return -1;
        }
        else if(dumpflag){
            //dump : all arguments should be 00~FFFFF
            if(0<=tempVal && tempVal<=0xFFFFF){
                if(args==1){
                    s=tempVal; args++;
                    strcpy(hexStart, trim(ptr));
                }
                else if(args==2){ 
                    e=tempVal; args++;
                    strcpy(hexEnd, trim(ptr));
                }
            }
            //if else invalid input ex: du 123A
            else return -1;
        }
        else if(fillflag){
            //fill : first and second argument (00~FFFFF) third argument (00~FF)
            //printf("tempval:%d\n", tempVal);
            if(args==1 && 0<=tempVal && tempVal<=0xFFFFF){
                s=tempVal; args++;
                strcpy(hexStart, trim(ptr));
            }
            else if(args==2 && 0<=tempVal && tempVal<=0xFFFFF){
                e=tempVal; args++;
                strcpy(hexEnd, trim(ptr));
            }
            else if(args==3 && 0<=tempVal && tempVal<=0xFF){
                val=tempVal; args++;
                strcpy(hexValue, trim(ptr));
            }
            else return -1;
        }
    }
    //command should be checked
    //all s,e,val should never be -1

    //comma number and argument number should match
    if(comma+2!=args) return -1;

    //call functions based on command and append to history list
    if(args==2 && (s!=-1) && (!strcmp(what, "du")||!strcmp(what,"dump")) ){
        sprintf(history_string, "%s %s", what, hexStart);
        history_add(history_string, &head);
        dump(m,1,s); return 0;
    }
    if(args==3 && (s!=-1 && e!=-1) &&  (!strcmp(what, "du")||!strcmp(what,"dump"))){
        sprintf(history_string, "%s %s, %s", what, hexStart,hexEnd);
        history_add(history_string, &head);
        dump(m,2,s,e); return 0;
    }
    if(args==4 && (s!=-1 && e!=-1 && val!=-1) && (!strcmp(what, "f")||!strcmp(what,"fill"))){
        sprintf(history_string, "%s %s, %s, %s", what, hexStart,hexEnd,hexValue);
        history_add(history_string, &head);
        fill(m,s,e,val); return 0;
    }
    if(editflag && args==3 && (adr!=-1 && val!=-1)){
        sprintf(history_string, "%s %s, %s", what, hexAddress, hexValue);
        history_add(history_string, &head);
        edit(m,adr, val); return 0;
    }

    return -1;
}


/*---------------------------------------------------------------------*/
/*Function: start*/
/*Purpose: reads user input and handles single word inputs*/
/*Return: void*/
/*---------------------------------------------------------------------*/
void start(void){

    printf("sicsim>");
    char buffer[MAX_INPUT];
    if(fgets(buffer, sizeof(buffer), stdin)){
        size_t len=strlen(buffer);
        if(len>0 && buffer[len-1]=='\n') buffer[len-1]='\0';
    }
    
    char *cmd=trim(buffer);
    if(strlen(cmd)==0) return;

    //one word inputs (trivial) - call appropriate functions
    if(!strcmp(cmd, "help") || !strcmp(cmd, "h")){
        shell_help();
        history_add(cmd, &head); return;
    }
    if(!strcmp(cmd, "dir") || !strcmp(cmd, "d")){
        shell_dir();
        history_add(cmd, &head); return;
    }
    if(!strcmp(cmd, "quit") || !strcmp(cmd, "q")){
        exit(0);
    }
    if(!strcmp(cmd, "history") || !strcmp(cmd, "hi")){
        history_add(cmd, &head);
        history_print(&head); return;
    }
    if(!strcmp(cmd, "du")||!strcmp(cmd,"dump")){
        dump(m,1, dump_adr); dump_adr+=160; 
        history_add(cmd, &head); return;
    }
    if(!strcmp(cmd, "opcodelist")){
        history_add(cmd, &head);
        op_list(hash); return;
    }
    if(!strcmp(cmd, "reset")){
        history_add(cmd, &head);
        reset(m); return;
    }

    //memory commands with multi word inputs
    if(argnum(cmd)==-1){
        printf("invalid input\n"); return;
    }

}
