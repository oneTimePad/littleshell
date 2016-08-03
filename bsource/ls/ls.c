
#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "../errors.h"
#include "colors.h"

#define CURR_DIRECTORY "PWD"
#define MAX_FULL_PATH 512
#define MAX_LONG_FORM_PRINT 2000
#define LEN_MAX_64_SINT 20


#define DIREC "d"
#define READ "r"
#define WRITE "w"
#define EXEC "x"
#define NONE "-"


#define ISTOOLONG(LENGTH) (LENGTH>=MAX_LONG_FORM_PRINT)? TRUE : FALSE
#define ISD(MODE) (!S_ISREG(MODE))? DIREC: NONE
#define ISR(MODE,MASK) (MODE & MASK)? READ : NONE
#define ISW(MODE,MASK) (MODE & MASK)? WRITE : NONE
#define ISX(MODE,MASK) (MODE & MASK)? EXEC : NONE
int main(int argc, char* argv[]){



    char* file;


    _BOOL show_hidden = FALSE;
    _BOOL long_form = FALSE;

    int opt;
    while((opt=getopt(argc,argv,"al"))!=-1){
        switch(opt){

          case 'l':
            long_form = TRUE;
            break;
          case 'a':
            show_hidden = TRUE;
            break;
          default:
            errExit("unknown flag -%s\n",opt);
            break;

        }
    }

    if(argc < optind+1) //if no file is specified assume cwd
      file = secure_getenv(CURR_DIRECTORY);
    else
      file = argv[optind];


    DIR* dir;

    if((dir=opendir(file))==(DIR*)NULL)
      errnoExit("opendir()");


    struct dirent *entry;

    while((entry=readdir(dir))!=NULL){ //traverse directory
        //check if we can show hidden files
        if(!show_hidden)
          if(entry->d_name[0]=='.')
            continue;

        //compute the absolute path name of the file
        size_t dir_len = strlen(file);
        size_t file_len = strlen(entry->d_name);
        size_t total_len = dir_len+file_len;

        if(total_len+1>=MAX_FULL_PATH-2)
            errExit("%s\n","filename is too long");


        char full_path[MAX_FULL_PATH];
        memset(full_path,0,MAX_FULL_PATH);
        strncpy(full_path,file,dir_len);
        full_path[dir_len]=0x0;
        if(file[dir_len-1]!='/')
          strncat(full_path,"/",(size_t)1);
        strncat(full_path,entry->d_name,file_len);

        full_path[((file[dir_len]=='/')? total_len : total_len+1)] = 0x0;

        //get file stats
        struct stat file_stat;
        if(stat(full_path,&file_stat)==-1)
          errnoExit("stat()");


        mode_t file_mode = file_stat.st_mode;

        int cforeground=WHITE;
        int cbackground=-1;
        //check if not regular file
        if(!S_ISREG(file_mode))
          cforeground= BLUE; //directories are blue

        //if executable make green
        if(file_mode&S_IXUSR | file_mode&S_IXGRP | file_mode&S_IXOTH){
            if(cforeground!=BLUE)
              cforeground=GREEN;

        }


        if(long_form){
          int cur_output_size =0;
          char long_form[MAX_LONG_FORM_PRINT];


          memset(long_form,0,MAX_LONG_FORM_PRINT);
          char * ptr = long_form;
          sprintf(ptr++,"%s",ISD(file_mode));
          //user mode flags
          sprintf(ptr,"%s%s%s",ISR(file_mode,S_IRUSR),ISW(file_mode,S_IWUSR),ISX(file_mode,S_IXUSR));
          ptr = ptr+3;
          //group mode flags
          sprintf(ptr,"%s%s%s",ISR(file_mode,S_IRGRP),ISW(file_mode,S_IWGRP),ISX(file_mode,S_IXGRP));
          ptr = ptr+3;
          //world mode flags
          sprintf(ptr,"%s%s%s",ISR(file_mode,S_IROTH),ISW(file_mode,S_IWOTH),ISX(file_mode,S_IXOTH));
          ptr = ptr+3;
          cur_output_size = cur_output_size+strlen(long_form);
          //printf("%s  \n",long_form);

          cur_output_size++;
          *ptr++ = ' ';

          struct group *grp;
          struct passwd *pwd;

          //convert uid to actual name
          errno = 0;
          if((pwd=getpwuid(file_stat.st_uid))==NULL){
            errnoExit("getpwuid()");
            //can't find a name for the user so just print number
            sprintf(ptr++,"%ld",(long)file_stat.st_uid);
          }
          else{
            size_t pw_name_len =strlen(pwd->pw_name);
            //check if this makes output too long
            if(ISTOOLONG(pw_name_len+cur_output_size))
              errExit("%s\n","username is too long");

            //print the username
            sprintf(ptr,"%s",pwd->pw_name);
            ptr =ptr+pw_name_len;
            cur_output_size+pw_name_len;
          }
          *ptr++ = ' ';
          //convert gid to actual name
          errno = 0;
          if((grp=getgrgid(file_stat.st_gid))==NULL){
              errnoExit("getgrgid()");
              //if name not found just print gid
              sprintf(ptr++,"%ld",(long)file_stat.st_gid);
          }
          else{
            size_t gr_name_len = strlen(grp->gr_name);
            //check for overflow
            if(ISTOOLONG(gr_name_len+cur_output_size))
              errExit("%s\n","Group name is too long");


            //print group name
            sprintf(ptr,"%s",grp->gr_name);
            cur_output_size+gr_name_len;
            ptr = ptr+gr_name_len;
          }

          *ptr++=' ';

          //safely fetch the size of the file (protect overflow)
          char file_size_holder[LEN_MAX_64_SINT];
          memset(file_size_holder,0,LEN_MAX_64_SINT);
          sprintf(file_size_holder,"%ld",(long)file_stat.st_size);
          size_t file_size_len = strlen(file_size_holder);
          //sanity check
          if(ISTOOLONG(cur_output_size+file_size_len))
            errExit("%s\n","File size is too long");

          ptr =ptr+file_size_len;
          cur_output_size = cur_output_size+file_size_len;
          //sprintf null-terms: safe
          strcat(long_form,file_size_holder);





          struct tm* time_struc;
          errno = 0;
          if((time_struc=localtime(&file_stat.st_mtime)) == NULL)
            errnoExit("localtime()");

          *ptr++ = ' ';

          size_t time_string_output_len;
          errno = 0;

          if((time_string_output_len=strftime(ptr,MAX_LONG_FORM_PRINT-cur_output_size,"%b %d %I:%M",time_struc))==0)
            errnoExit("strftime()");

          //char* time_string;
          //if((time_string = asctime(time_struc))==NULL)
          //  errnoExit("ascitime()");
          //size_t time_string_len = strlen(time_string);
          //if(ISTOOLONG(cur_output_size+time_string_len))
          //  errExit("%s\n","time is too long");
          //*ptr++ = ' ';
          //snprintf(ptr,time_string_output_len,"%s",time_string_ou);
          //ptr = ptr+time_string_len;

            printf("%s ",long_form);

        }
        cprintf(BRIGHT,cforeground,cbackground,"%s\n",entry->d_name);
    }

    if(errno!=0){
      perror("readdir_r()");
      exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
