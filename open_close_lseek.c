/************* open_close_lseek **************/
#include "functions.h"


int open_file(int mode)
{
    int i;
    //1. get file's minode
    int ino = getino(pathname);
    if (ino == 0)//file does not exist
    {   
        mycreat();//creat the file
        ino = getino(pathname);//then get the ino
    }
    MINODE *mip = iget(dev, ino);

    //check mip->INODE.i_mode to verify it's a regular file and permission OK
    if (mip->INODE.i_mode != 33188)
    {
        printf("file is not a REGULAR file\n");
        return -1;
    }

    //check whether the file is already opened with INCOMPATIBLE mode
    for (i = 0; i<NOFT; i++){
        if (oft[i].minodePtr == mip){
            if (oft[i].mode == 1 || oft[i].mode == 2 || oft[i].mode == 3){
                printf("file is already open as an incompatible mode\n");
                return -1;
            }
        }
    }
    OFT *oftp;
    //2. allocate an openTable entry OFT; initialize OFT entries
    for (i = 0; i<NOFT; i++)
    {
        if (oft[i].minodePtr == 0){
            oftp = &oft[i];
        }
    }
    
    oftp->mode = mode; // mode = 0|1|2|3 for R|W|RW|APPEND 
    oftp->refCount = 1;
    oftp->minodePtr = mip; // point at the file's minode[]

    //Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
    switch(mode)
    {
        case 0 : oftp->offset = 0;   // R: offset = 0
                 break;
        case 1 : truncate(mip);      // W: truncate file to 0 size
                 oftp->offset = 0;
                 break;
        case 2 : oftp->offset = 0;   // RW: do NOT truncate file
                 break;
        case 3 : oftp->offset = mip->INODE.i_size;   //APPEND mode
                 break;
        default: printf("invalid mode\n");
                 return -1;
    }

    //3. search for the first FREE fd[index] entry with the lowest index in PROC
    for (i = 0; i < NFD; i++)
    {
        if (running->fd[i]->refCount == 0){
            running->fd[i] = &oftp;
        }
    }

    //update INODE's time field
    if (mode == 1)
    {
        mip->INODE.i_atime = time(NULL);
    }
    else
    {
        mip->INODE.i_atime = mip->INODE.i_mtime = time(NULL);
    }
    
    mip->dirty = 1;
    //4. return index as file descriptor
    return i;
}

int truncate(MINODE* mip)
{
    int i, buf12[256],  buf13[256], dbuf[256];
    //1. release mip->INODE's data blocks;
    //a file may have 12 direct blocks, 256 indirect blocks and 256*256
    //double indirect data blocks. release them all.

    //direct blocks
    for (i = 0; i<12; i++)
    {
        if (mip->INODE.i_block[i]){
            bdalloc(dev, mip->INODE.i_block[i]);
        }
    }

    //indirect blocks
    if (mip->INODE.i_block[12])
    {
        get_block(dev, mip->INODE.i_block[12], (char *) buf12);
        
        for (i=0; i < 256; i++)
        {
            bdalloc(dev, buf12[i]);
        }
    }

    //double indirect blocks
    if (mip->INODE.i_block[13])
    {
        get_block(dev, mip->INODE.i_block[13], (char *) buf13);
        for (i=0; i<256; i++)
        {
            if (buf13[i])
            {
                get_block(dev, buf13[i], (char *) dbuf);

                for (i=0; i<256; i++)
                {
                    if (dbuf[i]){
                        bdalloc(dev, dbuf[i]);
                    }
                }
                bzero(dbuf, 256);
            }
        }
    }

    //2. update INODE's time field
    mip->INODE.i_ctime = time(NULL);

    //3. set INODE's size to 0 and mark Minode[ ] dirty
    mip->INODE.i_size = 0;
    mip->dirty = 1;
}