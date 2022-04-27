#ifndef FUNCTIONS
#define FUNCTIONS

// C Libs:
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>


// USR
#include "type.h"
#include "globals.h"


/* util.c */
int get_block(int dev, int blk, char *buf);

int put_block(int dev, int blk, char *buf);

int tokenize(char *pathname);

MINODE *iget(int dev, int ino);

void iput(MINODE *mip);

int search(MINODE *mip, char *name);

int getino(char *pathname);

int findmyname(MINODE *parent, u32 myino, char myname[]);

int findino(MINODE *mip, u32 *myino);

/* misc.c */

int myStat(char* filename);

int myChmod(char* filename, int mode);

int myUtime(char* filename);

/* alloc.c */

int tst_bit(char *buf, int bit);

int set_bit(char *buf, int bit);

int clr_bit(char* buf, int bit);

int decFreeInodes(int dev);

int incFreeInodes(int dev);

int idalloc(int dev, int ino);

int bdalloc(int dev, int bno);

int ialloc(int dev);

int balloc(int dev);

/* cd_ls_pwd.c */
int cd();

int ls_file(MINODE *mip, char *name);

int ls_dir(MINODE *mip);

int ls();

char *mypwd(MINODE *wd);

/* mkdir_creat_rmdir.c */
int mymkdir();

int kmkdir(MINODE *pmip, char* bname);

int enter_name(MINODE *pip, int ino, char* name);

int rm_child(MINODE *pmip, char *name);

int myrmdir();

int mycreat();

/* link_unlink */

int myLink(char* old_file, char* new_file);

int myUnlink();

int mySymlink(char* old_file, char* new_file);

int myReadlink(MINODE* mip, char* buf);

/* open_close_lseek */
int open_file(int mode);

int mytruncate(MINODE *mip);

int close_file();

int mylseek(int fd, int position);

int pfd();

int dup(int fd);

int dup2(int fd, int gd);

/* read_cat */

int myread(int fd, char *buf, int nbytes);

int read_file();

int myCat(char* filename);

/* write_cp */

int mywrite(int fd, char *buf, int nbytes);

int write_file();

int myCP(char* src, char* dest);

int myMV(char* src, char* dest);

/* access */
int myaccess(char *filename, char mode);

#endif
