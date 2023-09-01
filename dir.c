#include "sfs.h"
#include "dir.h"

int subdirectory_exists(int *parent_inumber, char *subdirname, int type)
{
    int MAX_SUBDIRECTORY = BLOCK_SIZE / sizeof(directory);
    directory parent_dir_data[MAX_SUBDIRECTORY];

    int bytes_read = read_i(*parent_inumber, parent_dir_data, sizeof(parent_dir_data), 0);

    for (int i = 0; i < bytes_read / sizeof(directory); i++)
    {
        if (parent_dir_data[i].valid && parent_dir_data[i].type == type && strcmp(parent_dir_data[i].name, subdirname) == 0)
        {
            *parent_inumber = parent_dir_data[i].inumber;
            return 1;
        }
    }
    return 0;
}
int get_parent_inumber(char *dirpath, char *current_directory)
{
    int parent_inumber = 0;
    int ptr = 0;

    for (int i = 1; i < strlen(dirpath); i++)
    {
        if (dirpath[i] == '/')
        {
            current_directory[ptr] = '\0';
            if (subdirectory_exists(&parent_inumber, current_directory, 0) == 0)
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
    current_directory[ptr] = '\0';
    return parent_inumber;
}
int read_file(char *filepath, char *data, int length, int offset)
{
    if (strcmp(filepath, "/") == 0)
    {
        printf("Root directory cannot be file\n");
        return -1;
    }
    char current_directory[16];
    int parent_inumber = get_parent_inumber(filepath, current_directory);
    if (parent_inumber == -1)
        return -1;
    if (!subdirectory_exists(&parent_inumber, current_directory, 1))
    {
        printf("File does not exist\n");
        return -1;
    }
    int inumber = parent_inumber;
    if (read_i(inumber, data, length, offset) == -1)
        return -1;
    return 0;
}
int write_file(char *filepath, char *data, int length, int offset)
{
    if (strcmp(filepath, "/") == 0)
    {
        printf("Root directory cannot be file\n");
        return -1;
    }
    char current_directory[16];
    int parent_inumber = get_parent_inumber(filepath, current_directory);
    if (parent_inumber == -1)
        return -1;
    int inumber = -1;
    if (!subdirectory_exists(&parent_inumber, current_directory, 1))
    {
        int MAX_SUBDIRECTORY = BLOCK_SIZE / sizeof(directory);
        directory parent_dir_data[MAX_SUBDIRECTORY];
        int bytes_read = read_i(parent_inumber, parent_dir_data, sizeof(parent_dir_data), 0);
        if (bytes_read == -1)
            return -1;

        inumber = create_file();
        if (inumber == -1)
            return -1;
        directory new_subdirectory;
        new_subdirectory.valid = 1;
        new_subdirectory.inumber = inumber;
        strcpy(new_subdirectory.name, current_directory);
        new_subdirectory.length = strlen(current_directory);
        new_subdirectory.type = 1;

        int bytes_written = write_i(parent_inumber, &new_subdirectory, sizeof(directory), bytes_read);
        if (bytes_written == -1)
            return -1;
    }
    else
    {
        inumber = parent_inumber;
    }
    write_i(inumber, data, length, offset);
    return 0;
}
int create_dir(char *dirpath)
{
    if (strcmp(dirpath, "/") == 0)
    {
        printf("Root directory already exists\n");
        return -1;
    }

    char current_directory[16];
    int parent_inumber = get_parent_inumber(dirpath, current_directory);
    if (parent_inumber == -1)
        return -1;

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
    if (strcmp(dirpath, "/") == 0)
    {
        printf("Cannot remove root\n");
        return -1;
    }
    char current_directory[16];
    int parent_inumber = get_parent_inumber(dirpath, current_directory);
    if (parent_inumber == -1)
        return -1;

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
    for (int i = 0; i < bytes_read / sizeof(directory); i++)
    {
        if (parent_dir_data[i].valid && parent_dir_data[i].type == 0 && strcmp(parent_dir_data[i].name, current_directory) == 0)
        {
            directory *removing_directory = &parent_dir_data[i];
            removing_directory->valid = 0;
            int bytes_written = write_i(parent_inumber, removing_directory, sizeof(directory), i * sizeof(directory));
            if (bytes_written == -1)
                return -1;
            return 0;
        }
    }

    return -1;
}
int get_dir(char *dirpath)
{
    if (strcmp(dirpath, "/"))
        strcat(dirpath, "/");

    char current_directory[16];
    int parent_inumber = get_parent_inumber(dirpath, current_directory);
    if (parent_inumber == -1)
        return -1;

    int MAX_SUBDIRECTORY = BLOCK_SIZE / sizeof(directory);
    directory parent_dir_data[MAX_SUBDIRECTORY];

    int bytes_read = read_i(parent_inumber, parent_dir_data, sizeof(parent_dir_data), 0);
    for (int i = 0; i < bytes_read / sizeof(directory); i++)
    {
        if (parent_dir_data[i].valid)
            printf("%s (%s)\n", parent_dir_data[i].name, parent_dir_data[i].type ? "FILE" : "DIR");
    }
    return 0;
}