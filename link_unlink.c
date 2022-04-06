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

int mySymlink(char* old_file, char* new_file)
{
  int ino;
  MINODE* mip;
  //check if old_file exists and new_file does NOT exist
  if (getino(old_file) == 0) {
    printf("Symlink failed, %s does not exist\n", old_file);
    return -1;
  }
  if (getino(new_file) != 0) {//check if new file already exists
    printf("Link failed, %s already exists\n", new_file);
    return -1;
  }
  //creat new_file; change new_file to LNK type;
  strncpy(pathname, new_file, sizeof(pathname) - 1);
  pathname[strlen(pathname)] = 0;
  mycreat();
  //change new_file to LNK type;
  ino = getino(new_file);
  mip = iget(dev, ino);
  mip->INODE.i_mode = 0x41ED;
  //strcpy(mip->INODE.i_block, old_file);
  mip->INODE.i_size = strlen(old_file);
  mip->dirty = 1;
  iput(mip);
  return 0;
}

int myReadlink(MINODE* mip, char* buf)
{
  if (S_ISLNK(mip->INODE.i_mode) != 0) {
    printf("Not a LNK file\n");
    return -1;
  }
  strcpy(buf, (char *)mip->INODE.i_block);
  return strlen(buf);
}
