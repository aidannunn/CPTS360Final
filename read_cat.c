/*********** read_cat.c file ****************/
#include "functions.h"
int myread(int fd, char *buf, int nbytes)
{
  int count = 0, lbk = 0, startByte = 0, avil, blk, remain;
  char readbuf[BLKSIZE], doublebuf[BLKSIZE], *cq;
  MINODE *mip;
  OFT *oftp;

  bzero(buf, BLKSIZE); //clears buf for new read
  oftp = running->fd[fd];
  mip = oftp->minodePtr;
  //avil = fileSize - OFT's offset // number of bytes still available in file.
  avil = mip->INODE.i_size - oftp->offset;
  cq = buf;                // cq points at buf[ ]

  printf("********* read file %d  %d *********\n", fd, nbytes);
  while (nbytes && avil)
  {
    printf("enter myread : file %d  size = %d  offset = %d\n",
    fd, mip->INODE.i_size, oftp->offset);
    //Compute LOGICAL BLOCK number lbk and startByte in that block from offset;

    lbk       = oftp->offset / BLKSIZE;
    startByte = oftp->offset % BLKSIZE;

    printf("lbk=%d start=%d size=%d\n", lbk, startByte, mip->INODE.i_size);

    // I only show how to read DIRECT BLOCKS. YOU do INDIRECT and D_INDIRECT

    if (lbk < 12){                     // lbk is a direct block
        blk = mip->INODE.i_block[lbk]; // map LOGICAL lbk to PHYSICAL blk
    }
    else if (lbk >= 12 && lbk < 256 + 12) {
         //  indirect blocks
         get_block(mip->dev, mip->INODE.i_block[12], readbuf);
         blk = readbuf[lbk - 12];
    }
    else{
         //  double indirect blocks, mailman algorithm
         get_block(mip->dev, mip->INODE.i_block[13], readbuf);
         lbk = lbk - 256 - 12;
         get_block(mip->dev, readbuf[lbk / 256], doublebuf);
         blk = doublebuf[lbk % 256];
    }

    /* get the data block into readbuf[BLKSIZE] */
    get_block(mip->dev, blk, readbuf);

    /* copy from startByte to buf[ ], at most remain bytes in this block */
    char *cp = readbuf + startByte;
    remain = BLKSIZE - startByte;   // number of bytes remain in readbuf[]

    if (remain < nbytes) {
      memcpy(cq, cp, nbytes);
      cq += nbytes;
      cp += nbytes;
      count += nbytes;
      oftp->offset += nbytes;
      avil -= nbytes;
      remain -= nbytes;
      nbytes = 0;
    }
    else {
      memcpy(cq, cp, remain);
      cq += remain;
      cp += remain;
      count += remain;
      oftp->offset += remain;
      avil -= remain;
      nbytes -= remain;
      remain = 0;
    }
      if (nbytes <= 0 || avil <= 0)
        break;

      // if one data block is not enough, loop back to OUTER while for more ...
  }
  printf("****************************************\n");
  printf("exit myread: read %d char from file descriptor %d\n", count, fd);
  printf("exit myread: file %d offset = %d\n", fd, oftp->offset);
  printf("****************************************\n");
  printf("%s\n", buf);
  return count;   // count is the actual number of bytes read
}

int read_file()
{
  char buf[BLKSIZE];
  //ask for a fd  and  nbytes to read;
  int fd = 0, nbytes = 0;
  printf("Enter file descriptor to read from\n");
  scanf("%d", &fd);
  printf("Enter number of bytes to read\n");
  scanf("%d", &nbytes);
  //verify that fd is indeed opened for RD or RW;
  if (running->fd[fd] == 0 || (running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)) {//R|W|RW|APP
    printf("fd is not opened for RD or RW\n");
    return -1;
  }
  return(myread(fd, buf, nbytes));
}

int myCat(char* filename)
{
  char mybuf[1024];  // a null char at end of mybuf[ ]
  int fd = open_file(0);
  if (fd == -1)
    return -1;
  myread(fd, mybuf, BLKSIZE);
  close_file(fd);
}
