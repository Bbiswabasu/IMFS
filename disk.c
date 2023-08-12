#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

typedef struct disk
{
    uint32_t size;
    uint32_t blocks;
    uint32_t reads;
    uint32_t writes;
    char **block_arr;
} disk;

disk *create_disk(int nbytes)
{
    disk *d = (disk *)malloc(nbytes);
    d->size = nbytes;
    d->blocks = (nbytes - 24) / BLOCK_SIZE;
    d->reads = 0;
    d->writes = 0;
    return d;
}
int read_block(disk *diskptr, int blocknr, void *block_data)
{
    if (blocknr < 0 || blocknr >= diskptr->blocks)
        return -1;
    memcpy(block_data, diskptr->block_arr[blocknr], BLOCK_SIZE);
    diskptr->reads++;
    return 0;
}
int write_block(disk *diskptr, int blocknr, void *block_data)
{
    if (blocknr < 0 || blocknr >= diskptr->blocks)
        return -1;
    memcpy(diskptr->block_arr[blocknr], block_data, BLOCK_SIZE);
    diskptr->writes++;
    return 0;
}
