#ifndef SETFACL_H
#define SETFACL_H


#define SET         1
#define MODIFY      2
#define REMOVE      4
#define SET_FILE    8
#define MODIFY_FILE 16
#define REMOVE_FILE 32
#define REMOVE_ALL  64
#define REMOVE_DEF  128
#define NO_MASK     256
#define MASK        512
#define DEFAULT     1024
#define RESTORE     2048
#define TEST        4096
#define RECURSIVE   8012
#define LOGICAL     16024
#define PHYSICAL    32048
#define VERSION     64096
#define HELP        128192
#define EOF         256384
#define STIN        512768

#define MAX_ACL_ENTRIES 10


#define READ  1
#define WRITE 2
#define EXEC  4

typedef union _SETFACL_OPTIONS {

  struct{
    char m:1;
    char b:1;
    char k:1;
    char n:1;
    char mask:1;
    char d:1;
    char restore:1;
    char test:1;
    char R:1;
    char L:1;
    char P:1;
    char v:1;
    char h:1;
    char eof:1;
    char stdin:1;
  } bits;

  unsigned int word;


} SFA_OPTIONS;

typedef union ACL_PERM_MASK{
  struct{
    char r:1;
    char w:1;
    char x:1;
    char none:1;
  } bits;
  unsigned char nibble:4;
} PERM;

typedef struct _ACLENTRY{
  acl_tag_t tag;
  uid_t u_qual
  git_t g_qual;
  PERM  permissions;

} ACLENTRY;

_BOOL setpermzero(PERM*);


#endif
