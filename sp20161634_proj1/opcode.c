#include "opcode.h"
#define MAX_HASH 20

void history_add(char* command, struct NODE **head);

/*---------------------------------------------------------------------*/
/*Function: calc_key*/
/*Purpose: calculates hash function*/
/*Return: integer value of hash key*/
/*---------------------------------------------------------------------*/
int calc_key(const char*cmd){
    int key=0;
    for(int i=0;i<strlen(cmd);i++){
        key+=(int)cmd[i];
    }
    return key%MAX_HASH;
}

/*---------------------------------------------------------------------*/
/*Function: addHash*/
/*Purpose: adds a node to hash table*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void addHash(struct NODE *node, struct NODE *hash[]){
    int key=calc_key(node->cmd);

    if(hash[key]==NULL){
        hash[key]=node;
    }
    else{
        node->next=hash[key];
        hash[key]=node;
    }
}

/*---------------------------------------------------------------------*/
/*Function: init_hash*/
/*Purpose: reads opcode.txt and makes hash table*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void init_hash(struct NODE *hash[]){
    FILE *fp = fopen("opcode.txt", "r");

    int cmd_num;
    char op[10];
    char format[10];
    while(EOF!=fscanf(fp,"%x %s %s", &cmd_num, op, format)){
        struct NODE *newNode=(struct NODE*)malloc(sizeof(struct NODE));
        strcpy(newNode->cmd, op);
        strcpy(newNode->format, format);
        newNode->num=cmd_num;
        newNode->next=NULL;
        addHash(newNode, hash);
    }
    
}

/*---------------------------------------------------------------------*/
/*Function: op_cmd*/
/*Purpose: takes mnemonic input and prints corresponding opcode*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void op_cmd(char *mnemonic, struct NODE *hash[], struct NODE **head){
    int key=calc_key(mnemonic);
    char history_string[100];
    struct NODE*cur=hash[key];
    while(cur!=NULL){
        if(!strcmp(cur->cmd, mnemonic)){
            sprintf(history_string,"opcode %s", mnemonic);
            history_add(history_string, head);
            printf("opcode is %X\n", cur->num);
            return;
        }
        cur=cur->next;
    }
    printf("no such opcode\n"); return;
}


/*---------------------------------------------------------------------*/
/*Function: op_list*/
/*Purpose: Prints all opcodes in format*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void op_list(struct NODE *hash[]){

    for(int i=0;i<MAX_HASH;i++){
        printf("%2d : ", i);
        if(hash[i]!=NULL){
            struct NODE *cur=hash[i];
            while(cur!=NULL){
                printf("[%s, %X] ", cur->cmd, cur->num);
                cur=cur->next;
                if(cur!=NULL){
                    printf(" -> ");
                }
            }
        }
        printf("\n");
    }
}