/* Implementation of Simple File System */

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

    inode new_inode;
    new_inode.valid = 1;
    new_inode.size = 0;
    new_inode.indirect = 0;

    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    if (read_block(my_disk, 0, sb) != 0)
        return -1;

    int inumber = 0;
    for (int blocknr = sb->inode_bitmap_block_idx; blocknr < sb->data_bitmap_block_idx; blocknr++)
    {
        uint8_t bitmap[BLOCK_SIZE];
        if (read_block(my_disk, blocknr, bitmap) != 0)
            return -1;
        for (int offset = 0; offset < BLOCK_SIZE / 8; offset++)
        {
            if (bitmap[offset] == 0xFF)
            {
                inumber += 8;
                continue;
            }
            for (int bit = 7; bit >= 0; bit--)
            {
                if ((bitmap[offset] >> bit) & 1)
                {
                    inumber++;
                    continue;
                }
                /* update inode bitmap */
                bitmap[offset] = bitmap[offset] | (1 << bit);
                if (write_block(my_disk, blocknr, bitmap) != 0)
                    return -1;

                /* store new inode on disk */
                const int NUM_INODES_PER_BLOCK = BLOCK_SIZE / sizeof(inode);
                blocknr = inumber / NUM_INODES_PER_BLOCK;
                offset = inumber % NUM_INODES_PER_BLOCK;
                inode inodes[NUM_INODES_PER_BLOCK];
                inodes[offset] = new_inode;
                if (write_block(my_disk, sb->inode_block_idx + blocknr, inodes))
                    return -1;
                return inumber;
            }
        }
    }
    return -1;
}
int remove_file(int inumber)
{
    /* read super block */
    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    if (read_block(my_disk, 0, sb) != 0)
        return -1;

    /* get bit position in inode bitmap */
    const int NUM_BITS_PER_BLOCK = 8 * BLOCK_SIZE;
    int blocknr = inumber / NUM_BITS_PER_BLOCK;
    int offset = inumber % NUM_BITS_PER_BLOCK / 8;
    int bit = inumber - blocknr * NUM_BITS_PER_BLOCK - offset * 8;

    /* update inode bitmap by setting the bit to 0 */
    uint8_t bitmap[BLOCK_SIZE];
    if (read_block(my_disk, sb->inode_bitmap_block_idx + blocknr, bitmap) != 0)
        return -1;
    bitmap[offset] &= (0xFF ^ (1 << (7 - bit)));
    if (write_block(my_disk, sb->inode_bitmap_block_idx + blocknr, bitmap) != 0)
        return -1;

    /* update inode by setting valid to 0 */
    const int NUM_INODES_PER_BLOCK = BLOCK_SIZE / sizeof(inode);
    blocknr = inumber / NUM_INODES_PER_BLOCK;
    offset = inumber % NUM_INODES_PER_BLOCK;
    inode inodes[NUM_INODES_PER_BLOCK];
    if (read_block(my_disk, sb->inode_block_idx + blocknr, inodes) != 0)
        return -1;
    inodes[offset].valid = 0;
    if (write_block(my_disk, sb->inode_block_idx + blocknr, inodes) != 0)
        return -1;
    return 0;
}

int write_i(int inumber, char *data, int length, int offset)
{
    /* check if inode is allocated and valid */
    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    if (read_block(my_disk, 0, sb) != 0)
        return -1;

    /* get bit position in inode bitmap */
    const int NUM_BITS_PER_BLOCK = 8 * BLOCK_SIZE;
    int inode_bitmap_blocknr = inumber / NUM_BITS_PER_BLOCK;
    int inode_bitmap_offset = inumber % NUM_BITS_PER_BLOCK / 8;
    int bit = inumber - inode_bitmap_blocknr * NUM_BITS_PER_BLOCK - inode_bitmap_offset * 8;

    uint8_t bitmap[BLOCK_SIZE];
    if (read_block(my_disk, sb->inode_bitmap_block_idx + inode_bitmap_blocknr, bitmap) != 0)
        return -1;
    if (!((bitmap[inode_bitmap_offset] >> (7 - bit)) & 1))
    {
        printf("inode doesn't exist\n");
        return -1;
    }

    const int NUM_INODES_PER_BLOCK = BLOCK_SIZE / sizeof(inode);
    int inode_blocknr = inumber / NUM_INODES_PER_BLOCK;
    int inode_offset = inumber % NUM_INODES_PER_BLOCK;
    inode inodes[NUM_INODES_PER_BLOCK];
    if (read_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
        return -1;
    if (inodes[inode_offset].valid == 0)
    {
        printf("inode no longer valid\n");
        return -1;
    }

    /* check whether offset is valid */
    if (inodes[inode_offset].size < offset)
    {
        printf("Invalid offset\n");
        return -1;
    }
    int data_blocknr = offset / BLOCK_SIZE;
    int data_offset = offset % BLOCK_SIZE;
    char data_block[BLOCK_SIZE];
    if (read_block(my_disk, data_blocknr, data_block) != 0)
        return -1;
    for (int i = 0; i < length; i++)
        data_block[i + offset] = data[i];
    data_block[offset + length] = EOF;
    inodes[inode_offset].size = offset + length;
    if (write_block(my_disk, sb->data_block_idx + data_blocknr, data_block))
        return 0;
    if (write_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
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
    inode_index = create_file();
    if (inode_index == -1)
    {
        printf("No free inode\n");
        exit(1);
    }
    printf("Inode %d allocated\n", inode_index);

    if (remove_file(0))
    {
        printf("Failed to remove file\n");
        exit(1);
    }
    printf("File removed\n");
    inode_index = create_file();
    if (inode_index == -1)
    {
        printf("No free inode\n");
        exit(1);
    }
    printf("Inode %d allocated\n", inode_index);

    char data[4];
    if (write_i(1, data, 0, 0) != 0)
    {
        printf("Write error\n");
        exit(1);
    }
    printf("Write success");
}