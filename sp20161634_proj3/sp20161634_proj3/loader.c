#include "loader.h"
#include "mem_cmd.h"
#include "shell_cmd.h"
#include "opcode.h"
#define CSECT 0
#define SYMBOL 1
#define PLUS 0
#define MINUS 1
#define MEMSET(string) memset(string, '\0', sizeof(string));

struct NODE *head;
struct ESTNODE *est_head;
struct NODE *hash[MAX_HASH];
int csaddr;
int execaddr;
int proglength;
int *bp;
int bp_num=0;
int bp_idx=0;
int bp_cu=0;
int BASE=0x33;
bool next_CC=false;
int CC=0; //'=' 0 '>:1' '<:-1'

struct REGISTERS{
    int A;  int X;
    int L;  int PC;
    int B;  int S;
    int T;
};

struct REGISTERS reg;

char REFTAB[20][20]={0};
unsigned char m[2<<20];


/*---------------------------------------------------------------------*/
/*Function: print_regs*/
/*Purpose: prints contents of registers*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void print_regs(void){
    printf("A : %06X  X : %06X\n", reg.A, reg.X);
    printf("L : %06X  PC: %06X\n", reg.L, reg.PC);
    printf("B : %06X  S : %06X\n", reg.B, reg.S);
    printf("T : %06X\n", reg.T);
}

/*---------------------------------------------------------------------*/
/*Function: print_bp*/
/*Purpose: prints breakpoints*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void print_bp(){
    printf("           breakpoint\n");
    printf("           ----------\n");
    for(int i=0;i<bp_num;i++){
        printf("           %X\n", bp[i]);
    }
}

/*---------------------------------------------------------------------*/
/*Function: clear_bp*/
/*Purpose: clears all breakpoints*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void clear_bp(){
    bp_num=0;
    reg.A=reg.X=reg.PC=reg.B=reg.S=reg.T=0;
    reg.L=proglength;
    free(bp);
}

/*---------------------------------------------------------------------*/
/*Function: add_bp*/
/*Purpose: adds breakpoints*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void add_bp(int new_bp){
    if(bp_num==0){
        if((bp=(int*)malloc(sizeof(int)))==NULL){
            printf("malloc error.\n");
        }
        bp_num++;
    }
    else{
        bp_num++;
        if((bp=(int*)realloc(bp,sizeof(int)*bp_num))==NULL){
            printf("realloc error.\n");
        }
    }
    bp[bp_num-1]=new_bp;
}

/*---------------------------------------------------------------------*/
/*Function: decHex*/
/*Purpose: converts 4byte word to 3byte word*/
/*Return: hexadecimal*/
/*---------------------------------------------------------------------*/
int decHex(const char *hex){
    char neg[10]="FF";
    char pos[10]="00";
    int ret;

    if(hex[0]==9 || hex[0]==8 || (hex[0]>='A' && hex[0]<='F')){
        //negative
        strcat(neg, hex);
        sscanf(neg, "%X", &ret);
    }
    else{
        strcat(pos, hex);
        sscanf(pos, "%X", &ret);
    }
    return ret;
}

/*---------------------------------------------------------------------*/
/*Function: file_exists*/
/*Purpose: checks if input file exists*/
/*Return: true if exists, false if not exists*/
/*---------------------------------------------------------------------*/
bool file_exists(char *name){
    struct stat temp;
    return (stat(name, &temp)==0);
}

/*---------------------------------------------------------------------*/
/*Function: destroy_EST*/
/*Purpose: destroys ESTAB*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void destroy_EST(struct ESTNODE **est_head){
    struct ESTNODE* cur=*est_head;
    struct ESTNODE* next;
    while(cur!=NULL){
        next=cur->next;
        free(cur);
        cur=next;
    }
    *est_head=NULL;
}

/*---------------------------------------------------------------------*/
/*Function: print_EST*/
/*Purpose: prints ESTAB*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void print_EST(struct ESTNODE **est_head){
    proglength=0;
    printf("control symbol address length\n");
    printf("section name\n");
    printf("--------------------------------\n");
    for(struct ESTNODE *cur=(*est_head);cur!=NULL;cur=cur->next){
        if(cur->length==-1){
            printf("%s\t%s\t%04X\n", cur->csect, cur->symbol, cur->address);
        }
        else{
            printf("%s\t%s\t%04X\t%04X\n", cur->csect, cur->symbol, cur->address, cur->length);
            proglength+=cur->length;
        }
    }
    printf("--------------------------------\n");
    printf("          total length  %04X\n", proglength);
}

/*---------------------------------------------------------------------*/
/*Function: EST_search*/
/*Purpose: searches ESTAB for given CSECT or SYMBOL*/
/*Return: corresponding address*/
/*---------------------------------------------------------------------*/
int EST_search(const char *name, int type, struct ESTNODE **est_head){
    //if 0-csect 1-symbol
    for(struct ESTNODE *cur=(*est_head);cur!=NULL;cur=cur->next){
        
        if(type==CSECT){
            if(!strcmp(name, cur->csect)){
                return cur->address;
            }
        }
        else{
            if(!strcmp(name, cur->symbol)){
                return cur->address;
            }
        }
    }
    return -1;
}

/*---------------------------------------------------------------------*/
/*Function: EST_add*/
/*Purpose: adds to ESTAB*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void EST_add(char *csect, char *symbol, int address, int length, int type,  struct ESTNODE **est_head){
    struct ESTNODE *newNode=(struct ESTNODE*)malloc(sizeof(struct ESTNODE));
    strcpy(newNode->csect, csect);
    strcpy(newNode->symbol, symbol);
    newNode->address=address;
    newNode->length=length;
    newNode->type=type;
    newNode->next=NULL;
    struct ESTNODE *cur;
    if(*est_head==NULL){
        *est_head=newNode;
    }
    else{
        for(cur=(*est_head);cur->next!=NULL;cur=cur->next){}
        cur->next=newNode;
    }


}

/*---------------------------------------------------------------------*/
/*Function: pass1*/
/*Purpose: proceeds pass 1 of loader*/
/*Return: 1 on success/ -1 on fail*/
/*---------------------------------------------------------------------*/
int pass1(char *filename){
    int CSLTH;
    FILE *file=fopen(filename, "r");
    int n;
    char buffer[100];
    char temp[100]; int num;

    fgets(buffer, sizeof(buffer), file);
    sscanf(buffer, "%s %x", temp, &num);
    n=strlen(temp);
    for(int i=0;i<n;i++) temp[i]=temp[i+1];
    CSLTH=num;

    //search ESTAB for control section name
    if(EST_search(temp, CSECT, &est_head)!=-1){
        printf("Error: duplicate external symbol\n");
        return -1;
    }
    else
        EST_add(temp, "", csaddr, num, CSECT, &est_head);
    
    while(1){
        fgets(buffer, sizeof(buffer), file);
        if(buffer[0]=='E'){
            break;
        }
        if(buffer[0]=='D'){
            n=strlen(buffer);
            for(int i=0;i<n;i++)
                buffer[i]=buffer[i+1];
            char *s=buffer;
            while(sscanf(s,"%12[^\n]s", temp)==1){
                s+=12;
                char *t=(char*)malloc(sizeof(char)*7);
                strncpy(t, temp+6, 6);
                sscanf(t, "%X", &num);
                
                for(int i=0;i<6;i++){
                    if(temp[i]==' '){
                        t[i]='\0';
                        break;
                    }
                    t[i]=temp[i];
                }

                if(EST_search(t, SYMBOL, &est_head)!=-1){
                    printf("Error: duplicate external symbol\n");
                    return -1;
                }
                else
                    EST_add("", t, csaddr+num, -1, SYMBOL, &est_head);
                free(t);
            }
        }
    }
    
    
    csaddr+=CSLTH;
    fclose(file);
    return 0;
}
/*---------------------------------------------------------------------*/
/*Function: pass2*/
/*Purpose: proceeds pass 2 of */
/*Return: 1 on success -1 on fail*/
/*---------------------------------------------------------------------*/
int pass2(char *filename){

    int CSLTH;
    FILE *file=fopen(filename, "r");
    char buffer[100];
    char temp[100]; int num,n;
    


    fgets(buffer, sizeof(buffer), file);
    sscanf(buffer, "%s %x", temp, &num);
    n=strlen(temp);
    for(int i=0;i<n;i++) temp[i]=temp[i+1];

    strcpy(REFTAB[1], temp);
    CSLTH=num;


    while(1){
        fgets(buffer, sizeof(buffer), file);
        if(buffer[0]=='E'){
            if(strlen(buffer)!=1){
                int END_ADDR=0;
                sscanf(buffer, "E%06X", &END_ADDR);
                execaddr=csaddr+END_ADDR;
            }
            break;
        }
        
        if(buffer[0]=='R'){
            char *t=buffer+1;
            int offset;
            while(sscanf(t, "%d%s%n", &num, temp, &offset)==2){
                t+=offset;
                strcpy(REFTAB[num], temp);
            }
        }

        if(buffer[0]=='T'){
            MEMSET(temp);
            char *t=buffer+1;
            strncpy(temp, t, 6);
            int target, len;
            //T000020
            sscanf(temp, "%x", &target);
            target+=csaddr;
            t+=6;

            MEMSET(temp);
            strncpy(temp, t, 2);
            sscanf(temp, "%x", &len);
            t+=2;

            for(int i=0;i<len;i++){
                MEMSET(temp);
                strncpy(temp, t,2);
                sscanf(temp, "%X", &num);
                t+=2;
                edit(m, target+i, num);
            }

        }
        if(buffer[0]=='M'){
            MEMSET(temp);
            char *t=buffer+1;
            strncpy(temp, t, 6);
            int target, len;
            sscanf(temp, "%x", &target);
            t+=6;

            MEMSET(temp);
            strncpy(temp, t, 2);
            sscanf(temp, "%x", &len);
            t+=2;

            int sign;
            if(*t=='+') sign=PLUS;
            else sign=MINUS;

            t+=1;
            MEMSET(temp);
            strncpy(temp, t, 2);
            int ref;
            sscanf(temp, "%d", &ref);

            target+=csaddr;
            MEMSET(temp);
            sprintf(temp, "%02X%02X%02X", m[target], m[target+1], m[target+2]);
            
            int value=decHex(temp);
            int EST_addr;
            if(ref==1)
                EST_addr=EST_search(REFTAB[ref], CSECT, &est_head);
            else EST_addr=EST_search(REFTAB[ref], SYMBOL, &est_head);


            if(sign==PLUS)
                value+=EST_addr;
            if(sign==MINUS)
                value-=EST_addr;
            
            char word[10];
            char *h=word;
            char byte[3]; int n;
            sprintf(h, "%08X", value);
            h+=2;
            for(int i=0;i<3;i++){
                strncpy(byte, h,2);
                sscanf(byte, "%X", &n);
                edit(m, target+i,n);
                h+=2;
            }

            
        }
    }
    csaddr+=CSLTH;
    return 0;
}

/*---------------------------------------------------------------------*/
/*Function: run*/
/*Purpose: runs program*/
/*Return: 1 on success -1 on fail*/
/*---------------------------------------------------------------------*/
int run(void){
    bool break_flag=false;
    bool prog_end=false;
    int operand;
    while(1){
        bool PC_edit=false;
        if(execaddr>=progaddr+proglength){
            prog_end=true;
            break;
        }

            for(int i=0;i<bp_num;i++){
                if(execaddr==bp[i]){
                    bp_cu++;
                    break_flag=true;
                    print_regs();
                    printf("Stop at checkpoint[%X]\n", execaddr);
                    break;
                }
            
        }

        int opcode= m[execaddr] & 0b11111100;
        //n,i,x,b,p,e;
        int n=(m[execaddr]&0b00000010)>>1;
        int i=(m[execaddr]&0b00000001);
        int x=(m[execaddr+1]>>7) & 1;
        int b=(m[execaddr+1]>>6) & 1;
        int p=(m[execaddr+1]>>5) & 1;
        int e=(m[execaddr+1]>>4) & 1;
        operand=0;
        x=x;
        //1=extended 0=not extended
        
        int format=get_format(opcode, hash);
        

        int len;
        if(opcode==0x44 || opcode==0xF0 || opcode==0x04){
            //BYTE directive
            if(opcode==0x44) len=3;
            else len=1;
        }
        else if(m[execaddr]==0x00){
            //empty reserved space by RESW or RESB
            len=1;
        }
        else if(format==1){
            //format 1
            len=1;
        }
        else if(format==2){
            //format 2
            len=2;
            operand=m[execaddr+1];
        }
        else if(format==3){
            //format 3/4
            if(e==1) {
                len=4;
                operand=operand|m[execaddr+3];
                operand=operand|(m[execaddr+2]<<8);
                operand=operand|((m[execaddr+1]%16)<<16);

            }
            else{ 
                len=3;
                operand=operand|m[execaddr+2];
                operand=operand|((m[execaddr+1]%16)<<8);
            }
            char hex[10];
            sprintf(hex, "%03X", operand);
            if (hex[0] == 9 || hex[0] == 8 || (hex[0] >= 'A' && hex[0] <= 'F')) {
                //if negative number
                operand = operand | 0xFFFFF000;
            }
        

            if(b==0 && p==1){
                //if PC
                if(n==1 && i==1){
                    //simple
                    operand+=(execaddr+len);
                }
                else if(n==1 && i==0){
                    //indirect
                    operand+=(execaddr+len);
                }
                else if(n==0 && i==1){
                    //immediate
                    operand+=(execaddr+len);
                }
            }
            if(b==0 && p==0){
                //if direct 
                if(n==1 && i==1){
                    
                }
                else if(n==1 && i==0){

                }
                else if(n==0 && i==1){
                    
                }
            }
            if(b==1 && p==0){
                //if BASE
                if(n==1 && i==1){
                    //simple
                    operand+=BASE;
                }
                else if(n==1 && i==0){

                }
                else if(n==0 && i==1){

                }
            }

        }
        //--------------FORMATCHECKEND-------------------------------
        if(opcode==0x14){//STL
            m[operand]=(reg.L&0x00FF0000)>>16;
            m[operand+1]=(reg.L&0x0000FF00)>>8;
            m[operand+2]=(reg.L&0x000000FF);
        }
        if(opcode==0x68){ //LDB m
            reg.B=operand;
        }
        if(opcode==0x48){ //JSUB
            reg.L=execaddr+len;
            execaddr=reg.PC=operand;
            PC_edit=true;
            
        }
        if(opcode==0x74){//LDT
            int t=0;
            t=t|(m[operand]<<16);
            t=t|(m[operand+1]<<8);
            t=t|(m[operand+2]);
            if(n==0 && i==1) t=operand;
            reg.T=t;
        }

        if(opcode==0x00){ //LDA
            int t=0;
            t=t|(m[operand]<<16);
            t=t|(m[operand+1]<<8);
            t=t|(m[operand+2]);

            if(n==0 && i==1) t=operand;
            reg.A=t;
        }
        if(opcode==0x0C){//STA
            m[operand]=(reg.A&0x00FF0000)>>16;
            m[operand+1]=(reg.A&0x0000FF00)>>8;
            m[operand+2]=(reg.A&0x000000FF);

        }
        if(opcode==0x28){//COMP
            if(reg.A>operand) CC=-1;
            else if(reg.A<operand) CC=1;
            else CC=0;
        }
        if(opcode==0x30){//JEQ
            if(CC==0){
                execaddr=reg.PC=operand;
                PC_edit=true;
            }
        }
        if(opcode==0xB4){//CLEAR
            if(operand>>4==0) reg.A=0;
            if(operand>>4==1) reg.X=0;
            if(operand>>4==4) reg.S=0;
        }
        if(opcode==0xE0){//TD
            CC=-1;
        }
        if(opcode==0xD8){//RD
            next_CC=true;//if next_CC=true next COMPR CC is = (0)
        }
        if(opcode==0xA0){//COMPR
            //might make some inprovements
            if(next_CC) CC=0;
            else{
                if(reg.A>reg.S) CC=1;
                else if(reg.A<reg.S) CC=-1;
                else CC=0;
            }                
        }
        if(opcode==0x54){//STCH
             m[operand+reg.X]=reg.A&0xFF;
        }
        if(opcode==0xB8){//TIXR
            //might make some improvements
            reg.X++;
            if(reg.X<reg.T) CC=-1;
            else if(reg.X>reg.T) CC=1;
            else CC=0;
        }

        if(opcode==0x38){//JLT
            if(CC==-1){
                execaddr=reg.PC=operand;
                PC_edit=true;
            }
        }
        if(opcode==0x10){//STX
            m[operand]=(reg.X&0x00FF0000)>>16;
            m[operand+1]=(reg.X&0x0000FF00)>>8;
            m[operand+2]=(reg.X&0x000000FF);
        }

        if(opcode==0x4C){//RSUB
            execaddr=reg.PC=reg.L;
            PC_edit=true;
        }

        if(opcode==0x50){//LDCH
            reg.A=0;
            reg.A=reg.A|m[operand+reg.X];
        }
        if(opcode==0xDC){
            //WD do nothing
        }
        if(opcode==0x3C){
            if(n==1&&i==0){
               int t=0;
               t=t|(m[operand]<<16);
               t=t|(m[operand+1]<<8);
               t=t|(m[operand+2]);
               execaddr=reg.PC=t; 
               PC_edit=true;
            }
        }

        
        if(!PC_edit){
            execaddr+=len;
            reg.PC=execaddr;
        }


        if(break_flag){
            break;
        }
    }

    if(prog_end){
        print_regs();
        printf("            End Program\n");
        execaddr=progaddr;
        reg.A=reg.X=reg.PC=reg.B=reg.S=0;
        reg.L=proglength;
    }

    return 0;
}

/*---------------------------------------------------------------------*/
/*Function: loader*/
/*Purpose: loads program*/
/*Return: 1 on success -1 on fail*/
/*---------------------------------------------------------------------*/
int loader(char *cmd){
    destroy_EST(&est_head);
    char inst[10], obj1[100], obj2[100], obj3[100];
    int file_num = sscanf(cmd, "%s %s %s %s", inst, obj1, obj2, obj3);
    csaddr=progaddr;
    execaddr=progaddr;

    if(file_num==2){
        if(!file_exists(obj1)){
            printf("load error: no file\n"); return -1;
        } 
        if(pass1(obj1)==-1) return -1;

        csaddr=progaddr; execaddr=progaddr;
        if(pass2(obj1)==-1) return -1;

        print_EST(&est_head);
        reg.L=proglength;


    }
    else if(file_num==3){
        if(!file_exists(obj1) || !file_exists(obj2)){
            printf("load error: no file\n"); return -1;
        }
        if(pass1(obj1)==-1) return -1;
        if(pass1(obj2)==-1) return -1;

        csaddr=progaddr; execaddr=progaddr;
        if(pass2(obj1)==-1) return -1;
        if(pass2(obj2)==-1) return -1;

        print_EST(&est_head);
        reg.L=proglength;

    }
    else if(file_num==4){
        if(!file_exists(obj1) || !file_exists(obj2) || !file_exists(obj3)){
            printf("load error: no file\n"); return -1;
        }
        if(pass1(obj1)==-1) return -1;
        if(pass1(obj2)==-1) return -1;
        if(pass1(obj3)==-1) return -1;

        csaddr=progaddr; execaddr=progaddr;

        if(pass2(obj1)==-1) return -1;
        if(pass2(obj2)==-1) return -1;
        if(pass2(obj3)==-1) return -1;
        
        print_EST(&est_head);
        reg.L=proglength;
    }
    else{
        return -1;
    }
    history_add(cmd, &head);
    return 0;
}