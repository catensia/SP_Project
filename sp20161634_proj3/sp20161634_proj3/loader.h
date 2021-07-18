#include "defines.h"

int loader(char *cmd);
bool file_exists(char *filename);
int pass1(char *filename);
int EST_search(const char *name, int type, struct ESTNODE **est_head);
void EST_add(char *csect, char *symbol, int address, int length, int type,  struct ESTNODE **est_head);
void print_bp();
void clear_bp();
void add_bp(int new_bp);
int run(void);
void destroy_EST(struct ESTNODE **est_head);