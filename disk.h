#define BLOCK_SIZE 4096

typedef struct disk disk;
disk *create_disk(int nbytes);
int read_block(disk *diskptr, int blocknr, void *block_data);
int write_block(disk *diskptr, int blocknr, void *block_data);
