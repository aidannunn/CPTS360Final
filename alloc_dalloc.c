#include "functions.h"

int tst_bit(char *buf, int bit) // in Chapter 11.3.1
{
    return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit) // in Chapter 11.3.1
{
    buf[bit/8] |= (1 << (bit % 8));
}

int clr_bit(char* buf, int bit)
{
    buf[bit/8] &= ~(1 << (bit%8));
}

int decFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // dec free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int idalloc(int dev, int ino)
{
  int i;  
  char buf[BLKSIZE];

  if (ino > ninodes){
    printf("inumber %d out of range\n", ino);
    return -1;
  }

  // get inode bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);

  // write buf back
  put_block(dev, imap, buf);

  // update free inode count in SUPER and GD
  incFreeInodes(dev);
}

int bdalloc(int dev, int bno)
{
    int i;
    char buf[BLKSIZE];

    if (bno > nblocks)
    {
        printf("inumber %d out of range\n", bno);
        return -1;
    }

    // get inode bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, bno-1);

    // write buf back
    put_block(dev, bmap, buf);

    // update free inode count in SUPER and GD
    incFreeInodes(dev);    
}

int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
    int  i;
    char buf[BLKSIZE];


    // read inode_bitmap block
    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++)
    { // use ninodes from SUPER block
        if (tst_bit(buf, i)==0)
        {
            set_bit(buf, i);
            put_block(dev, imap, buf);

            decFreeInodes(dev);

            printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
            return i+1;
        }
    }
    return 0;
}

// WRITE YOUR OWN balloc(dev) function, which allocates a FREE disk block number
int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++)
  {
    if (tst_bit(buf, i)==0)
    {
        set_bit(buf, i);
	    put_block(dev, bmap, buf);

	    decFreeInodes(dev);

	    printf("allocated block = %d\n", i+1);
        return i+1;
    }
  }

  return 0;
}

