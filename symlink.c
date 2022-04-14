/*********** symlink.c file ****************/
#include "functions.h"

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
  mip->INODE.i_mode = 0xA1ED;
  strcpy((char*)mip->INODE.i_block, old_file);
  mip->INODE.i_size = strlen(old_file);
  mip->dirty = 1;
  iput(mip);
  return 0;
}

int myReadlink(MINODE* mip, char* buf)
{
  if (!S_ISLNK(mip->INODE.i_mode)) {
    return -1;
  }
  strcpy(buf, (char *)mip->INODE.i_block);
  return strlen(buf);
}
