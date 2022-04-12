/************* mkdir_creat_rmdir **************/
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

int rm_child(MINODE *pmip, char *name)
{
    char buf[BLKSIZE];
    char tempName[BLKSIZE];
    DIR *dp, *follower;//add follower pointer to keep track of record right beforehand
    char *cp;
    INODE *ip;
    int flag = 0;
    int i = 0;

    printf("removing child %s\n", name);
    //search parent INODE's data block(s) for the entry of name
    for (; i < 12; i++)
    {
        if (flag)
        {
            break;//exit for loop
        }
        ip = &pmip->INODE;
        get_block(dev, ip->i_block[i], buf);
        dp = (DIR*)buf;
        cp = buf;
        while(cp < buf + BLKSIZE)
        {
            //printf("name=%s\n", name);
            strncpy(tempName, dp->name, dp->name_len);
            tempName[dp->name_len] = 0;//remove newline that's in dp->name
            //printf("dp->name=%s\n", dp->name);
            //getchar();
            if (strcmp(tempName, name)==0)
            {
                i--;
                printf("found %s\n", dp->name);
                flag = 1;
                break;//exit while loop search
            }
            follower = dp;
            cp += dp->rec_len;
            dp = (DIR*)cp;
        }
    }

    //delete name entry from parent directory
    //if first and only entry in a data block
    if (dp->rec_len == BLKSIZE && cp == buf)
    {
        printf("case 1: first and only entry in a data block\n");
        //deallocate the data block
        bdalloc(dev, ip->i_block[i]);
        //reduce parent's file size by BLKSIZE
        ip->i_size -= BLKSIZE;

        //compact parent’s i_block[ ] array to eliminate the deleted entry if it’s between nonzero entries
        //look through i_blocks, starting at i, remove the one we stopped on, iterate through the rest and move their numbers backwards to backfill, add 0 to the last one
        //putblock in loop to update afer each edit
        //bzero whole block, write fresh block back to memory
        i++;
        for (; i < 12; i++)
        {
            if (ip->i_block[i+1] != 0)
            {
                get_block(dev, ip->i_block[i], buf);
                put_block(dev, ip->i_block[i-1], buf);
            }
        }
        ip->i_block[i] = 0;
        put_block(dev, ip->i_block[i], buf);
    }
    //else if LAST entry in block
    else if((cp + dp->rec_len) == buf + BLKSIZE)
    {
        printf("case 2: LAST entry in block\n");
        //absorb rec_len into the predecessor entry
        follower->rec_len += dp->rec_len;
        put_block(dev, ip->i_block[i], buf);
    }
    //if entry is first but not the only entry or in the middle of a block:
    else if (dp->name != 0)
    {
        printf("case 3: entry is first but not the only entry or in the middle of a block\n");
        //find last entry
        DIR* endDp = dp;
        char* endCp = cp;

        while(endCp + endDp->rec_len < buf + BLKSIZE)
        {
            endCp += endDp->rec_len;
            endDp = (DIR*)endCp;
        }

        int tempLen = dp->rec_len;

        //move trailing entries left
        memcpy(dp, cp + dp->rec_len, (&buf[BLKSIZE]-cp));

        endCp -= tempLen;
        endDp = (DIR*)endCp;

        //add deleted rec_len to the LAST entry
        endDp->rec_len += tempLen;

        put_block(dev, ip->i_block[i], buf);
    }

    return 0;
}

int myrmdir()
{
    //get in-memory INODE of pathname
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);

    //verify INODE is a DIR (by INODE.i_mode field)
    //check pmip->INODE is a DIR
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("inode is not dir\n");
        return -1;
    }

    //verify minode is not busy
    if (mip->refCount > 1)
    {
        printf("minode is busy\n");
        return -1;
    }

    //verify DIR is empty (traverse data blocks for number of entries = 2)
    if (mip->INODE.i_links_count > 2)
    {
        printf("DIR is not empty->links");
        return -1;
    }
    if (mip->INODE.i_links_count == 2)
    {
        int entries = 0;
        for (int i = 0; i < 12; i++)
        {
            char* buf[BLKSIZE];
            get_block(dev, mip->INODE.i_block[i], (char*)buf);
            DIR *dp = (DIR*)buf;
            char *cp = (char*)buf;
            //printf("cp=%d\n", cp);
            //printf("dp->rec_len=%d\n", dp->rec_len);

            while((int*)cp < ((int*)buf + BLKSIZE))
            {
                printf("dp->name=%s\n", dp->name);
                //printf("adding entry\n");
                //printf("name = %s\n", dp->name);
                if (strcmp(dp->name, "")==0)
                {
                    printf("leaving loop\n");
                    break;
                }
                //printf("cp=%d\n", cp);
                //printf("dp->rec_len=%d\n", dp->rec_len);
                //printf("dp file type=%d\n", dp->file_type);
                entries += 1;
                cp += dp->rec_len;
                dp = (DIR*)cp;
                //getchar();
            }

            if (entries > 2)
            {
                printf("DIR is not empty");
                return -1;
            }
        }
    }


    printf("finally out of loops\n");
    //get parent's ino and inode
    int pino = findino(mip, &ino);
    printf("getting pmip\n");
    MINODE *pmip = iget(mip->dev, pino);

    printf("found pino\n");
    //get name from parent DIR's data block
    char tempName[BLKSIZE];
    findmyname(pmip, ino, tempName);

    printf("found my name\n");
    //remove name from parent directory
    rm_child(pmip, tempName);

    printf("did rm_child\n");

    //dec parent links_count by 1
    pmip->INODE.i_links_count--;
    //mark parent pmip dirty
    pmip->dirty = 1;
    iput(pmip);

    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);

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
