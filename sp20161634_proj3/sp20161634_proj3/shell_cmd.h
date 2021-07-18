#include "defines.h"

void symtab_print(struct NODE **head);
void history_add(char command[], struct NODE **head);
void history_print(struct NODE **head);
void shell_dir(void);
void shell_help(void);
void type_filename(const char *filename, struct NODE **head);
char* trim(char *cmd);