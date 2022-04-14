/************* mkdir_creat **************/
#include "functions.h"

int enter_name(MINODE *pip, int ino, char* name)
{
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    int blk;

    for (int i = 0; i < 12; i++)
    {
        if (pip->INODE.i_block[i] == 0)
        {
            blk = balloc(dev);
            pip->INODE.i_size = BLKSIZE;
            pip->INODE.i_block[i] = blk;
            pip->dirty = 1;
            get_block(dev, blk, buf);
            dp = (DIR*)buf;
            cp = buf;
            dp->name_len = strlen(name);
            strcpy(dp->name, name);
            dp->inode = ino;
            dp->rec_len = BLKSIZE;
            put_block(dev, blk, buf);
            break;//exit for loop
        }

        blk = pip->INODE.i_block[i];

        //step to last entry in the data block
        get_block(pip->dev, pip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        while(cp+dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }

        int ideal_length = 4 * ((8 + dp->name_len + 3) / 4);//all dir entries' recommended length (rec_len) = ideal_length, except last entry. Last entry's rec_len is to the end of the block
        int need_length = 4 * ((8 + strlen(name) + 3) / 4);

        int remain = dp->rec_len - ideal_length;

        if (remain >= need_length)
        {
            dp->rec_len = ideal_length;//trim the previous entry to its ideal_length
            cp += dp->rec_len;
            dp = (DIR*)cp;

            //enter the new entry as the LAST entry
            dp->inode = ino;
            strcpy(dp->name, name);
            dp->name_len = strlen(name);
            dp->rec_len = remain;
            put_block(dev, blk, buf);
            return 0;
        }
    }

    return 0;
}



int kmkdir(MINODE *pmip, char* bname)
{
    //allocate an INODE and a disk block
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //load INODE into a minode
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED; // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = BLKSIZE; // size in bytes
    ip->i_links_count = 2; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk; // new DIR has one data block
    for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    }
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk


    //make data block 0 of INODE to contain . and .. entries
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    get_block(dev, blk, buf);
    DIR *dp = (DIR *)buf;
    //char* cp = buf;

    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (DIR* )((char *)dp + 12);
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, blk, buf); // write to blk on disk

    //enter_name()
    enter_name(pmip, ino, bname);
}

int mymkdir()
{
    //divide pathname into dirname and basename
    char temppath[128];
    strcpy(temppath, pathname);
    char* dname = dirname(pathname);
    char* bname = basename(temppath);
    printf("dname=%s\n", dname);
    printf("bname=%s\n", bname);

    //get pino and pmip
    int pino = getino(dname);
    MINODE *pmip = iget(dev, pino);

    //check if dirname exists
    if (pino == -1)
    {
        printf("dir does not exist\n");
        return -1;
    }

    //check pmip->INODE is a DIR
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("inode is not dir\n");
        return -1;
    }

    //check that basename doesn't exist in parent DIR
    if (search(pmip, bname) != 0)
    {
        printf("basename exists in parent dir\n");
        return -1;
    }

    kmkdir(pmip, bname);

    pmip->INODE.i_links_count += 1;
    pmip->dirty = 1;

    iput(pmip);
}

int kcreat(MINODE *pmip, char* bname)
{
    //allocate an INODE and a disk block
    int ino = ialloc(dev);
    int blk = balloc(dev);

    //load INODE into a minode
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 33188;//0644;
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = 0;// size in bytes
    ip->i_links_count = 1;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    //ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    //ip->i_block[0] = blk; // new DIR has one data block
    /* for (int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;
    } */
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk


    /* //make data block 0 of INODE to contain . and .. entries
    char buf[BLKSIZE];
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    get_block(dev, blk, buf);
    DIR *dp = (DIR *)buf;
    //char* cp = buf;

    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    // make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, blk, buf); // write to blk on disk */

    //enter_name()
    enter_name(pmip, ino, bname);
}

int mycreat()
{
    //divide pathname into dirname and basename
    char temppath[128];
    strcpy(temppath, pathname);
    char* dname = dirname(pathname);
    char* bname = basename(temppath);
    printf("dname=%s\n", dname);
    printf("bname=%s\n", bname);

    //get pino and pmip
    int pino = getino(dname);
    MINODE *pmip = iget(dev, pino);

    //check if dirname exists
    if (pino == -1)
    {
        printf("file does not exist\n");
        return -1;
    }

    //check pmip->INODE is a DIR
    if (!S_ISDIR(pmip->INODE.i_mode))
    {
        printf("inode is not dir\n");
        return -1;
    }

    //check that basename doesn't exist in parent DIR
    if (search(pmip, bname) != 0)
    {
        printf("basename exists in parent dir\n");
        return -1;
    }

    kcreat(pmip, bname);

    //pmip->INODE.i_links_count = 1;
    pmip->dirty = 1;

    iput(pmip);
}
