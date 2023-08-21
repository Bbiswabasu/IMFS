#include <stdint.h>

#define MAGIC_NUMBER 12345
#define NUM_BLOCKS 1024
#define MOUNTED 1

typedef struct super_block
{
    uint32_t magic_number;
    uint32_t blocks;
    uint32_t inode_blocks;
    uint32_t inodes;
    uint32_t inode_bitmap_block_idx;
    uint32_t inode_block_idx;
    uint32_t data_bitmap_block_idx;
    uint32_t data_block_idx;
    uint32_t data_blocks;
} super_block;

typedef struct inode
{
    uint32_t valid;
    uint32_t size;
    uint32_t direct[5];
    uint32_t indirect;
} inode;

disk *my_disk;
int state;
int format(disk *diskptr);
int mount(disk *diskptr);
int create_file();
int remove_file(int inumber);
int write_i(int inumber, char *data, int length, int offset);
int read_i(int inumber, char *data, int length, int offset);