/* Implementation of Simple File System */
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
int get_free_block(int start_block, int end_block)
{
    int bit_number = 0;
    /* find free block from bitmap */
    for (int inode_bitmap_blocknr = start_block; inode_bitmap_blocknr < end_block; inode_bitmap_blocknr++)
    {
        uint8_t bitmap[BLOCK_SIZE];
        if (read_block(my_disk, inode_bitmap_blocknr, bitmap) != 0)
            return -1;
        for (int inode_bitmap_offset = 0; inode_bitmap_offset < BLOCK_SIZE / 8; inode_bitmap_offset++)
        {
            if (bitmap[inode_bitmap_offset] == 0xFF)
            {
                bit_number += 8;
                continue;
            }
            for (int bit = 7; bit >= 0; bit--)
            {
                if ((bitmap[inode_bitmap_offset] >> bit) & 1)
                    bit_number++;
                else
                {
                    bitmap[inode_bitmap_offset] = bitmap[inode_bitmap_offset] | (1 << bit);
                    if (write_block(my_disk, inode_bitmap_blocknr, bitmap) != 0)
                        return -1;
                    return bit_number;
                }
            }
        }
    }
    return -1;
}
int create_file()
{
    if (state != MOUNTED)
        return -1;

    /* create new inode */
    inode new_inode;
    new_inode.valid = 1;
    new_inode.size = 0;
    new_inode.indirect = -1;
    for (int i = 0; i < 5; i++)
        new_inode.direct[i] = -1;

    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    if (read_block(my_disk, 0, sb) != 0)
        return -1;

    int inumber = get_free_block(sb->inode_bitmap_block_idx, sb->data_bitmap_block_idx);
    if (inumber == -1)
    {
        printf("No free inode\n");
        return -1;
    }

    /* store new inode on disk */
    const int NUM_INODES_PER_BLOCK = BLOCK_SIZE / sizeof(inode);
    int inode_blocknr = inumber / NUM_INODES_PER_BLOCK;
    int inode_offset = inumber % NUM_INODES_PER_BLOCK;
    inode inodes[NUM_INODES_PER_BLOCK];
    if (read_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
        return -1;
    inodes[inode_offset] = new_inode;

    if (write_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
        return -1;
    return inumber;
}
int remove_file(int inumber)
{
    /* read super block */
    super_block *sb = (super_block *)malloc(BLOCK_SIZE);
    if (read_block(my_disk, 0, sb) != 0)
        return -1;

    /* get bit position in inode bitmap */
    const int NUM_BITS_PER_BLOCK = 8 * BLOCK_SIZE;
    int inode_bitmap_blocknr = inumber / NUM_BITS_PER_BLOCK;
    int inode_bitmap_offset = inumber % NUM_BITS_PER_BLOCK / 8;
    int bit = inumber - inode_bitmap_blocknr * NUM_BITS_PER_BLOCK - inode_bitmap_offset * 8;

    /* update inode bitmap by setting the bit to 0 */
    uint8_t bitmap[BLOCK_SIZE];
    if (read_block(my_disk, sb->inode_bitmap_block_idx + inode_bitmap_blocknr, bitmap) != 0)
        return -1;
    bitmap[inode_bitmap_offset] &= (0xFF ^ (1 << (7 - bit)));
    if (write_block(my_disk, sb->inode_bitmap_block_idx + inode_bitmap_blocknr, bitmap) != 0)
        return -1;

    /* update inode by setting valid to 0 */
    const int NUM_INODES_PER_BLOCK = BLOCK_SIZE / sizeof(inode);
    int inode_blocknr = inumber / NUM_INODES_PER_BLOCK;
    int inode_offset = inumber % NUM_INODES_PER_BLOCK;
    inode inodes[NUM_INODES_PER_BLOCK];
    if (read_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
        return -1;
    inodes[inode_offset].valid = 0;
    if (write_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
        return -1;

    /* free data blocks from data block bitmap */
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

    int bytes_written = 0;
    int data_blocknr = offset / BLOCK_SIZE;
    int data_offset = offset % BLOCK_SIZE;
    char data_block[BLOCK_SIZE];
    while (bytes_written < length)
    {
        /* check whether data_blocknr is already allocated on the data block bitmap */
        int data_block_index;
        if (data_blocknr < 5)
        {
            if ((data_block_index = inodes[inode_offset].direct[data_blocknr]) == -1)
            {
                /* allocate new data block and assign it to data_block_index */
                data_block_index = get_free_block(sb->data_bitmap_block_idx, sb->inode_block_idx);
                inodes[inode_offset].direct[data_blocknr] = data_block_index;
                printf("Allocated data block: %d\n", data_block_index);
            }
        }
        else
        {
            /* get indirect pointer */
        }
        if (read_block(my_disk, sb->data_block_idx + data_block_index, data_block) != 0)
            return -1;

        while (bytes_written < length && data_offset != BLOCK_SIZE)
        {
            data_block[data_offset] = data[bytes_written];
            bytes_written++;
            data_offset++;
        }
        if (write_block(my_disk, sb->data_block_idx + data_block_index, data_block) != 0)
            return -1;
        data_offset = 0;
        data_blocknr++;
    }
    inodes[inode_offset].size = offset + bytes_written;
    if (write_block(my_disk, sb->inode_block_idx + inode_blocknr, inodes) != 0)
        return -1;
    return bytes_written;
}
int read_i(int inumber, char *data, int length, int offset)
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

    int bytes_read = 0;
    int data_blocknr = offset / BLOCK_SIZE;
    int data_offset = offset % BLOCK_SIZE;
    char data_block[BLOCK_SIZE];
    while (bytes_read < length)
    {
        /* check whether data_blocknr is already allocated on the data block bitmap */
        int data_block_index;
        if (data_blocknr < 5)
        {
            data_block_index = inodes[inode_offset].direct[data_blocknr];
        }
        else
        {
            /* get indirect pointer */
        }
        if (read_block(my_disk, sb->data_block_idx + data_block_index, data_block) != 0)
            return -1;

        int eof = 0;
        while (bytes_read < length && data_offset != BLOCK_SIZE)
        {
            if (offset + bytes_read >= inodes[inode_offset].size)
            {
                eof = 1;
                break;
            }
            data[bytes_read] = data_block[data_offset];
            bytes_read++;
            data_offset++;
        }
        if (eof)
            break;
        data_offset = 0;
        data_blocknr++;
    }
    return bytes_read;
}
