/*********** mount_umount.c file ****************/
#include "functions.h"

MOUNT *getmptr(int dev)
{
	for (int i = 0; i < NMTABLE; i++) {
		if (mountTable[i].dev == dev)
			return &mountTable[i];
	}
}

int mount()    /*  Usage: mount filesys mount_point OR mount */
{
	int ino, tableValue = -1;
	MINODE *mip;
	char virdisk[256], dpath[256], buf[BLKSIZE];

	// 1. Ask for filesys (a virtual disk) and mount_point (a DIR pathname).
	//    If no parameters: display current mounted filesystems.
	printf("Please enter a virtual disk and DIR pathname\n");
	fgets(line, 128, stdin);
	line[strlen(line)-1] = 0;
	sscanf(line, "%s %s", virdisk, dpath);

	if (strcmp(line, "") == 0)
	{
		printf("Current mounted disks:\n");
		for (int i = 0; i < NMTABLE; i++) {
			if (mountTable[i].dev != 0)
				printf("%s\n", mountTable[i].name);
		}
		printf("\n");
		return 0;
	}
	// 2. Check whether filesys is already mounted:
	//    (you may store mounted filesys name in the MOUNT table entry).
	//    If already mounted, reject;
	//    else: allocate a free MOUNT table entry (dev=0 means FREE).
	for (int i = 0; i < NMTABLE; i++) {
		if (mountTable[i].dev != 0 && mountTable[i].name == virdisk) {
			printf("disk already mounted\n");
			return -1;
		}
	}
	//Finds next free mount table entry
	for (int i = 0; i < NMTABLE; i++) {
		if (mountTable[i].dev == 0) {
			tableValue = i;
			break;
		}
	}
	strcpy(mountTable[tableValue].name, virdisk);
	strcpy(mountTable[tableValue].mount_name, dpath);
	// 3. LINUX open filesys for RW; use its fd number as the new DEV;
	//    Check whether it's an EXT2 file system: if not, reject.
	printf("checking EXT2 FS ....");
	if ((fd = open(virdisk, O_RDWR)) < 0){
		printf("open %s failed\n", virdisk);
		exit(1);
	}

	dev = fd;    // global dev same as this fd

	/********** read super block  ****************/
	get_block(dev, 1, buf);
	sp = (SUPER *)buf;

	/* verify it's an ext2 file system ***********/
	if (sp->s_magic != 0xEF53){
	    printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
	    exit(1);
	}
	printf("EXT2 FS OK\n");

	//4. For mount_point: find its ino, then get its minode:
	ino  = getino(dpath);  // get ino:
	mip  = iget(running->cwd->dev, ino);    // get minode in memory;

	// 5. Verify mount_point is a DIR.  // can only mount on a DIR, not a file
	//    Check mount_point is NOT busy (e.g. can't be someone's CWD)
	if (!S_ISDIR(mip->INODE.i_mode))
	{
		printf("error, mount_point is not DIR");
		return -1;
	}
	if (mip->refCount > 2) {
		printf("error, mount_point is busy");
		return -1;
	}

	//6. Allocate a FREE (dev=0) mountTable[] for newdev;
	//   Record new DEV, ninodes, nblocks, bmap, imap, iblk in mountTable[]
	mountTable[tableValue].dev = dev;
	mountTable[tableValue].ninodes = sp->s_inodes_count;
	mountTable[tableValue].nblocks = sp->s_blocks_count;
	mountTable[tableValue].bmap = gp->bg_block_bitmap;
	mountTable[tableValue].imap = gp->bg_inode_bitmap;
	mountTable[tableValue].iblk = gp->bg_inode_table;

	// 7. Mark mount_point's minode as being mounted on and let it point at the
	//    MOUNT table entry, which points back to the mount_point minode.
	mip->mounted = 1;
	mip->mptr = &mountTable[tableValue];
	mountTable[tableValue].mounted_inode = mip;
	//	return 0 for SUCCESS;
	return 0;
}

int umount(char *filesys)
{
	int tableValue = -1;
	MINODE* mip;
//1. Search the MOUNT table to check filesys is indeed mounted.
	for (int i = 0; i < NMTABLE; i++) {
		if (strcmp(filesys, mountTable[i].name) == 0)
			tableValue = i;
	}
	if (tableValue == -1) {
		printf("%s is not mounted\n", filesys);
		return -1;
	}

// 2. Check whether any file is still active in the mounted filesys;
//       e.g. someone's CWD or opened files are still there,
//    if so, the mounted filesys is BUSY ==> cannot be umounted yet.
//    HOW to check?      ANS: by checking all minode[].dev with refCount>0
	for (int i = 0; i < NMINODE; i++) {
			if (minode[i].dev == mountTable[tableValue].dev && minode[i].refCount > 0) {
				printf("%s is busy, cannot be unmounted\n", filesys);
				return -1;
			}
	}

// 3. Find the mount_point's inode (which should be in memory while it's mounted on).
//    Reset it to "not mounted"; then
//    iput() the minode.  (because it was iget()ed during mounting)
	mip = mountTable[tableValue].mounted_inode;
	mip->mounted = 0;
	iput(mip);

// 4. return 0 for SUCCESS;
	return 0;
}
