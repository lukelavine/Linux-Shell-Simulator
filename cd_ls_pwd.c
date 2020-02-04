/************* cd_ls_pwd.c file **************/

#include "type.h"
/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern char   gpath[256];
extern char   *name[64];
extern int    n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

#define OWNER  000700
#define GROUP  000070
#define OTHER  000007

char *t = "xwrxwrxwr";

int cd(char* pathname)
{
    MINODE* mip;
    int ino;

    if (pathname && pathname[0])
    {
        ino = getino(pathname);
        
        if (ino > 0)
        {
            mip = iget(dev, ino);

            if(S_ISDIR(mip->INODE.i_mode))
            {
                iput(running->cwd);
                running->cwd = mip;
                return 0;
            }
            else
            {
                printf("%s is not a directory\n", pathname);
                return 2;
            }
        }
        else
        {
            printf("%s does not exist\n", pathname);
            return 1;
        }
    }
    else  // Just cd into root
    {
        iput(running->cwd);
        running->cwd = root;
        return 0;
    }
}

int ls_file(int ino, char *name) {
	int i;
	char ftime[64];
	
	MINODE *mip = iget(dev, ino);
	INODE *ip = &(mip->INODE);
	
	if (S_ISREG(ip->i_mode)) printf("-");
	if (S_ISDIR(ip->i_mode)) printf("d");
	if (S_ISLNK(ip->i_mode)) printf("l");
	
	for( i=8; i>=0; i--){
		if(ip->i_mode & (1 << i)) printf("%c", t[i]);
		else printf("-");
	}
	
	printf("%4d ", ip->i_links_count);
	printf("%4d ", ip->i_gid);
	printf("%4d ", ip->i_uid);
	
	strcpy(ftime, ctime(&ip->i_ctime));
	ftime[strlen(ftime)-1] = 0;
	printf("%s ", ftime);
	
	printf("%6d ", ip->i_size);
	
	printf("%s", name);
	printf("\n");
	
	iput(mip);
}

int ls_dir(char *dirname){
	
    int ino;
    MINODE *wd, *mip;
    char *cp, temp[256], sbuf[BLKSIZE];

    if (!dirname || !dirname[0])
    {
        wd = running->cwd;
        ino = running->cwd->ino;
    }
    else
    {
        if (dirname[0] == '/')
        {
            wd = root;
            dirname++;
        }
        else
        {
            wd = running->cwd;
        }
        ino = getino(dirname);
    }

    mip = iget(wd->dev, ino);
    

    if (S_ISDIR(mip->INODE.i_mode))
    {
        for (int i = 0; i < 12; i++)
        {
            if (mip->INODE.i_block[i] == 0)
                break;

            get_block(wd->dev, mip->INODE.i_block[i], sbuf);
            dp = (DIR *) sbuf;
            cp = (char *) sbuf;

            while (cp < sbuf + BLKSIZE)
            {
                //printf("cp:%x sbufBLK:%x rlen: %d\n", cp, sbuf+BLKSIZE, dp->rec_len);
                strncpy(temp, dp->name, dp->name_len);
                temp[dp->name_len] = '\0';
                ls_file(dp->inode, temp);

                cp += dp->rec_len;  // Move to next record
                dp = (DIR *)cp;
            }
        }
    }
    else
    {
        ls_file(ino, dirname);
        //printf("%s is not a dir\n", dirname);
    }

    iput(mip);

    return 0;
}

int rpwd(MINODE *wd)
{
    char buf[BLKSIZE], myname[EXT2_NAME_LEN];
    int my_ino, parent_ino;

    DIR* dp;
    char* cp;

    MINODE* pip;  // Parent MINODE

    if (wd == root)
    {
        return 0;
    }

    my_ino = search(wd, ".");
    parent_ino = search(wd, "..");

    pip = iget(wd->dev, parent_ino);
    
    get_block(wd->dev, pip->INODE.i_block[0], buf);
    dp = (DIR *) buf;
    cp = buf;

    while(cp < buf + BLKSIZE)
    {
        strncpy(myname, dp->name, dp->name_len);
        myname[dp->name_len] = '\0';
        
        if(dp->inode == my_ino)
        {
            break;
        }

        cp += dp->rec_len;
        dp = (DIR*) cp;
    }

    rpwd(pip);
    iput(pip);

    // Prints this part of cwd
    printf("/%s", myname);
    return 0;
}

change_dir(char *pathname)
{
  cd(pathname);
}


int list_file(char *pathname)
{
  ls_dir(pathname);
}


int pwd(MINODE *wd)
{

    if(wd == root)
    {
        printf("/");
    }
    else
    {
        rpwd(wd);
    }
    printf("\n");

    return 0;
}
