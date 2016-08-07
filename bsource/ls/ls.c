
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
#include <getopt.h>
#include "../../errors.h"
#include "colors.h"
#include "ls.h"

#define CURR_DIRECTORY "PWD"
#define MAX_TIME_STRING 1024
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

#define SPECIAL_BITS 1

#define SPACES "   "

/**
* converts gid into corresponding name
* gid: id to converts
* name: buffer with enough space to hold grp name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
static _BOOL getnamefromgid(gid_t gid, char *name, size_t buf_size){

  struct group * grp;
  errno = 0;
  if((grp = getgrgid(gid))==NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(buf_size,grp->gr_name)){
    errno = ENOMEM;
    return FALSE;
  }

  strncpy(name,grp->gr_name,buf_size);
  return TRUE;

}

/**
* converts uid into corresponding name
* uid: id to converts
* name: buffer with enough space to hold usr name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
static _BOOL getnamefromuid(uid_t uid, char *name, size_t buf_size){
  struct passwd* pw;
  errno = 0;
  if((pw = getpwuid(uid)) == NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(buf_size,pw->pw_name)){
    errno = ENOMEM;
    return FALSE;
  }

  strncpy(name,pw->pw_name,buf_size);
  return TRUE;
}


/**
* form full path for filename
* path: set of directories to file
* entry: ptr to dirent entry for file
* buff: output buffer
* size: size of output buffer
* returns: status
**/
static _BOOL formPath(const char* path, const struct dirent* entry,char* buff,size_t size){

  //compute the absolute path name of the file
  size_t dir_len = strlen(path);
  size_t file_len = strlen(entry->d_name);
  size_t total_len = dir_len+file_len;

  if(total_len+2>size){
      errno = ENOMEM;
      return FALSE;
  }


  strncpy(buff,path,dir_len);
  buff[dir_len]='\0';
  if(buff[dir_len-1]!='/')
    strncat(buff,"/",(size_t)1);
  strncat(buff,entry->d_name,file_len);

  buff[((path[dir_len]=='/')? total_len : total_len+1)] = '\0';
  return TRUE;

}

static printPerm(const FILE_ENTRY *entry, int flags){
  printf("%c%c%c%c%c%c%c%c%c%c",    (S_ISREG(entry->perm)) ? 'd' : '-',
                  (entry->perm&S_IRUSR) ? 'r' : '-',
                  (entry->perm&S_IWUSR) ? 'w' : '-',
                  (entry->perm&S_IXUSR) ?
                     (( (entry->perm&S_ISUID) && (flags&SPECIAL_BITS)) ? 's' : 'x'):
                     (( (entry->perm&S_ISUID) && (flags&SPECIAL_BITS)) ? 'S' : '-'),
                   (entry->perm&S_IRGRP) ? 'r' : '-',
                   (entry->perm&S_IWGRP) ? 'w' : '-',
                   (entry->perm&S_IXGRP) ?
                      (( (entry->perm&S_ISGID) && (flags&SPECIAL_BITS)) ? 's' : 'x'):
                      (( (entry->perm&S_ISGID) && (flags&SPECIAL_BITS)) ? 'S' : '-'),
                  (entry->perm&S_IROTH) ?  'r' : '-',
                  (entry->perm&S_IWOTH) ?  'w' : '-',
                  (entry->perm&S_IXOTH) ?
                    (((entry->perm&S_ISVTX) && (flags&SPECIAL_BITS)) ? 't' : 'x') :
                    (((entry->perm&S_ISVTX) && (flags&SPECIAL_BITS)) ? 'T' : '-') );



}


static struct option long_options[] = {
  {"all",          no_argument,    0 , 'a'},
  {"almost-all",   no_argument,    0 , 'A'},
  {"author",       no_argument,    0 , 'u'},
  {"inode",        no_argument,    0 , 'i'},
  {"dereference",  no_argument,    0 , 'L'},
  {"recursive",    no_argument,    0 , 'R'},
  {"size",         no_argument,    0 , 's'},
  {"help",         no_argument,    0 , 'h'},
  { 0,             0,              0 ,   0 }
};


int main(int argc, char* argv[]){



    LS_OPTIONS opt_mask;
    memset(&opt_mask,0,sizeof(LS_OPTIONS));


    int opt =0;
    int long_index = 0;
    while((opt = getopt_long(argc,argv,"laAuiLoRsthp", long_options, &long_index)) != -1){
        switch(opt){

          case 'l':
            opt_mask.bits.l = 1; //long list form
            break;
          case 'a':
            opt_mask.bits.a = 1; //do not  ignore entries with .
            break;
          case 'A':
            opt_mask.bits.A = 1; //do not list implied . and ..
            break;
          case 'u':
            opt_mask.bits.u = 1; //with -l print the author of each file
            break;
          case 'i':
            opt_mask.bits.i = 1; //print the index number of each file
            break;
          case 'L':
            opt_mask.bits.L = 1; //dereference symbolic links
            break;
          case 'o':
            opt_mask.bits.o = 1; //like -l, but do not list group information
            break;
          case 'R':
            opt_mask.bits.R = 1; //list subdirectories recursively
            break;
          case 's':
            opt_mask.bits.s = 1; //sort by file size
            break;
          case 't':
            opt_mask.bits.t = 1; //sort by modification time, newest first
            break;
          case 'p':
            opt_mask.bits.p = 1; //print special bits;
            break;
          case 'h':
            opt_mask.bits.h = 1; //display this help and exit
            break;
          case '?':
            fflush(stdout);
            exit(EXIT_FAILURE);
            break;
          default:
            errExit("%s\n","error occured while parsing options");
            break;


        }
    }

    char * file = (argc>1) ? ( (argv[ ((optind>0) ? optind: optind+1) ]==NULL) ? secure_getenv(CURR_DIRECTORY) : argv[ ((optind>0) ? optind: optind+1) ]) : secure_getenv(CURR_DIRECTORY);



    DIR* dir;

    if((dir=opendir(file))==NULL)
      errnoExit("opendir()");


    int entry_ind = 0;
    FILE_ENTRY *entries = (FILE_ENTRY *)malloc(DEF_MAX_ENTRIES*sizeof(FILE_ENTRY));

    struct dirent *entry;
    errno = 0;
    FILE_ENTRY *en = NULL;
    for(;(entry=readdir(dir))!=NULL;entry_ind++){ //traverse directory
        en  = entries + entry_ind;

        if(!formPath(file,entry,en->full_path,PATH_LIM))
          errnoExit("formPath()");

        //get file stats
        struct stat file_stat;
        if(((opt_mask.halfword&DEREF)? stat(en->full_path,&file_stat) : lstat(en->full_path,&file_stat)) == -1)
          errnoExit((opt_mask.halfword&DEREF)? "stat()" : "lstat()");

        //initialize file struct
        en->perm    = file_stat.st_mode;
        en->ino_num = file_stat.st_ino;
        en->hlinks  = file_stat.st_nlink;
        en->uid     = file_stat.st_uid;
        en->gid     = file_stat.st_gid;
        en->size    = file_stat.st_size;
        en->phy_blks= file_stat.st_blocks;
        en->t_atime = file_stat.st_atime;
        en->t_mtime = file_stat.st_mtime;
        en->t_ctime = file_stat.st_ctime;

        errno = 0;
    }

    if(errno!=0){
      perror("readdir_r()");
      exit(EXIT_FAILURE);
    }

    int max_entries = entry_ind;
    entry_ind = 0;
    char name_buf[LOGIN_NAME_MAX];
    memset(name_buf,0,LOGIN_NAME_MAX);
    char time_buf[MAX_TIME_STRING];
    memset(time_buf,0,MAX_TIME_STRING);
    for(;entry_ind<max_entries;entry_ind++){
        en = entries+entry_ind;

        //ignore the implied '.' and '..' entries
        if(opt_mask.halfword&NO_DOT)
          if(strcmp(basename(en->full_path),".") ==0 || strcmp(basename(en->full_path),"..")==0)
            continue;

        //if -A is not set, ignore all entries starting with a '.'
        if(!(opt_mask.halfword&YES_DOT))
          if(basename(en->full_path)[0] == '.')
            continue;

        //inode number
        if(opt_mask.halfword&INODE){
          printf("%ld",(long)en->ino_num);
          printf(SPACES);
        }
        //long-form
        if(opt_mask.halfword&LONG_LIST){
              //permision mask
              printPerm(en,((opt_mask.halfword&SPECIAL) ? SPECIAL_BITS: 0));
              printf("%s",SPACES);
              //owner of file
              if(getnamefromuid(en->uid,name_buf,LOGIN_NAME_MAX))
                errnoExit("getnamefromuid()");
              printf("%s",name_buf);
              printf("%s",SPACES);
              //group owner of file
              if(getnamefromgid(en->gid,name_buf,LOGIN_NAME_MAX))
                errnoExit("getnamefromgid()");
              printf("%s",name_buf);
              printf("%s",SPACES);

              //size of file in bytes
              printf("%ld",(long)en->size);
              printf("%s",SPACES);

              //last modification time of file
              struct tm* time_struc;
              errno = 0;
              if((time_struc=localtime(&en->t_mtime)) == NULL)
                errnoExit("localtime()");

              errno = 0;
              if(strftime(time_buf,MAX_TIME_STRING,"%b %d %I:%M",time_struc)==0)
                errnoExit("strftime()"); //checks if errno is not zero
              printf("%s",time_buf);
              printf("%s",SPACES);
        }

        char* file_name = basename(en->full_path);
        printf("%s\n",file_name);













    }


    free(entries);

        /*
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
    }*/


    exit(EXIT_SUCCESS);
}
