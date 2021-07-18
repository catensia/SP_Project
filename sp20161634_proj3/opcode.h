#include "defines.h"

int op_format(const char* mnemonic, struct NODE* hash[]);
int op_return(const char* mnemonic, struct NODE* hash[]);
void init_hash(struct NODE*hash[]);
void op_cmd(char *mnemonic, struct NODE*hash[], struct NODE **head);
void op_list(struct NODE*hash[]);
int get_format(int code, struct NODE* hash[]);