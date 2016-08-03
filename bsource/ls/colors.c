#include "colors.h"


/**
* sets printed text to have color
* fg: foreground text color
* bg: background text color
* attr: attribute for color
**/
static void setColor(int fg,int bg,int attr){
  char color[MAX_COLOR];
  if(fg==-1 && bg==-1)
    sprintf(color,"%c[%dm",0x1B,attr);
  else if(bg==-1)
    sprintf(color,"%c[%d;%dm",0x1B,attr,fg+30);
  else if(fg==-1)
    sprintf(color,"%c[%d;%dm",0x1B,attr,bg+30);
  else
    sprintf(color,"%c[%d;%d;%dm" ,0x1B,attr,fg+30,bg+40);
  printf("%s",color);
}

/**
* printf that prints in color
* attr: attribute for color
* fg: foregound text color
* bg: background text color
* format: format specified
* varargs...
**/
void cprintf(int attr,int fg, int bg,const char* format,...){

    va_list ap;
    va_start(ap,format);
    setColor(fg,bg,attr);
    vprintf(format,ap);
    setColor(-1,-1,RESET);
    va_end(ap);

}
