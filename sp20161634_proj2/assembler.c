#include "assembler.h"
#include "opcode.h"
#include <inttypes.h>
#define MAX_FILENAME 100
#define ERROR -1
#define MEMSET(string) memset(string, '\0', sizeof(string));

//head of linked list for history cmd
struct NODE *head;

//head of linked list for last success symbol table
struct NODE *last_success_symtab;

//hash table pointer
struct NODE *hash[MAX_HASH];

//head of linked list for current symbol table
struct NODE *symbol;
FILE *lst, *obj, *itm;

/*---------------------------------------------------------------------*/
/*Function: file_ext_appender*/
/*Purpose: Get a file name and change the extension format with parameter*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void file_ext_appender(char* filename, const char* extension) {
	int flen = strlen(filename);
	for (int i = 0; i < 3; i++) {
		filename[flen - 1 - i] = '\0';
	}
	strcat(filename, extension);
}

/*---------------------------------------------------------------------*/
/*Function: remove_files*/
/*Purpose: remove filename.obj, filename.lst, filename.itm*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void remove_files(const char *filename){
    char fname[MAX_FILENAME]; 
    int rem=0;

    strcpy(fname, filename);
    file_ext_appender(fname, "lst");
    rem=remove(fname);
    if(rem==-1) printf("deletion failed\n");
    
    file_ext_appender(fname, "obj");
    rem=remove(fname);
    if(rem==-1) printf("deletion failed\n");

    file_ext_appender(fname, "itm");
    rem=remove(fname);
    if(rem==-1) printf("deletion failed\n");
    
}

/*---------------------------------------------------------------------*/
/*Function: creates_files*/
/*Purpose: creates filename.obj, filename.lst, filename.itm*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void create_files(const char *filename){
    //make files
    char fname[MAX_FILENAME];
    strcpy(fname, filename);
    file_ext_appender(fname, "lst");
    lst=fopen(fname, "w+");
    
    file_ext_appender(fname, "obj");
    obj=fopen(fname, "w+");

    file_ext_appender(fname, "itm");
    itm=fopen(fname, "w+");

}

/*---------------------------------------------------------------------*/
/*Function: symbol_clear*/
/*Purpose: removes symbol table*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void symbol_clear(struct NODE **head){
    struct NODE *cur=(*head);
    struct NODE* next;
    while(cur!=NULL){
        next=cur->next;
        free(cur);
        cur=next;
    }

    *head=NULL;
}

/*---------------------------------------------------------------------*/
/*Function: symtab_retval*/
/*Purpose: retrieves value from corresponding symbol from symbol table*/
/*Return: symbol data if exists, -1 if doesn't exists*/
/*---------------------------------------------------------------------*/
int symtab_retval(const char *label, struct NODE **head){
    for(struct NODE *cur=(*head);cur!=NULL;cur=cur->next){
        if(!strcmp(label, cur->cmd)){
            return cur->num;
        }
    }
    return -1;
}

/*---------------------------------------------------------------------*/
/*Function: symtab_exist*/
/*Purpose: determine whether label exists in symbol table*/
/*Return: true if exists, false if doesn't exists*/
/*---------------------------------------------------------------------*/
bool symtab_exist(const char *label, struct NODE **head){
    for(struct NODE *cur=(*head);cur!=NULL;cur=cur->next){
        if(!strcmp(label, cur->cmd)){
            return true;
        }
    }
    return false;
}

/*---------------------------------------------------------------------*/
/*Function: symtab_add*/
/*Purpose: add data to symbol table*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void symtab_add(const char *label, int locctr, struct NODE **head){
    struct NODE *newNode=(struct NODE*)malloc(sizeof(struct NODE));
    strcpy(newNode->cmd, label);
    newNode->num=locctr;
    newNode->next=NULL;
    struct NODE *cur=(*head);
    if(*head==NULL || strcmp((*head)->cmd, newNode->cmd)>=0){
        newNode->next=(*head);
        *head=newNode;
    }
    else{
        while(cur->next!=NULL && strcmp(cur->next->cmd,newNode->cmd)<0){
            cur=cur->next;
        }
        newNode->next=cur->next;
        cur->next=newNode;
    }
}

/*---------------------------------------------------------------------*/
/*Function: is_register*/
/*Purpose: checks if given string is a register*/
/*Return: corresponding register number, -1 if not a register*/
/*---------------------------------------------------------------------*/
int is_register(const char *operand){
    if(!strcmp(operand,"A")) return 0;
    if(!strcmp(operand,"X")) return 1;
    if(!strcmp(operand,"L")) return 2;
    if(!strcmp(operand,"B")) return 3;
    if(!strcmp(operand,"S")) return 4;
    if(!strcmp(operand,"T")) return 5;
    if(!strcmp(operand,"F")) return 6;
    if(!strcmp(operand,"PC")) return 8;
    if(!strcmp(operand,"SW")) return 9;

    return -1;
}

/*---------------------------------------------------------------------*/
/*Function: appendString*/
/*Purpose: Makes object code and prints into corresponding files*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void appendString(char *target, int xbpe, int temp, int mode, bool indexed){
	char xbpe_str[5];
	char OPADR_str[10];
	char t[10];
    sprintf(xbpe_str, "%X", xbpe);
    
    if(mode==0){
        strcat(target, xbpe_str);
        if(temp>=0){
	    sprintf(OPADR_str, "%03X", temp);
	    strcat(target, OPADR_str);
        }
        else{
            sprintf(t, "%X", temp);
            int tlen=strlen(t);
            for(int i=0;i<4;i++){
                t[i]=t[i+tlen-3];
            }
            strcat(target, t);
        }
    }
    if(mode==1){
        if(temp>=0){
            if(!indexed) sprintf(OPADR_str, "1%05X", temp);
            else sprintf(OPADR_str, "9%05X", temp);
            strcat(target, OPADR_str);
        }
        else{
            sprintf(t, "%X", temp);
            int tlen=strlen(t);
            for(int i=0;i<6;i++){
                t[i]=t[i+tlen-5];
            }
            strcat(target, t);
        }
    }
    
}

/*---------------------------------------------------------------------*/
/*Function: LSTprinter*/
/*Purpose: prints to console with format (for debugging)*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void LSTprinter(int line_number, int LOCCTR, const char *label, const char *opcode, const char *oper, const char*idx, bool index_exists){
    
        printf("%d\t", line_number);
        if(LOCCTR!=-1) printf("%04X\t", LOCCTR);
        else printf("\t");

        if(label[0]!='*') printf("%s\t", label);
        else printf("\t");

        if(opcode[0]!='*') printf("%s\t", opcode);
        else printf("\t");


        if(index_exists){
            //BUFFER, X 
            if(strlen(oper)+strlen(idx)>5) printf("%s, %s ", oper, idx);
            else printf("%s, %s\t", oper, idx);
        }
        else{
            if(oper[0]!='*') printf("%s\t", oper);
            else printf("\t");
        }
        printf("\t");
}

/*---------------------------------------------------------------------*/
/*Function: LSTfprinter*/
/*Purpose: prints to listing file with format*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void LSTfprinter(FILE *fp, int line_number, int LOCCTR, const char *label, const char *opcode, const char *oper, const char*idx, bool index_exists){
    
        fprintf(fp, "%d\t", line_number);
        if(LOCCTR!=-1) fprintf(fp, "%04X\t", LOCCTR);
        else fprintf(fp, "\t");

        if(label[0]!='*') fprintf(fp, "%s\t", label);
        else fprintf(fp, "\t");

        if(opcode[0]!='*') fprintf(fp, "%s\t", opcode);
        else fprintf(fp, "\t");


        if(index_exists){
            //BUFFER, X 
            if(strlen(oper)+strlen(idx)>5) fprintf(fp, "%s, %s ", oper, idx);
            else fprintf(fp, "%s, %s\t", oper, idx);
        }
        else{
            if(oper[0]!='*') fprintf(fp, "%s\t", oper);
            else fprintf(fp, "\t");
        }
        fprintf(fp, "\t");
}


/*---------------------------------------------------------------------*/
/*Function: getINSTtype*/
/*Purpose: gets instruction type*/
/*Return: instruction type, and -1 if invalid*/
/*---------------------------------------------------------------------*/
int getINSTtype(const char *label, const char *opcode, const char *oper, const char*idx){
    if(label[0]!='*' && opcode[0]!='*' && oper[0]!='*' && idx[0]!='*' ) return 1;
    if(label[0]=='*' && opcode[0]!='*' && oper[0]!='*' && idx[0]!='*' ) return 2;
    if(label[0]!='*' && opcode[0]!='*' && oper[0]!='*' && idx[0]=='*' ) return 3;
    if(label[0]=='*' && opcode[0]!='*' && oper[0]!='*' && idx[0]=='*' ) return 4;
    if(label[0]!='*' && opcode[0]!='*' && oper[0]=='*' && idx[0]=='*' ) return 5;
    if(label[0]=='*' && opcode[0]!='*' && oper[0]=='*' && idx[0]=='*' ) return 6;

    return -1;
}


/*---------------------------------------------------------------------*/
/*Function: onlynumber*/
/*Purpose: checks if string contains only digits*/
/*Return: true/false*/
/*---------------------------------------------------------------------*/
bool onlynumber(const char *s){
    for(int i=0;i<strlen(s);i++){
        if(!(s[i]>='0' && s[i]<='9')) return false;
    }
    return true;
}


/*---------------------------------------------------------------------*/
/*Function: assemble*/
/*Purpose: executes assembling process*/
/*Return: -1 if error occurs, 1 if successful*/
/*---------------------------------------------------------------------*/
int assemble(const char *filename, struct NODE **head){
    
    FILE *fp;
    int len=strlen(filename);
    //if no file
    if(!(fp=fopen(filename,"r"))){
        printf("no file\n"); 
        return -1;
    }
    //if invalid extension 
    if(!(filename[len-1]=='m' && filename[len-2]=='s' && filename[len-3]=='a' && filename[len-4]=='.')){
        fclose(fp);
        printf("invalid extension\n");
         return -1;
    }

    int line_number=5;      //current line number
    char line[120];         //current line string
    struct INST cur;        //tokenized data of current line
    cur.comment=false;      
    lst=obj=NULL;
    bool ERR_FLAG=false;    //ERROR FLAG
    int LOCCTR=0;           //Location counter of current line
    int start_adr=0;        //Starting address of program
    int program_length=0;   //Program length
    int num;                //multipurpose
    char hexa[2];           //to convert digits to hexadecimal
    int modif_record_num=0; //number of modification records
    int *modif;             //modification records dynamically allocated

    create_files(filename);


    //========================================PASS 1========================================
    //read first input line
    if(fgets(line, sizeof(line), fp)!=NULL){
        cur=myparser(line, line_number);

        if(!strcmp(cur.opcode, "START")){
            //SAVE  #[OPERAND] AS STARTING ADDRESS
            sscanf(cur.oper, "%X", &start_adr);
            
            LOCCTR=start_adr;
            fprintf(itm, "%d\t%04X\t%s\t%s\t%s *\n", line_number,LOCCTR,  cur.label, cur.opcode, cur.oper);

            //INITIALIZE LOCCTR TO STARTING ADDRESS
        }
        else{
            //INITIALIZE LOCCTR TO 0
            LOCCTR=0;
        }
        line_number+=5;
    }
    while(fgets(line, sizeof(line), fp)!=NULL){
        int dx=0;
        cur=myparser(line, line_number);
        if(!strcmp(cur.opcode,"END")){
            fprintf(itm,"%d\t", line_number);
            fprintf(itm, "-1\t*\t%s\t%s *\n", cur.opcode, cur.oper);
            break;
        }

        //if this is not a comment line
        if(!cur.comment){
            //if there is a symbol in the label field
            if(cur.label[0]!='\0'){
                if(symtab_exist(cur.label, &symbol)){
                    //set error flag
                    ERR_FLAG=true;
                    printf("Error at line %d : duplicate symbol\n", line_number);
                    remove_files(filename);
                    symbol_clear(&symbol);
                    return -1;
                }
                else{
                    symtab_add(cur.label, LOCCTR, &symbol);
                }
            }
            sscanf(cur.oper, "%d", &num);

            //if opcode is found in optab
            if(op_return(cur.opcode, hash)!=-1){
                if(cur.format==4) dx+=4;
                else{
                    dx+=op_format(cur.opcode,hash);
                }
            }
            else if(!strcmp(cur.opcode,"WORD")){
                //if OPCODE is WORD add 3 to LOCCTR
                dx+=3;
            }
            else if(!strcmp(cur.opcode,"RESW")){
                //if OPCODE is RESW add 3 * #[OPERAND] to LOCCTR
                dx+=3*num;
            }
            else if(!strcmp(cur.opcode, "RESB")){
                //if OPCODE is RESB add #[OPERAND] to LOCCTR
                dx+=num;
            }
            else if(!strcmp(cur.opcode, "BYTE")){
                //if OPCODE is BYTE find length of constant in bytes and add to LOCCTR
                if(cur.oper[0]=='C'){
                    dx+=(int)strlen(cur.oper)-3;
                }
                else if(cur.oper[0]=='X'){
                    dx+=(int)(strlen(cur.oper)-3)/2;
                }
            }
            else{
                //if OPCODE is not BASE, return ERROR
                if(strcmp(cur.opcode, "BASE")){
                    printf("Error at line %d : invalid opcode\n", line_number);
                    remove_files(filename);
                    symbol_clear(&symbol);
                    return -1;
                }
                //if invalid format, return ERROR
                if(cur.type==-1){
                    ERR_FLAG=true;
                    printf("Error at line %d : invalid format instruction\n", line_number);
                    remove_files(filename);
                    symbol_clear(&symbol);
                    return -1;
                }
            }
            //if format 4
            if(cur.format==4){
                int templen=strlen(cur.opcode);
                for(int i=templen;i>0;i--){
                    cur.opcode[i]=cur.opcode[i-1];
                }
                cur.opcode[0]='+';
                modif_record_num++;
            }

            //print to intermediate file in a more accessible format
            fprintf(itm, "%d\t", line_number);
            if(!strcmp(cur.opcode, "BASE")) fprintf(itm, "-1\t");
            else fprintf(itm, "%04X\t", LOCCTR);
            if(cur.label[0]=='\0') fprintf(itm, "*\t");
            else fprintf(itm, "%s\t", cur.label);
            if(cur.opcode[0]=='\0') fprintf(itm, "*\t");
            else fprintf(itm, "%s\t", cur.opcode);
            if(cur.oper[0]=='\0') fprintf(itm, "*\t");
            else fprintf(itm, "%s\t", cur.oper);
            if(cur.idx[0]=='\0') fprintf(itm, "*\n");
            else fprintf(itm, "%s\n", cur.idx); 
            LOCCTR+=dx;
        }
        else{
            fprintf(itm, "%s", line);
        }

        line_number+=5;
    }

    program_length=LOCCTR-start_adr;
    //===================================END OF PASS 1========================================


    //REWIND FILE POINTER BACK
    rewind(itm);

    int operand_address=0;      //operand address
    int BASE=0;                 //BASE value for base relative instructions
    int recordlen=0;            //length of record
    bool newline=false;         //for RESB, RESW instructions which makes newline in obj files
    char record[100];           //for record in obj file
    char appendOBJ[100];        //for appending
    char append[10];            //for appending

    //Set modification dynamic array
    modif=(int*)malloc(sizeof(int)*modif_record_num);
    int modif_index=0;
    int first_executable=-1;
    int record_global=0;

    //Reset record associated char arrays
    memset(record, '\0', sizeof(record));
    memset(append,'\0', sizeof(append));
    record[0]='T';

    //===================================PASS 2 START=========================================
    if(fgets(line, sizeof(line), itm)!=NULL){
        
        sscanf(line, "%d %d %s %s %s %s", &line_number, &LOCCTR, cur.label, cur.opcode, cur.oper, cur.idx);

        if(!strcmp(cur.opcode, "START")){
            fprintf(lst, "%d\t%04X\t%s\t%s\t%s\t\n", line_number, LOCCTR, cur.label, cur.opcode, cur.oper);
        }
        fprintf(obj, "H%s %06X%06X\n", cur.label, start_adr, program_length);
    }
    while(fgets(line, sizeof(line), itm)!=NULL){

        sscanf(line, "%d %X %s %s %s %s", &line_number, &LOCCTR, cur.label, cur.opcode, cur.oper, cur.idx);
        

        int xbpe;                   //xbpe value
        int OP_val;                 //Opcode value
        char OBJ_str[100];          //Object code
        ERR_FLAG=ERR_FLAG;          
        bool immediate=false;       //checks immediate
        bool indirect=false;        //checks indirect
        int temp;
        bool extended=false;        //checks extended
        bool index_exists=false;    //checks index
        int format=getINSTtype(cur.label, cur.opcode, cur.oper, cur.idx);

        if(format==1 || format==2) index_exists=true;

        if(line[0]=='.'){
            fprintf(lst, "%d\t\t%s", line_number, line);
            continue;
        }        
        LSTfprinter(lst, line_number, LOCCTR, cur.label, cur.opcode, cur.oper, cur.idx, index_exists); 


        //search OPTAB for OPCODE
        if(cur.opcode[0]=='+'){
            temp=strlen(cur.opcode); extended=true;
            for(int i=0;i<temp;i++) cur.opcode[i]=cur.opcode[i+1];
        }
        //CHECKED FOR FORMAT 4 OPERATIONS

        //if opcode == BASE, set base value
        if(!strcmp(cur.opcode, "BASE")){
            BASE=symtab_retval(cur.oper, &symbol);
        }

        if(op_return(cur.opcode, hash)!=-1){
            //if there is a symbol in OPERAND field
            if(cur.oper[0]!='*'){
                if(format==1 || format==2){
                    //IF INDEX EXISTS, REMOVE COMMA AT BACK

                    index_exists=true;
                    temp=strlen(cur.oper);
                }

                //if immediate addressing
                if(cur.oper[0]=='#'){
                    temp=strlen(cur.oper); immediate=true;
                    for(int i=0;i<temp;i++) cur.oper[i]=cur.oper[i+1];

                    //+LDT #4096
                    if(onlynumber(cur.oper)) sscanf(cur.oper, "%d", &operand_address);
                    //LDB #LENGTH
                    else{
                        if(symtab_exist(cur.oper, &symbol)){
                            operand_address=symtab_retval(cur.oper, &symbol);
                        }
                    }
                }

                //if indirect addressing
                else if(cur.oper[0]=='@'){
                    temp=strlen(cur.oper); indirect=true;
                    for(int i=0;i<temp;i++) cur.oper[i]=cur.oper[i+1];
                }


                //IF NORMAL OPCODE STL RETADR
                if(symtab_exist(cur.oper, &symbol)){
                    operand_address=symtab_retval(cur.oper, &symbol);
                }
                //IF IMMEDIATE CONSTANT : LDA #3
                else if(onlynumber(cur.oper)){
                    sscanf(cur.oper,"%d", &operand_address);
                }
                //IF REGISTER ex : CLEAR X
                else if(is_register(cur.oper)!=-1){
                    operand_address=is_register(cur.oper);
                }
                else{
                    operand_address=0;
                    printf("Error at line %d : undefined symbol\n", line_number);
                    remove_files(filename);
                    symbol_clear(&symbol);
                    return -1;
                }

                //ASSEMBLE OBJECT CODE INSTRUCTION


                //if operand is a register
                OP_val=op_return(cur.opcode, hash);
                if(is_register(cur.oper)!=-1){
                    sprintf(OBJ_str, "%02X", OP_val);
                    int first=is_register(cur.oper);
                    int second=is_register(cur.idx);
                    if(second==-1) second=0;
                    char f[2], s[2];
                    f[0]=first+'0'; s[0]=second+'0'; s[1]=f[1]='\0';
                    strcat(OBJ_str, f);
                    strcat(OBJ_str, s);

                    line_number+=5;
                    fprintf(lst, "%s\n", OBJ_str);

                    goto TEXTRECORD;

                }
                else if(immediate){
                    sprintf(OBJ_str, "%02X", OP_val+1);
                }
                else if(indirect){
                    sprintf(OBJ_str, "%02X", OP_val+2);
                }
                else{
                    sprintf(OBJ_str, "%02X", OP_val+3);
                }

                //CALCULATE DISPLACEMENT

                //TRY PC FIRST
                temp=operand_address-LOCCTR-3;
                if(immediate && onlynumber(cur.oper)){
                    sscanf(cur.oper, "%d", &temp);
                    
                }

                if(extended){
                    if(!onlynumber(cur.oper) && !immediate && !indirect){
                        modif[modif_index++]=record_global+1;
                    }
                    appendString(OBJ_str, xbpe, operand_address, 1, index_exists);
                }
                else if(-2048<=temp && temp<=2047){
                    
                    //calculate displacement PC relative
                    xbpe=0;
                    if(extended) xbpe+=1;
                    if(index_exists) xbpe+=8;
                    xbpe+=2;
                    if(onlynumber(cur.oper)) xbpe=0;
                    appendString(OBJ_str, xbpe, temp,0, index_exists);
                }
                else{
                    //TRY BASE
                    temp=operand_address-BASE;
                    if(immediate && onlynumber(cur.oper)){
                        sscanf(cur.oper, "%d", &temp);
                    }
                    if(0<=temp && temp<=4095){
                        xbpe=0;
                        if(index_exists)xbpe+=8;
                        xbpe+=4;
                        appendString(OBJ_str, xbpe, temp,0, index_exists);
                    }
                    //if nothing works, print error
                    else{
                        printf("Error at line %d: unreachable address\n", line_number);
                        remove_files(filename);
                    }
                }

            }

            else{
                OP_val=op_return(cur.opcode, hash);
                sprintf(OBJ_str, "%X0000", OP_val+3);
            }

        
        }
        //if OPCODE = BYTE
        else if(!strcmp(cur.opcode, "BYTE")){
            memset(OBJ_str, '\0', sizeof(OBJ_str));
            //calculates constant length in bytes
            if(cur.oper[0]=='C'){
                for(int i=2;i<strlen(cur.oper)-1;i++){
                    sprintf(hexa, "%02X", (int)cur.oper[i]);
                    strcat(OBJ_str, hexa);
                }
            }
            else if(cur.oper[0]=='X'){
                for(int i=2;i<strlen(cur.oper)-1;i++){
                    OBJ_str[i-2]=cur.oper[i];
                }
                OBJ_str[strlen(cur.oper)-3]='\0';
            }
        }
        else if(!strcmp(cur.opcode, "WORD")){
            //generate object code with WORD directive
            memset(OBJ_str, '\0', sizeof(OBJ_str));
            int word_directive;
            sscanf(cur.oper, "%d", &word_directive);
            sprintf(OBJ_str, "%06d", word_directive);

        }
        else{
            memset(OBJ_str, '\0', sizeof(OBJ_str));
        }

        fprintf(lst, "%s\n", OBJ_str);
        line_number+=5;

        //Generates OBJECT CODE to write to .obj file, and write necessary newlines(RESW, RESB or full column)
        TEXTRECORD:
        if(first_executable==-1) first_executable=LOCCTR;


        //if record is full, or opcode is RESB or RESW, write a newline
        if(recordlen+strlen(OBJ_str)/2>=30 || ((!strcmp(cur.opcode, "RESB") || !strcmp(cur.opcode, "RESW")))){
            if(!newline){
                sprintf(append, "%02X", recordlen);
                strcat(record, append);
                strcat(record, appendOBJ);
                fprintf(obj, "%s\n", record);
                memset(append, '\0', sizeof(append));
                memset(record, '\0', sizeof(record));
                memset(appendOBJ, '\0', sizeof(appendOBJ));
                recordlen=0;
                record[0]='T';
            }
            else{
                memset(append, '\0', sizeof(append));
                memset(record, '\0', sizeof(record));
                memset(appendOBJ, '\0', sizeof(appendOBJ));
                record[0]='T';
                continue;
            }
        }

        //Initialize new record with current LOCCTR
        if(recordlen==0){
            if(LOCCTR!=-1){  
            sprintf(append, "%06X", LOCCTR);
            strcat(record, append);
            }
        }

        //Append OBJECT STRINGS to record
        if(recordlen+strlen(OBJ_str)/2<30){
            strcat(appendOBJ, OBJ_str);
            record_global+=strlen(OBJ_str)/2;
            recordlen+=strlen(OBJ_str)/2;
        }
        
        //If RESB OR RESW, indicate newline
        if(!strcmp(cur.opcode, "RESB") || !strcmp(cur.opcode, "RESW"))  newline=true;
        else newline=false;


        //if opcode is END, write remaining object codes and write Modification records and End record
        if(!strcmp(cur.opcode, "END")){
            
            if(strlen(record)>6){
                sprintf(append, "%02X", recordlen);
                strcat(record, append);
                strcat(record, appendOBJ);
                fprintf(obj, "%s\n", record);
            }

            for(int i=0;i<modif_index;i++){
                fprintf(obj, "M%06X%02X\n", modif[i], 5);
            }

            memset(record, '\0', sizeof(record));
            sprintf(append,"%06X", first_executable);
            record[0]='E';
            strcat(record, append);
            fprintf(obj, "%s\n", record);
            break;
        }
    }

    //===================================END OF PASS 2=========================================
    
    //Close files
    fclose(itm);
    fclose(lst);
    fclose(obj);
    
    //Assemble success, so move symbol table to last success
    last_success_symtab=symbol;
    symbol=NULL;

    //remove intermediate file
    char fname[MAX_FILENAME]; 
    strcpy(fname, filename);
    file_ext_appender(fname, "itm");
    remove(fname);

    file_ext_appender(fname, "lst");
    printf("[%s],", fname);
    file_ext_appender(fname, "obj");
    printf(" [%s]\n", fname);


    return 1;
}




/*---------------------------------------------------------------------*/
/*Function: myparser*/
/*Purpose: tokenizes given string*/
/*Return: a struct containing tokenized information*/
/*---------------------------------------------------------------------*/
struct INST myparser(const char *line, int line_number){
    struct INST cur;
    cur.comment=false;
    cur.line_number=line_number;
    cur.format=-1;
    
    int words=0;
    char label[30], opcode[30], oper[30], idx[30], trash[30];

    //INITIALIZE VALUES
    cur.label[0]=cur.opcode[0]=cur.oper[0]=cur.idx[0]=cur.trash[0]='\0'; 
    label[0]=opcode[0]=oper[0]=idx[0]=trash[0]='\0';
    words=sscanf(line, "%s %s %s %s %s", label, opcode, oper, idx, trash);

    //IF + is in front of OPCODE format 4
    if(label[0]=='+'){
        for(int i=0;i<strlen(label);i++) label[i]=label[i+1]; cur.format=4;
    }
    if(opcode[0]=='+'){
        for(int i=0;i<strlen(opcode);i++) opcode[i]=opcode[i+1]; cur.format=4;
    }

    int oper_len=strlen(oper), opcode_len=strlen(opcode);

    //assembler directives without labels
    if(words==2 && !strcmp(label, "END")){
        cur.type=4;
        strcpy(oper, opcode);
        strcpy(opcode, label);
        label[0]='\0';
        copy_strings(&cur, label, opcode, oper, idx);
        return cur;
    }
    if(words==2 && !strcmp(label, "BASE")){
        cur.type=4;
        strcpy(oper, opcode);
        strcpy(opcode, label);
        label[0]='\0';
        copy_strings(&cur, label, opcode, oper, idx);
        return cur;
    }


    if(label[0]=='.'){
        //LINE IS COMMENT
        cur.comment=true;
        return cur;
    }

    if(words==4 && oper[oper_len-1]==','){
        //LABEL OPCODE OPERAND, INDEX
        cur.type=1;
        oper[oper_len-1]='\0';
        copy_strings(&cur, label, opcode, oper, idx);
        return cur;
    }
    else if(words==3 && idx[0]=='\0'){
        if(opcode[opcode_len-1]==','){
            //  OPCODE OPERAND, INDEX

            strcpy(idx, oper); strcpy(oper, opcode); strcpy(opcode, label);
            oper_len=strlen(oper);
            oper[oper_len-1]='\0';
            label[0]='\0';
            cur.type=2;
            copy_strings(&cur, label, opcode, oper, idx);
            return cur;
        }
        else{
            // LABEL OPCODE OPERAND
            cur.type=3;
            
            copy_strings(&cur, label, opcode, oper, idx);
            return cur;
        }
    }
    else if(words==2 && idx[0]=='\0' && oper[0]=='\0'){
        if(op_return(label, hash)!=-1){
            //OPCODE OPERAND
            cur.type=4;
            strcpy(oper, opcode); strcpy(opcode, label);
            label[0]='\0';
        }
        else if(op_return(opcode, hash)!=-1){
            //LABEL OPCODE
            cur.type=5;
        }
        else{
            //NO OPCODE PRESENT (set errorflag)
            cur.type=-1;
        }
        copy_strings(&cur, label, opcode, oper, idx);
        return cur;
    }    
    else if(words==1){
        cur.type=6;
        strcpy(opcode, label);
        label[0]='\0';
        copy_strings(&cur, label, opcode, oper, idx);
        return cur;
    }
    else{
        //CALL DEBUGGER
        cur.type=-1;
        printf("ERROR AT LINE %d\n", line_number);
        return cur;
    }

}


/*---------------------------------------------------------------------*/
/*Function: copy_strings*/
/*Purpose: copies from string to string (multiple)*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void copy_strings(struct INST *target, const char *label, const char *opcode, const char *oper, const char *idx){
    strcpy(target->label, label); strcpy(target->oper, oper);
    strcpy(target->opcode, opcode); strcpy(target->idx, idx);
}