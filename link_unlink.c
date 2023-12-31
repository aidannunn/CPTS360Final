/*********** link_unlink.c file ****************/
#include "functions.h"

int myLink(char* old_file, char* new_file)
{
  //check if old file exists and is not directory
  int oino = getino(old_file);
  MINODE* omip = iget(dev, oino);
  if ((omip->INODE.i_mode & 0xF000) == 0x4000) {
    printf("Link failed, %s is a directory\n", old_file);
    return -1;
  }
  if (getino(new_file) != 0) {//check if new file already exists
    printf("Link failed, %s already exists\n", new_file);
    return -1;
  }
  char* parent = dirname(new_file);
  char* child = basename(new_file);
  int pino = getino(parent);
  MINODE* pmip = iget(dev, pino);
  // creat entry in new parent DIR with same inode number of old_file
  enter_name(pmip, oino, child);
  omip->INODE.i_links_count++; // inc INODE’s links_count by 1
  omip->dirty = 1; // for write back by iput(omip)
  iput(omip);
  iput(pmip);
  return 0;
}

int myUnlink()
{
  char* parent, *child;
  char buf[BLKSIZE];
  //Check if file can be unlinked
  int ino = getino(pathname);
  MINODE* mip = iget(dev, ino);
  if ((mip->INODE.i_mode & 0xF000) == 0x4000) {
    printf("unlink failed, %s is a directory\n", pathname);
    return -1;
  }
  if (ino == 0)
    return -1;
  //remove name entry from parent DIR's data block
  parent = dirname(pathname); child = basename(pathname);
  int pino = getino(parent);
  MINODE* pmip = iget(dev, pino);
  rm_child(pmip, child);
  pmip->dirty = 1;
  iput(pmip);
  mip->INODE.i_links_count--;
  if (mip->INODE.i_links_count > 0)
    mip->dirty = 1;
  else
  {
    //deallocate all data blocks in INODE
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    //deallocate INODE
    idalloc(mip->dev, mip->ino);
  }
  iput(mip);
}
