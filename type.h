/*************** type.h file ************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE    64
#define NFD         8
#define NPROC       2

typedef struct minode{
  INODE INODE;
  int dev, ino;
  int refCount;
  int dirty;
  // for level-3
  int mounted;
  struct mntable *mptr;
}MINODE;


typedef struct oft{ // for level-2
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;
  int          uid;
  int          status;
  MINODE      *cwd;
  OFT         *fd[NFD];
}PROC;

//*******************FUNCTION DECLARATIONS*****************
//UTIL.c
int get_block(int dev, int blk, char buf[]);
int put_block(int dev, int blk, char buf[]);
int tokenize(char *pathname);
MINODE *iget(int dev, int ino);
int iput(MINODE *mip);
int getino(char *pathname);
int search(MINODE *mip, char *name);
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int clr_bit(char *buf, int bit);

//cd_ls_pwd.c
change_dir(char *pathname);
int list_file(char *pathname);
int pwd(MINODE *wd);

//free_alloc.c
int ialloc(int dev);
int balloc(int dev);
int idalloc(int dev, int ino);
int bdalloc(int dev, int blk);

//mkdir_creat_rmdir.c
int make_dir();
int rmdir();
int enter_name(MINODE *pip, int myino, char *myname);
int rm_child(MINODE *parent, char *name);
int creat_file();
int my_creat(MINODE *pip, char *name);

//link.c
int link(char *oldname, char *newname);
int unlink(char *pathname);
int symlink(char *oldname, char *newname);
char* readlink(char *pathname);
