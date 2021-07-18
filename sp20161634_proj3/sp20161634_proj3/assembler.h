#include "defines.h"

struct INST{
    bool comment;
    int line_number;
    int type;
    int format;
    /*
    LABEL OPCODE OPERAND, INDEX  - type 1
          OPCODE OPERAND, INDEX  - type 2
    LABEL OPCODE OPERAND         - type 3
          OPCODE OPERAND         - type 4
    LABEL OPCODE                 - type 5
          OPCODE                 - type 6
    */
    char trash[30];
    char label[30];
    char oper[30];
    char opcode[30];
    char idx[30];  
};

void file_ext_appender(char* filename, const char* extension);
void LSTfprinter(FILE *fp, int line_number, int LOCCTR, const char *label, const char *opcode, const char *oper, const char*idx, bool index_exists);
void LSTprinter(int line_number, int LOCCTR, const char *label, const char *opcode, const char *oper, const char*idx, bool index_exists);
void symbol_clear(struct NODE **head);
void appendString(char *target, int xbpe, int temp, int mode, bool indexed);
int is_register(const char *operand);
bool onlynumber(const char *s);
int symtab_retval(const char *label, struct NODE **head);
bool symtab_exist(const char *label, struct NODE **head);
void symtab_add(const char *label, int locctr, struct NODE **head);
void create_files( const char *filename);
void remove_files(const char *filename);
void copy_strings(struct INST *target, const char *label, const char *opcode, const char *oper, const char *idx);
struct INST myparser(const char *line, int line_number);
int assemble(const char *filename, struct NODE **head);
