#include <stdint.h>
typedef struct directory
{
    uint32_t valid;
    uint32_t type;
    char name[16];
    uint32_t length;
    uint32_t inumber;
} directory;

int read_file(char *filepath, char *data, int length, int offset);
int write_file(char *filepath, char *data, int length, int offset);
int create_dir(char *dirpath);
int remove_dir(char *dirpath);
int get_dir(char *dirpath);