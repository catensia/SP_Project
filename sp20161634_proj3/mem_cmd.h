#include "defines.h"

void dump(unsigned char mem[], int args, ...);
void print(unsigned char mem[], int start, int end);
char *intToHex(int n, int len);
void fill(unsigned char mem[], int start, int end, int value);
void edit(unsigned char mem[], int address, int value);
void reset(unsigned char mem[]);