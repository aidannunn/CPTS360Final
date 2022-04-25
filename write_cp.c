/*********** write_cp.c file ****************/
#include "functions.h"
int mywrite(int fd, char *buf, int nbytes)
{
  int count = 0, lbk = 0, blk, remain, startByte;
  char *cp;
  MINODE *mip;
  OFT *oftp;
  char *cq = buf;
  oftp = running->fd[fd];
  mip = oftp->minodePtr;
  while (nbytes > 0)
  {
    lbk       = oftp->offset / BLKSIZE;
    startByte = oftp->offset % BLKSIZE;

    if (lbk < 12) {                         // direct block
      if (mip->INODE.i_block[lbk] == 0){   // if no data block yet
         mip->INODE.i_block[lbk] = balloc(mip->dev);// MUST ALLOCATE a block
      }
      blk = mip->INODE.i_block[lbk];      // blk should be a disk block now
    }
    else if (lbk >= 12 && lbk < 256 + 12){ // INDIRECT blocks
      // HELP INFO:
      if (mip->INODE.i_block[12] == 0){
          int ibuf[256];
          //allocate a block for it;
          mip->INODE.i_block[12] = balloc(mip->dev);
          //zero out the block on disk !!!!
          get_block(mip->dev, mip->INODE.i_block[12], (char*)ibuf);
          int *arr = ibuf;
          for (int i = 0; i < 256; i++) {
            arr[i] = 0;
          }
          put_block(mip->dev, mip->INODE.i_block[12], (char*)ibuf);
          mip->INODE.i_blocks++;;
      }
      //i_block[12] exists
      //get i_block[12] into an int ibuf[256];
      int ibuf[256];
      get_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);
      blk = ibuf[lbk - 12];
      if (blk==0){
        //allocate a disk block;
        blk = ibuf[lbk - 12] = balloc(mip->dev);
        mip->INODE.i_blocks++;
        //record it in i_block[12];
        put_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);
        }
     }
     else {
       // double indirect blocks */
       int ibuf[BLKSIZE], inbuf[BLKSIZE], dinbuf[BLKSIZE];
       if (mip->INODE.i_block[13] == 0) {
         //Make and Allocate
         mip->INODE.i_block[13] = balloc(mip->dev);
         get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
         int *arr = ibuf;
         for (int i = 0; i < 256; i++) {
           arr[i] = 0;
         }
         put_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
         mip->INODE.i_blocks++;
       }
       get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
       lbk = lbk - 256 - 12;
       int indirect = ibuf[lbk / 256];
       if (indirect == 0) {
         //Make and Allocate
         get_block(mip->dev, indirect, (char *)inbuf);
         int *arr = inbuf;
         for (int i = 0; i < 256; i++) {
           arr[i] = 0;
         }
         put_block(mip->dev, indirect, (char *)inbuf);
         mip->INODE.i_blocks++;
         put_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
       }

       get_block(mip->dev, indirect, (char *)dinbuf);
       if (dinbuf[lbk % 256] == 0) {
         //Make and Allocate
         dinbuf[lbk % 256] = balloc(mip->dev);
         mip->INODE.i_blocks++;
         put_block(mip->dev, indirect, (char *)dinbuf);
       }
       //blk = dinbuf[lb % 256];
    }

    char wbuf[BLKSIZE];
    get_block(mip->dev, blk, wbuf);
    cp = wbuf + startByte;
    remain = BLKSIZE - startByte;

    if (remain < nbytes)
      nbytes = remain;

    memcpy(cp, cq, nbytes);
    count += nbytes;
    oftp->offset += nbytes;
    remain -= nbytes;
    if (oftp->offset > mip->INODE.i_size)  // especially for RW|APPEND mode
        mip->INODE.i_size += nbytes;    // inc file size (if offset > fileSize)
    nbytes = 0;

    put_block(mip->dev, blk, wbuf);
    // loop back to outer while to write more .... until nbytes are written
  }
  mip->dirty = 1;
  printf("wrote %d char into file descriptor fd=%d\n", count, fd);
  return count;
}

int write_file()
{
  int fd, nbytes;
  char towrite[BLKSIZE], buf[BLKSIZE];
  printf("Enter file descriptor to read from\n");
  scanf("%d", &fd);
  printf("Enter string to write\n");
  while ((getchar()) != '\n'); //This will consume the '\n' char
  fgets(towrite, BLKSIZE, stdin);
  towrite[strlen(towrite)-1] = 0;
  if (running->fd[fd] == 0 || running->fd[fd]->mode == 0) {//R|W|RW|APP
    printf("fd is not opened for Write\n");
    return -1;
  }
  nbytes = strlen(towrite);
  return(mywrite(fd, towrite, nbytes));
}

int myCP(char* src, char* dest) //cp src dest
{
  char buf[BLKSIZE];
  //open src for read and dest for write
  strcpy(pathname, src);
  int fd = open_file(0);
  strcpy(pathname, dest);
  int gd = open_file(1);

  //Copy
  int count;
  while (1) {
    count = myread(fd, buf, BLKSIZE);
    if (count == 0)
      break;
    buf[count] = 0;
    mywrite(gd, buf, count);
  }
  //close src and dest
  close_file();
  strcpy(pathname, src);
  close_file();
}

int myMV(char* src, char* dest)
{
  //Check if src exists
  int ino = getino(src);
  MINODE* mip = iget(dev, ino);
  if (ino == 0) {
    printf("Source file %s does not exist\n", src);
    return -1;
  }
  //Check if src is same dev as dev (THIS CHECK IS NOT CORRECT, FIX IN LEVEL 3)
  if (mip->dev == dev)
  {
    //case1: same dev
    myLink(src, dest);
    strcpy(pathname, src);
    myUnlink();
  }
  else
  {
    //case2: Different dev
    myCP(src, dest);
    strcpy(pathname, src);
    myUnlink();
  }
}
