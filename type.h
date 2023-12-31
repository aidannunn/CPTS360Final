/*************** type.h file for LEVEL-1 ****************/
#ifndef TYPE
#define TYPE
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NMTABLE     8
#define NPROC       2
#define NFD        10
#define NOFT       40

typedef struct minode
{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct Mount *mptr;  // for level-3
}MINODE;

typedef struct oft  //OpenFileTable
{
  int mode;         //R|W|RW|APP
  int refCount;
  MINODE *minodePtr;
  int offset;
}OFT;


typedef struct proc
{
  struct proc *next;
  int          pid;      // process ID
  int          uid;      // user ID
  int          gid;
  MINODE      *cwd;      // CWD directory pointer
  OFT *fd[NFD];
}PROC;

typedef struct Mount{
  int    dev;       // dev (opened vdisk fd number) 0 means FREE
  int    ninodes;   // from superblock
  int    nblocks;
  int    bmap;      // from GD block
  int    imap;
  int    iblk;
  struct minode *mounted_inode;
  char   name[64];  // device name, e.g. mydisk
  char   mount_name[64]; // mounted DIR pathname
} MOUNT;

typedef struct myst
{
  dev_t       myst_dev;     /* device */
  ino_t       myst_ino;     /* inode */
  mode_t      myst_mode;    /* protection */
  nlink_t     myst_nlink;   /* number of hard links */
  uid_t       myst_uid;     /* user ID of owner */
  gid_t       myst_gid;     /* group ID of owner */
  off_t       myst_size;    /* total size, in bytes */
  dev_t       myst_rdev;    /* device type (if inode device) */
  u32         myst_blksize; /* blocksize for filesystem I/O */
  u32         myst_blocks;  /* number of blocks allocated */
  time_t      myst_mtime;   /* time of last modification */
  time_t      myst_atime;   /* time of last access */
  time_t      myst_ctime;   /* time of last change */
}MYST;

#endif
