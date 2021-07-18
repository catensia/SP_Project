#include "shell_cmd.h"

/*---------------------------------------------------------------------*/
/*Function: trim*/
/*Purpose: removes leading and trailing spaces in string*/
/*Return: char pointer of trimmed string*/
/*---------------------------------------------------------------------*/
char* trim(char *cmd){
    //trims leading and trailing spaces
    while(isspace(*cmd))cmd++;
    if(*cmd==0) return cmd;

    int end=strlen(cmd)-1;
    while(isspace(cmd[end])){
        end--;
    }
    cmd[end+1]='\0';

    return cmd;
}
/*---------------------------------------------------------------------*/
/*Function: checkExe*/
/*Purpose: checks if given file name is executable*/
/*Return: true if executable, false if not executable */
/*---------------------------------------------------------------------*/
bool checkExe(const char *name){
    struct stat sb;
    if (stat(name, &sb) == 0 && sb.st_mode & S_IXUSR){
        /* executable */
        return true;
    }
    else return false;
}


/*---------------------------------------------------------------------*/
/*Function: checkDir*/
/*Purpose: checks if given file name is directory*/
/*Return: true if directory / false if not directory */
/*---------------------------------------------------------------------*/
bool checkDir(const char* name){
    //checks if given file name is a directory
    struct stat status;
    stat(name,&status);
    return S_ISDIR(status.st_mode);
}

/*---------------------------------------------------------------------*/
/*Function: shell_help*/
/*Purpose: Print available commands*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void shell_help(void){
    printf("h[elp]\n");
    printf("d[ir]\n");
    printf("q[uit]\n");
    printf("du[mp] [start, end]\n");
    printf("e[dit] address, value\n");
    printf("f[ill] start, end, value\n");
    printf("reset\n");
    printf("opcode mnemonic\n");
    printf("opcodelist\n");
    printf("assemble filename\n");
    printf("type filename\n");
    printf("symbol\n");
}

/*---------------------------------------------------------------------*/
/*Function: shell_dir*/
/*Purpose: prints files/directories in current directory*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void shell_dir(void){
    DIR *dir_i=opendir(".");
    struct dirent *dir_e;
    if(dir_i!=NULL){
        while((dir_e=readdir(dir_i))){
            char filename[100];
            //copy file name
            strcpy(filename, dir_e->d_name);
            int len=strlen(filename);
            //check if execution file (.out)
            if(checkExe(dir_e->d_name)){
                filename[len]='*';
                filename[len+1]='\0';
            }
            //check if directory
            if(checkDir(dir_e->d_name)){
                filename[len]='/';
                filename[len+1]='\0';
            }
            printf("%s\n", filename);
        }
    }
    closedir(dir_i);
}

/*---------------------------------------------------------------------*/
/*Function: history_add*/
/*Purpose: adds command to history linked list*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void history_add(char* command, struct NODE **head){
    struct NODE *newNode=(struct NODE*)malloc(sizeof(struct NODE));
    strcpy(newNode->cmd, command);
    newNode->next=NULL;
    struct NODE *cur;

    if(*head==NULL){
        *head=newNode; return;
    }
    else{
        for(cur=(*head);cur->next!=NULL;cur=cur->next){}
            cur->next=newNode;
    }
}

/*---------------------------------------------------------------------*/
/*Function: history_print*/
/*Purpose: prints contents of linked list */
/*Return: none*/
/*---------------------------------------------------------------------*/
void history_print(struct NODE **head){

    if(*head==NULL) return;
    //prints contents of linked list
    int cnt=1;
    for(struct NODE *cur=(*head);cur!=NULL;cur=cur->next, cnt++){
        printf("%d %s\n", cnt, cur->cmd);
    }
}


/*---------------------------------------------------------------------*/
/*Function: type_filename*/
/*Purpose: prints contents of file*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void type_filename(const char *filename, struct NODE **head){
    if(checkDir(filename)) {
        printf("%s is a directory. Abort\n", filename); return;
    }
    FILE *fp;
    if((fp=fopen(filename, "r"))){
        char content=fgetc(fp);
        while(content!=EOF){
            printf("%c", content);
            content=fgetc(fp);
        }
        printf("\n");
        fclose(fp);
        char history_string[100];
        sprintf(history_string, "type %s", filename);
        history_add(history_string, head);
        return;
    }
    printf("File doesn't exist\n"); return;
}

/*---------------------------------------------------------------------*/
/*Function: symbol_print*/
/*Purpose: prints symbol table in lexicographical order*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void symtab_print(struct NODE **head){
    if(*head==NULL) return;
    for(struct NODE *cur=(*head);cur!=NULL;cur=cur->next){
        printf("\t%s\t%04X\n", cur->cmd, cur->num);
    }
}
