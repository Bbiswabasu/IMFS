#define BLOCK_SIZE 4096

typedef struct disk
{
    uint32_t size;
    uint32_t blocks;
    uint32_t reads;
    uint32_t writes;
    char **block_arr;
} disk;

disk *create_disk(int nbytes);
int read_block(disk *diskptr, int blocknr, void *block_data);
int write_block(disk *diskptr, int blocknr, void *block_data);
void show_disk_info(disk *diskptr);