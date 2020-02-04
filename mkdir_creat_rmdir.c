#include "type.h"

extern MINODE *root;
extern PROC   *running;
extern int    fd, dev;
extern char   line[256], cmd[32], pathname[256];

int enter_name(MINODE *pip, int myino, char *myname)
{
    int i, remain, needed_len = 4*((8+strlen(myname)+3)/4);
    char buf[BLKSIZE], *cp;
    DIR *dp;

    for(i=0;i<12;i++){
        if (pip->INODE.i_block[i]==0) break;
    }
    i--;
    get_block(pip->dev, pip->INODE.i_block[i], buf);
        
    dp = (DIR *)buf;
    cp = buf;
        
    printf("step to LAST entry in data block %d\n", pip->INODE.i_block[i]);
    while (cp + dp->rec_len < buf + BLKSIZE){
        printf("%s\n", dp->name);
        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    remain = dp->rec_len - 4*((8+dp->name_len+3)/4);

    if(remain >= needed_len){
        printf("enough space, inserting in block %d\n", pip->INODE.i_block[i]);
        dp->rec_len = 4*((8+dp->name_len+3)/4);
            
        cp += dp->rec_len;
        dp = (DIR *) cp;
            
        dp->inode = myino;
        dp->name_len = strlen(myname);
        dp->rec_len = remain;
        strncpy(dp->name, myname, dp->name_len);
    } else {
        i++;
        pip->INODE.i_block[i] = balloc(pip->dev);
        pip->INODE.i_size += BLKSIZE;
        printf("not enough space, inserting in new block %d\n", pip->INODE.i_block[i]);
        get_block(pip->dev, pip->INODE.i_block[i], buf);

        cp = buf;
        dp = (DIR *) cp;
        
        dp->inode = myino;
        dp->name_len = strlen(myname);
        dp->rec_len = BLKSIZE;
        strncpy(dp->name, myname, dp->name_len);
    }
    put_block(pip->dev, pip->INODE.i_block[i], buf);
    printf("wrote block to %d to insert %s into parent dir\n", pip->INODE.i_block[i], myname);
}

int mymkdir(MINODE *pip, char *name)
{
    int ino = ialloc(pip->dev);
    int bno = balloc(pip->dev);

    printf("ino = %d bno = %d\n", ino, bno);

    MINODE *mip = iget(pip->dev, ino);
    INODE *ip = &mip->INODE;

    DIR* dp;
    char *cp, buf[BLKSIZE];

    ip->i_mode = (0x41ED);      // OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    //ip->i_gid  = running->gid;// Group Id
    ip->i_size = BLKSIZE;       // Size in bytes 
    ip->i_links_count = 2;	// Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;           // LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = bno;       // new DIR has one data block   

    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;     // Set all blocks to 0
    }
    
    mip->dirty = 1;             // Set dirty for writeback
    iput(mip);

    // Initializing the newly allocated block
    get_block(mip->dev, ip->i_block[0], buf);

    dp = (DIR *) buf;
    cp = buf;

    // Create initial "." directory
    strcpy(dp->name, ".");
    dp->inode = ino;
    dp->name_len = 1;
    dp->rec_len = 12;
    
    cp += dp->rec_len;
    dp = (DIR*) cp;

    // Create initial ".." directory
    strcpy(dp->name, "..");
    dp->inode = pip->ino;
    dp->name_len = 2;
    dp->rec_len = 1012;  // Uses up the rest of the block

    // Write back initialized dir block
    put_block(mip->dev, bno, buf);
    
    enter_name(pip, ino, name);

    return ino;
}

int make_dir()
{
    char *temp, *parent, *child;
    int pino, ino;
    MINODE *start, *pip;

    if (pathname[0]=='/'){
        start = root;
    }else{
        start = running->cwd;
    }
    
    temp = strdup(pathname);
    parent = dirname(temp);
    temp = strdup(pathname);
    child = basename(temp);
  
    pino = getino(parent);
    pip = iget(start->dev, pino);

    // checking if parent INODE is a dir 
    if (S_ISDIR(pip->INODE.i_mode))
    {
        // check child does not exist in parent directory
        ino = search(pip, child);

        if (ino > 0)
        {
            printf("Child %s already exists\n", child);
            return -1;
        }
    }
    else
    {
        printf("%s is not a dir\n", parent);
        return 2;
    }
    
    mymkdir(pip, child);
    
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;
    iput(pip);

    return 0;
}

int creat_file(){
    char *temp, *parent, *child;
    int pino, ino;
    MINODE *start, *pip;

    if (pathname[0]=='/'){
        start = root;
    }else{
        start = running->cwd;
    }
    
    temp = strdup(pathname);
    parent = dirname(temp);
    temp = strdup(pathname);
    child = basename(temp);
    temp = strdup(parent);
  
    pino = getino(temp);
    pip = iget(start->dev, pino);

    // checking if parent INODE is a dir 
    if (S_ISDIR(pip->INODE.i_mode))
    {
        // check child does not exist in parent directory
        ino = search(pip, child);

        if (ino > 0)
        {
            printf("Child %s already exists\n", child);
            return -1;
        }
    }
    else
    {
        printf("%s is not a dir\n", parent);
        return 2;
    }
    
    my_creat(pip, child);
    
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;
    iput(pip);

    return 0;
}

int my_creat(MINODE *pip, char *name){
    int ino = ialloc(pip->dev);

    printf("ino = %d bno = %d\n", ino, 0);

    MINODE *mip = iget(pip->dev, ino);
    INODE *ip = &mip->INODE;

    DIR* dp;
    char *cp, buf[BLKSIZE];

    ip->i_mode = (0x81A4);      // OR 040755: DIR type and permissions
    ip->i_uid  = running->uid;	// Owner uid 
    //ip->i_gid  = running->gid;// Group Id
    ip->i_size = 0;       // Size in bytes 
    ip->i_links_count = 1;	// Links count=1
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  // set to current time
    ip->i_blocks = 2;           // LINUX: Blocks count in 512-byte chunks 
    //ip->i_block[0] = bno;       // new DIR has one data block   

    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;     // Set all blocks to 0
    }
    
    mip->dirty = 1;             // Set dirty for writeback
    iput(mip);
    
    enter_name(pip, ino, name);

    return ino;
}

int rmdir(){
    int ino, i;
    MINODE *mip, *pip;
    char *cp, buf[BLKSIZE], *parent, *child, *temp;
    DIR *dp;
    
    temp = strdup(pathname);
    parent = dirname(temp);
    temp = strdup(pathname);
    child = basename(temp);
    temp = strdup(pathname);

    ino = getino(temp);
    mip = iget(dev, ino);
    
    if(strcmp(child, ".") == 0 || strcmp(child, "..") == 0){
        printf("cannot delete inherint entry . or .. %s\n", child);
        iput(mip);
        return -1;
    }
    
    
    if(running->uid !=0 && running->uid != mip->INODE.i_uid){
        printf("permission denied\n");
        iput(mip);
        return -1;
    }
    
    // checking if mip is a dir 
    if (!S_ISDIR(mip->INODE.i_mode)){
        printf("%s is not a dir\n", child);
        return -1;
    }
    
    //checking if mip is busy
    if (mip->refCount > 1){
        printf("%s is busy\n", child);
        return -1;
    }
    
    //see if dir has anymore dir entries
    if (mip->INODE.i_links_count > 2){
        printf("%s is not a empty\n", child);
        return -1;
    }
    
    //see if dir has any file entries
    get_block(mip->dev, mip->INODE.i_block[0], buf);
    dp = (DIR *) buf;
    cp = buf;
    cp += dp->rec_len;
    dp = (DIR*) cp;
    if(dp->rec_len != 1012){
        printf("%s is not empty %d\n", child, dp->rec_len);
        return -1;
    }
    
    for (i=0; i<12; i++){
        if (mip->INODE.i_block[i]==0) continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip);
    
    pip = iget(mip->dev, getino(parent));
    rm_child(pip, child);
    
    pip->INODE.i_links_count--;
    pip->INODE.i_atime = time(0L);
    pip->INODE.i_mtime = time(0L);
    pip->dirty = 1;
    iput(pip);
    
    return 0;
}

int rm_child(MINODE *parent, char *name){
    int ideal;
    char *cp, buf[BLKSIZE], temp[EXT2_NAME_LEN];
    DIR *dp, *prev, *pprev;
    
    for (int i = 0; i < 12; i++)
    {
        if (!parent->INODE.i_block[i])
        {
            // printf("No more blocks! %s not found!\n", name);
            break;
        }

        get_block(parent->dev, parent->INODE.i_block[i], buf);
        dp = prev = (DIR *)buf;
        cp = buf;

        while (cp < buf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%4d %4d %4d %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

            if (!strcmp(temp, name))
            {
                //printf("found %s : ino = %d\n", name, dp->inode);
                ideal = 4*((8+dp->name_len+3)/4);
                if (dp->rec_len > ideal){
                    //printf("removing from end of block\n");
                    prev->rec_len = dp->rec_len+ideal;
                } else if (prev == dp) {
                    //printf("removing from only entry in block\n");
                    bdalloc(parent->dev, parent->INODE.i_block[i]);
                    parent->INODE.i_size -= BLKSIZE;
                    for(int j = i; j<11; j++){
                        if(!parent->INODE.i_block[j]) break;
                        parent->INODE.i_block[j] = parent->INODE.i_block[j+1];
                    }
                } else if (dp->rec_len == ideal) {
                    //printf("removing from middle of block\n");
                    char *prev_cp = cp;
                    prev = dp;
                    cp += dp->rec_len;
                    dp = (DIR*) cp;
                    while(cp < buf + BLKSIZE){
                        //printf("cp:%x bufblkideal:%x\n", cp, buf+BLKSIZE);

                        prev->inode = dp->inode;
                        prev->name_len = dp->name_len;
                        prev->rec_len = dp->rec_len;
                        strncpy(prev->name, dp->name, dp->name_len);
                        printf("%d %d %d %s\n", prev->inode, prev->name_len, prev->rec_len, prev->name);
                        pprev = prev;
                        cp += dp->rec_len;
                        prev_cp += prev->rec_len;
                        prev = (DIR*) prev_cp;
                        dp = (DIR*) cp;
                    }
                    pprev->rec_len += ideal;
                } else{
                	printf("Unaccounted case\n");
                	return -1;
                }
                //printf("Writing out block %d\n", parent->INODE.i_block[i]);
                put_block(parent->dev, parent->INODE.i_block[i], buf);
                return 0;
            }

            cp += dp->rec_len;
            prev = dp;
            dp = (DIR*) cp;
        }
    }
}
