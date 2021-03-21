#ifndef __CHECK__
#define __CHECK__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
struct NODE{
    struct NODE *next;
    char format[10];
    char cmd[100];
    int num;
};

#endif