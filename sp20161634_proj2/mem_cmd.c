#include "mem_cmd.h"

/*---------------------------------------------------------------------*/
/*Function: edit*/
/*Purpose: edits memory based on address and value*/
/*Return: void*/
/*---------------------------------------------------------------------*/
void edit(unsigned char mem[], int address, int value){
    mem[address]=(char)value;
}

/*---------------------------------------------------------------------*/
/*Function: reset*/
/*Purpose: resets whole memory to 0*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void reset(unsigned char mem[]){
    for(int i=0;i<=0xFFFFF;i++){
        mem[i]=0;
    }
}


/*---------------------------------------------------------------------*/
/*Function: fill*/
/*Purpose: assigns value to memory inside given address range*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void fill(unsigned char mem[], int start, int end, int value){
    //range overflow exception
    if(start>end) {
        printf("invalid range\n");
        return;
    }

    for(int i=start;i<=end;i++){
        mem[i]=value;
    }
}

/*---------------------------------------------------------------------*/
/*Function: print*/
/*Purpose: prints formatted contents of virtual memory*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void print(unsigned char mem[], int start, int end){
    //ex: du 15 57 (decimal)  -->  0 ~ 59
    int s=(start/16)*16;
    int e=(end/16+1)*16-1;
    //range overflow exception
    if(e>0xFFFFF) e=0xFFFFF;
    for(int i=s;i<=e;i+=16){
        //address | memory contents | ASCII CODE
        printf("%05X ", i);

        for(int j=i;j<i+16;j++){
            if(j<start || j>end) printf("   ");
            else{
                printf("%02X ", (int)mem[j]);
            }
        }
        printf("; ");

        for(int j=i;j<i+16;j++){
            if(j<start || j>end) printf(".");
            else if(mem[j]<0x20 || mem[j]>0x7E) printf(".");
            else printf("%c", mem[j]);
        }

        printf("\n");
    }
}

/*---------------------------------------------------------------------*/
/*Function: dump*/
/*Purpose: variadic function - checks argument size and calls corresponding function*/
/*Return: none*/
/*---------------------------------------------------------------------*/
void dump(unsigned char mem[], int args, ...){
    va_list ap;
    va_start(ap, args);
    int num;
    if(args==1){
        //if only dump or dump start 
        int start,end;
        for(int i=0;i<args;i++){
            num=va_arg(ap,int);
            if(i==0) start=num;
        }
        end=start+159;
        va_end(ap);
        print(mem, start, end);
    }
    else if(args==2){
        //dump start end
        int start, end;
        for(int i=0;i<args;i++){
            num=va_arg(ap, int);
            if(i==0) start=num;
            if(i==1) end=num;
        }
        va_end(ap);
        //range exception when dump 4,3
        if(start>end){
            printf("range error!\n"); return;
        }
        print(mem, start, end);
        
    }
}