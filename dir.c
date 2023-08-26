#include "sfs.h"
#include "dir.h"
int read_file(char *filepath, char *data, int length, int offset)
{
}
int write_file(char *filepath, char *data, int length, int offset)
{
}
int subdirectory_exists(int *parent_inumber, char *subdirname)
{
    int MAX_SUBDIRECTORY = BLOCK_SIZE / sizeof(directory);
    directory parent_dir_data[MAX_SUBDIRECTORY];

    int bytes_read = read_i(parent_inumber, parent_dir_data, sizeof(parent_dir_data), 0);

    for (int i = 0; i < bytes_read / sizeof(directory); i++)
    {
        if (parent_dir_data[i].valid && parent_dir_data[i].type == 0 && strcmp(parent_dir_data[i].name, subdirname) == 0)
        {
            *parent_inumber = parent_dir_data[i].inumber;
            return 1;
        }
    }
    return 0;
}
int create_dir(char *dirpath)
{
    if (strcmp(dirpath, "/") == 0)
    {
        printf("Root directory already exists\n");
        return -1;
    }
    int parent_inumber = 0;
    int ptr = 0;
    char current_directory[16];
    for (int i = 1; i < strlen(dirpath); i++)
    {
        if (dirpath[i] == '/')
        {
            current_directory[ptr] = '\0';
            if (!subdirectory_exists(&parent_inumber, current_directory))
            {
                printf("%s does not exist\n", current_directory);
                return -1;
            }
            ptr = 0;
        }
        else
        {
            current_directory[ptr] = dirpath[i];
            ptr++;
        }
    }

    int MAX_SUBDIRECTORY = BLOCK_SIZE / sizeof(directory);
    directory parent_dir_data[MAX_SUBDIRECTORY];

    int bytes_read = read_i(parent_inumber, parent_dir_data, sizeof(parent_dir_data), 0);
    if (bytes_read == -1)
        return -1;

    /* create new directory file */
    int inumber = create_file();
    if (inumber == -1)
        return -1;

    /* add new directory to the parent directory file data */
    directory new_subdirectory;
    current_directory[ptr] = '\0';
    new_subdirectory.valid = 1;
    new_subdirectory.inumber = inumber;
    strcpy(new_subdirectory.name, current_directory);
    new_subdirectory.length = strlen(current_directory);
    new_subdirectory.type = 0;

    int bytes_written = write_i(parent_inumber, &new_subdirectory, sizeof(directory), bytes_read);
    if (bytes_written == -1)
        return -1;
    return 0;
}
int remove_dir(char *dirpath)
{
}
int get_dir(char *dirpath)
{
    int parent_inumber = 0;
    int ptr = 0;
    char current_directory[16];
    for (int i = 1; i < strlen(dirpath); i++)
    {
        if (dirpath[i] == '/')
        {
            current_directory[ptr] = '\0';
            if (!subdirectory_exists(&parent_inumber, current_directory))
            {
                printf("%s does not exist\n", current_directory);
                return -1;
            }
            ptr = 0;
        }
        else
        {
            current_directory[ptr] = dirpath[i];
            ptr++;
        }
    }
    int MAX_SUBDIRECTORY = BLOCK_SIZE / sizeof(directory);
    directory parent_dir_data[MAX_SUBDIRECTORY];

    int bytes_read = read_i(parent_inumber, parent_dir_data, sizeof(parent_dir_data), 0);

    for (int i = 0; i < bytes_read / sizeof(directory); i++)
    {
        if (parent_dir_data[i].valid)
            printf("%s\n", parent_dir_data[i].name);
    }
}