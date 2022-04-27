#include "functions.h"

//access() return 1 if permission OK, 0 if not
int access(char *filename, char mode)
{
    int r; 
    char ownerBits, otherBits;

    if (running->uid == 0)   // SUPERuser always OK
        return 1;

    // NOT SUPERuser: get file's INODE
    int ino = getino(filename);
    MINODE* mip = iget(dev, ino);

    

    

    //get perm_bits by getting file's permissions mip->inode.i_mode is filetype nad permission bits. we just want lower 9, so & by lower 9 bits (bit masking)
    ownerBits = otherBits = mip->INODE.i_mode & 511;
    
    if (mip->INODE.i_uid == running->uid) //Check if INODE.i_uid is equal to current user's
    //owner:
    //bit shifting for owner. move their upper three bits to be lower three bits
    //shift bits right
    //perm_bits = perm_bits >> 6;
        r = tst_bit(ownerBits, mode);           //(check owner's rwx with mode); by tst_bit()
    else
    //other:
    //bit masking. binary & to "mask" bits we don't want to see
    //perm_bits = perm_bits & binary three 1s 111 = perm_bits & 7;
        r = tst_bit(otherBits, mode);           //(check other's rwx with mode); by tst_bit()

    iput(mip);
  
    return r;
}