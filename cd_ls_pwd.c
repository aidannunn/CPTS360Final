/************* cd_ls_pwd.c file **************/
#include "functions.h"

int cd()
{
  //printf("cd: under construction READ textbook!!!!\n");

  // READ Chapter 11.7.3 HOW TO chdir

  if (strcmp(pathname, "") == 0)
  {
    strncpy(pathname, "/", 128);
  }

  int ino = getino(pathname);
  if (ino == 0)
  {
    printf("error, ino = 0");
    return -1;
  }

  MINODE *mip = iget(dev, ino);

  //verify mip->INODE is a dir
  if (!S_ISDIR(mip->INODE.i_mode))
  {
    printf("error, mip->INODE is not DIR");
    return -1;
  }

  iput(running->cwd);
  running->cwd = mip;
}

int ls_file(MINODE *mip, char *name)
{
  //printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  INODE *ip;
  char ftime[64], buf[256];
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  ip = &(mip->INODE);
  if (S_ISREG(ip->i_mode))
    printf("%c", '-');
  if (S_ISDIR(ip->i_mode))
    printf("%c", 'd');
  if (S_ISLNK(ip->i_mode))
    printf("%c", 'l');

  for (int i=8; i >= 0; i--)
  {
    if (ip->i_mode & (1 << i)) // print r|w|x
    printf("%c", t1[i]);
    else
    printf("%c", t2[i]); // or print -
  }

  printf("%4d", ip->i_links_count);
  printf("%4d", ip->i_gid);
  printf("%4d\t",ip->i_uid); // uid

  // print time
  strcpy(ftime, ctime((time_t*)&mip->INODE.i_mtime)); // print time in calendar form
  ftime[strlen(ftime)-1] = 0; // kill \n at end
  printf("%4s ",ftime);

  printf("%8d ",ip->i_size); // file size


  // print name
  printf("%10s", basename(name)); // print file basename

  // print -> linkname if symbolic file
  if (S_ISLNK(ip->i_mode))
  {
      printf(" -> %s", (char* )ip->i_block); // print linked name
  }

  //do ls -l and show [dev, ino]
  printf("  [%d %2d]", mip->dev,mip->ino);

  printf("\n");

  return 0;
}

int ls_dir(MINODE *mip)
{
  //printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE)
  {
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;

    mip = iget(dev, dp->inode);
    ls_file(mip, temp);
    iput(mip);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  printf("\n");
  return 0;
}

int ls()
{
  //printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");
  //ls_dir(running->cwd);
  if (strcmp(pathname, "") == 0)
  {
    ls_dir(running->cwd);
    return 0;
  }

  int ino = getino(pathname);
  MINODE *mip = iget(dev, ino);

  int mode = mip->INODE.i_mode;

  if (S_ISDIR(mode))
    {ls_dir(mip);}
  else
    {ls_file(mip, pathname);}

  iput(mip);
}

char rpwd(MINODE *wd)
{
  if (wd == root){
    return 0;
    }

  int my_ino;
  int parent_ino;
  char my_name[256];

  parent_ino = findino(wd, &my_ino);
  printf("my_ino=%d, parent_ino=%d\n", my_ino, parent_ino);

  MINODE *pip = iget(dev, parent_ino);
  findmyname(pip, my_ino, my_name);

  rpwd(pip);

  printf("/%s", my_name);
}

char *mypwd(MINODE *wd)
{
  //printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return 0;
  }
  else{
    rpwd(wd);
  }

  printf("\n");
}
