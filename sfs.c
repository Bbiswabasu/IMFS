#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "disk.h"
#include "sfs.h"

int format(disk *diskptr)
{
    const int NUM_USABLE_BLOCKS = NUM_BLOCKS - 1;        // one block reserved for super_block
    const int NUM_INODE_BLOCKS = NUM_USABLE_BLOCKS / 10; // 10% of usable blocks are inode blocks
    const int NUM_INODES = NUM_INODE_BLOCKS * BLOCK_SIZE / sizeof(inode);
    const int NUM_INODE_BITMAP_BLOCKS = ceil(NUM_INODES * 1.0 / (8 * BLOCK_SIZE));
    const int NUM_REMAINING_BLOCKS = NUM_USABLE_BLOCKS - NUM_INODE_BITMAP_BLOCKS - NUM_INODE_BLOCKS; // to be used as data blocks and data bitmap blocks
    const int NUM_DATA_BITMAP_BLOCKS = ceil(NUM_REMAINING_BLOCKS * 1.0 / (8 * BLOCK_SIZE));
    const int NUM_DATA_BLOCKS = NUM_REMAINING_BLOCKS - NUM_DATA_BITMAP_BLOCKS;

    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    memset(sb, 0, BLOCK_SIZE);
    sb->magic_number = MAGIC_NUMBER;
    sb->blocks = NUM_BLOCKS;
    sb->inode_blocks = NUM_INODE_BLOCKS;
    sb->inodes = NUM_INODES;
    sb->inode_bitmap_block_idx = 1;
    sb->data_bitmap_block_idx = 1 + NUM_INODE_BITMAP_BLOCKS;
    sb->inode_block_idx = sb->data_bitmap_block_idx + NUM_DATA_BITMAP_BLOCKS;
    sb->data_block_idx = 1 + sb->inode_block_idx + NUM_INODE_BLOCKS;
    sb->data_blocks = NUM_DATA_BLOCKS;

    state = 0;
    int status = write_block(diskptr, 0, sb);
    free(sb);
    return status;
}
int mount(disk *diskptr)
{
    super_block *sb = (super_block *)malloc(BLOCK_SIZE); // buffer for storing super_block as read from disk
    if (read_block(diskptr, 0, sb) != 0)
        return -1;
    if (sb->magic_number == MAGIC_NUMBER)
    {
        free(sb);
        my_disk = diskptr;
        state = MOUNTED;
        return 0;
    }
    else
    {
        free(sb);
        return -1;
    }
}
int create_file()
{
    if (state != MOUNTED)
        return -1;
    inode *new_inode = (inode *)malloc(sizeof(inode));
    new_inode->valid = 1;
    new_inode->size = 0;
    new_inode->indirect = 0;

    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    if (read_block(my_disk, 0, sb) != 0)
        return -1;

    int inode_index = 0;
    for (int block = sb->inode_bitmap_block_idx; block < sb->data_bitmap_block_idx; block++)
    {
        uint8_t bitmap[BLOCK_SIZE];
        if (read_block(my_disk, block, bitmap) != 0)
            return -1;
        for (int chunk = 0; chunk < BLOCK_SIZE / 8; chunk++)
        {
            if (bitmap[chunk] == 0xFF)
            {
                inode_index += 8;
                continue;
            }
            for (int bit = 7; bit >= 0; bit--)
            {
                if ((bitmap[chunk] >> bit) & 1)
                {
                    inode_index++;
                    continue;
                }
                /* update inode bitmap */
                bitmap[chunk] = bitmap[chunk] | (1 << bit);
                write_block(my_disk, block, bitmap);

                /* store new inode on disk */
                write_block(my_disk, sb->inode_block_idx + inode_index, new_inode);
                return inode_index;
            }
        }
    }
    return -1;
}
int main()
{
    disk *my_disk = create_disk((NUM_BLOCKS + 1) * BLOCK_SIZE);
    printf("Disk created successfully\n");
    show_disk_info(my_disk);

    if (format(my_disk) != 0)
    {
        printf("Error while formatting disk for SFS\n");
        exit(1);
    }
    printf("Disk formatted successfully for SFS\n");

    if (mount(my_disk) != 0)
    {
        printf("Disk mount error\n");
        exit(1);
    }
    printf("Disk mounted successfully\n");
    int inode_index = create_file();
    if (inode_index == -1)
    {
        printf("No free inode\n");
        exit(1);
    }
    printf("Inode %d allocated\n", inode_index);
    // inode_index = create_file();
    // if (inode_index == -1)
    // {
    //     printf("No free inode\n");
    //     exit(1);
    // }
    // printf("Inode %d allocated\n", inode_index);
}