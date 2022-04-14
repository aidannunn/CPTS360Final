/************* rmdir **************/
#include "functions.h"

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