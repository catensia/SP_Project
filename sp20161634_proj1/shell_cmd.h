#include "defines.h"


void history_add(char command[], struct NODE **head);
void history_print(struct NODE **head);
void shell_dir(void);
void shell_help(void);
char* trim(char *cmd);