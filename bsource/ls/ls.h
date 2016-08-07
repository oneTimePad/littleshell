#ifndef LS_H
#define LS_H

#define NOMEMBUF(buf, str)  (strlen(str) + 1 > buf) ? TRUE: FALSE
#define LONG_LIST 1
#define YES_DOT   2
#define NO_DOT    4
#define AUTHOR    8
#define INODE     16
#define DEREF     32
#define NO_GRP    64
#define RECUR     128
#define SRT_SIZE  256
#define SRT_MODF  512
#define HELP      1024
#define SPECIAL   2048
#define VERBOSE   4096

#ifdef _GNU_SOURCE
#include <linux/limits.h>
#define PATH_LIM PATH_MAX+NAME_MAX+1
#else
#define PATH_LIM 8012
#endif



typedef struct _FILE_ENTRY{
    mode_t   perm;    //permission bit-mask
    ino_t    ino_num; //inode number for file
    nlink_t  hlinks;  //number of hard links
    uid_t    uid;     //owner id
    gid_t    gid;     //id of group who owns file
    off_t    size;    //size of file in bytes
    blkcnt_t phy_blks;//size of file in physical blocks
    time_t   t_atime; //access time
    time_t   t_mtime; //modification time
    time_t   t_ctime; //status change time
    char     full_path[PATH_LIM];
} FILE_ENTRY;



typedef union _LS_OPTIONS{

  struct{
    unsigned char l:1; //long list form
    unsigned char a:1; //do not  ignore entries with .
    unsigned char A:1; //do not list implied . and ..
    unsigned char u:1; //with -l print the author of each file
    unsigned char i:1; //print the index number of each file
    unsigned char L:1; //dereference symbolic links
    unsigned char o:1; //like -l, but do not list group information
    unsigned char R:1; //list subdirectories recursively
    unsigned char s:1; //sort by file size
    unsigned char t:1; //sort by modification time, newest first
    unsigned char h:1; //display this help and exit
    unsigned char p:1; //print special bits in perm maks
    unsigned char v:1; //verbose
    unsigned char unk2:1;
    unsigned char unk3:1;
    unsigned char unk4:1;
  } bits;

  short int halfword;

} LS_OPTIONS;
#endif
