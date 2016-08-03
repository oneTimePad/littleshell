#ifndef COLORS_H

#include <stdio.h>
#include <stdarg.h>

#define COLOR_H

#define BLACK 0
#define GREEN 2
#define BLUE  4
#define MAGENTA 5
#define WHITE 7

#define RESET  0
#define BRIGHT 1



#define MAX_COLOR 20

void cprintf(int,int,int,const char*,...);

#endif
