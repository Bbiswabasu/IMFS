/* Implementation of emulated disk interface */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

disk *create_disk(int nbytes)
{
    disk *d = (disk *)malloc(nbytes);
    d->size = nbytes;
    d->blocks = (nbytes - 24) / BLOCK_SIZE;
    d->reads = 0;
    d->writes = 0;
    d->block_arr = (char **)((uint64_t)d + BLOCK_SIZE);
    return d;
}
int read_block(disk *diskptr, int blocknr, void *block_data)
{
    if (blocknr < 0 || blocknr >= diskptr->blocks)
        return -1;
    memcpy(block_data, diskptr->block_arr + BLOCK_SIZE / sizeof(char *) * blocknr, BLOCK_SIZE);
    diskptr->reads++;
    return 0;
}
int write_block(disk *diskptr, int blocknr, void *block_data)
{
    if (blocknr < 0 || blocknr >= diskptr->blocks)
        return -1;
    memcpy(diskptr->block_arr + BLOCK_SIZE / sizeof(char *) * blocknr, block_data, BLOCK_SIZE);
    diskptr->writes++;
    return 0;
}
int free_disk(disk *diskptr)
{
    free(diskptr);
    return 0;
}
void show_disk_info(disk *diskptr)
{
    printf("size: %d\n", diskptr->size);
    printf("blocks: %d\n", diskptr->blocks);
    printf("reads: %d\n", diskptr->reads);
    printf("writes: %d\n", diskptr->writes);
    printf("**block_arr: %d\n", diskptr->block_arr);
}