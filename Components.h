#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "Config.h"
#include <time.h>

struct File
{
    int inode_id = 0;
    char filename[MAX_FILENAME_SIZE];
};

struct INode
{
    int id = -1;
    int fsize = 0; // in KB
    int fmode = 0; // 1-dentry 0-file
    int count = 0; // if dentry
    int mcount = 0; // history maximum number
    time_t ctime = 0;
    int dir_address[NUM_DIRECT_ADDRESS];
    int indir_address[NUM_INDIRECT_ADDRESS];
};

struct Superblock
{
    int systemsize = 0;
    int blocksize = 0;
    int blocknum = 0;
    int address_length = 0;
    int max_filename_size = 0;
    int superblock_size = 0;
    int inode_size = 0;
    int inode_bitmap_size = 0;
    int block_bitmap_size = 0;
    int inode_table_size = 0;
};


class Address
{
private:
    unsigned char Address_info[3];// 24 bits

public:
    Address();
    ~Address();
    int getblockID();
	void setblockID(int id);
	int getOffset();
	void setOffset(int offset);
    int addressToint();
    void intToaddress(int it, int pid, unsigned char (&tempid)[2]);
};

#endif