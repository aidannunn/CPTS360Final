/*********** misc.c file ****************/
#include "functions.h"

int myStat(char* filename)
{
  MYST stat;
  int ino;
  INODE *ip;
  MINODE* mip;
  ino = getino(filename);
  mip = iget(dev, ino);
  stat.myst_dev = mip->dev;
  stat.myst_ino = mip->ino;
  ip = &(mip->INODE);
  stat.myst_mode = ip->i_mode;
  stat.myst_uid = ip->i_uid;
  stat.myst_nlink = ip->i_links_count;
  stat.myst_gid = ip->i_gid;
  stat.myst_size = ip->i_size;
  stat.myst_blksize = BLKSIZE;
  stat.myst_blocks = ip->i_blocks;
  stat.myst_atime = ip->i_atime;
  stat.myst_mtime = ip->i_mtime;
  stat.myst_ctime = ip->i_ctime;
  iput(mip);
}

int myChmod(char* filename, int mode)
{
  int ino;
  MINODE* mip;
  ino = getino(filename);
  mip = iget(dev, ino);
  mip->INODE.i_mode |= mode;
  mip->dirty = 1;
  iput(mip);
}

int myUtime(char* filename)
{
  int ino;
  MINODE* mip;
  ino = getino(filename);
  mip = iget(dev, ino);
  mip->INODE.i_atime = time(0L);
  mip->dirty = 1;
  iput(mip);
}
