/*********** util.c file ****************/

#include "type.h"
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char   gpath[256];
extern char   *name[64];
extern int    n;

extern int    fd, dev;
extern int    nblocks, ninodes, bmap, imap, inode_start;
extern char   line[256], cmd[32], pathname[256];


int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

//Double check
int tokenize(char *pathname)
{
    // Tokenize str into tokens array, assume it's large enough
    printf("tokenize %s\n", pathname);	

    n = 0;
    strcpy(gpath, pathname);
    gpath[strlen(pathname)] = 0;
    
    char *temp = strtok(pathname, "/");  // first call to strtok()
    if (!temp) return 0;
    else name[n++] = temp;
    printf("%s ", temp);

    // Call strtok() until it returns NULL
    while ((name[n] = strtok(0, "/")))
    {
        printf("%s ", name[n]);
        n++;
    }

    printf("\n");
    // Return # of tokens
    return n;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, disp;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];

    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk  = (ino-1) / 8 + inode_start;
       disp = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d disp=%d\n", ino, blk, disp);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + disp;
       // copy INODE to mp->INODE
       mip->INODE = *ip;

       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

int iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write back */
 //printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino); 

 block =  ((mip->ino - 1) / 8) + inode_start;
 offset =  (mip->ino - 1) % 8;

 /* first get the block containing this inode */
 get_block(mip->dev, block, buf);

 ip = (INODE *)buf + offset;
 *ip = mip->INODE;

 put_block(mip->dev, block, buf);
} 

int search(MINODE *mip, char *dirname)
{
    // Returns ino of name if it exists in mip
    char buf[BLKSIZE], temp[EXT2_NAME_LEN];

    char* cp;
    DIR * dp;

    printf("search for %s in MINODE = [%d, %d]\n", dirname, mip->dev, mip->ino);
    for (int i = 0; i < 12; i++)
    {
        if (!mip->INODE.i_block[i])
        {
            // printf("No more blocks! %s not found!\n", name);
            break;
        }

        get_block(mip->dev, mip->INODE.i_block[i], buf);
        dp = (DIR *) buf;
        cp = buf;

        while (cp < buf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%4d %4d %4d %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

            if (!strcmp(temp, dirname))
            {
                printf("found %s : ino = %d\n", dirname, dp->inode);
                return dp->inode;
            }

            cp += dp->rec_len;
            dp = (DIR*) cp;
        }
    }

    // Couldn't find ino
    return -1;
}

int getino(char *pathname)
{
  int i, ino, blk, disp;
  INODE *ip;
  MINODE *mip;

  //printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  if (pathname[0]=='/')
    mip = iget(dev, 2);
  else
    mip = iget(running->cwd->dev, running->cwd->ino);

  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      ino = search(mip, name[i]);

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(dev, ino);
   }
   iput(mip);
   return ino;
}

int tst_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j))
     return 1;
  return 0;
}

int set_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}

