#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sfs.h"
#include "dir.h"

int main()
{
    /* initializing the disk and filesystem by formatting and mounting the disk */
    disk *my_disk = create_disk((NUM_BLOCKS + 1) * BLOCK_SIZE);
    printf("Disk created successfully\n");
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

    if (create_file() == 0)
        printf("Root directory created successfully\n");
    else
        exit(1);

    while (1)
    {
        char cmd[20];
        scanf("%s", cmd);
        if (strcmp(cmd, "show_disk_info") == 0)
        {
            show_disk_info(my_disk);
            printf("\n");
        }
        else if (strcmp(cmd, "format_disk") == 0)
        {
            if (format(my_disk) != 0)
            {
                printf("Error while formatting disk for SFS\n");
                exit(1);
            }
            printf("Disk formatted successfully for SFS\n");
        }
        else if (strcmp(cmd, "mount_disk") == 0)
        {
            if (mount(my_disk) != 0)
            {
                printf("Disk mount error\n");
                exit(1);
            }
            printf("Disk mounted successfully\n");
        }
        else if (strcmp(cmd, "create_file") == 0)
        {
            int inumber = create_file();
            if (inumber == -1)
                printf("No free inode\n");
            else
                printf("Inode %d allocated\n", inumber);
        }
        else if (strcmp(cmd, "remove_file") == 0)
        {
            int inumber;
            printf("Enter inode number: ");
            scanf("%d", &inumber);
            if (remove_file(inumber) == -1)
                printf("Failed to remove file\n");
            else
                printf("File removed\n");
        }
        else if (strcmp(cmd, "write_i") == 0)
        {
            int inumber, length, offset;
            printf("Enter inode number: ");
            scanf("%d", &inumber);
            printf("Enter length of data to be written: ");
            scanf("%d", &length);
            printf("Enter offset of data to be written: ");
            scanf("%d", &offset);
            char data[length];
            printf("Enter data:\n");
            scanf("%s", data);
            int bytes_written = write_i(inumber, data, length, offset);
            if (bytes_written == -1)
                printf("Error while writing to file\n");
            else
                printf("%d bytes of data written successfully\n", bytes_written);
        }
        else if (strcmp(cmd, "read_i") == 0)
        {
            int inumber, length, offset;
            printf("Enter inode number: ");
            scanf("%d", &inumber);
            printf("Enter length of data to be read: ");
            scanf("%d", &length);
            printf("Enter offset of data to be read: ");
            scanf("%d", &offset);
            char data[length];
            int bytes_read = read_i(inumber, data, length, offset);
            if (bytes_read == -1)
                printf("Error while reading file\n");
            else
                printf("%d bytes of data read successfully:\n%s\n", bytes_read, data);
        }
        else if (strcmp(cmd, "create_dir") == 0)
        {
            char dirpath[1000];
            printf("Enter directory path: ");
            scanf("%s", dirpath);
            if (create_dir(dirpath) != 0)
                printf("Error creating directory\n");
            else
                printf("Directory created successfully\n");
        }
        else if (strcmp(cmd, "remove_dir") == 0)
        {
            char dirpath[1000];
            printf("Enter directory path: ");
            scanf("%s", dirpath);
            if (remove_dir(dirpath) != 0)
                printf("Error removing directory\n");
            else
                printf("Directory removed successfully\n");
        }
        else if (strcmp(cmd, "write_file") == 0)
        {
            char filepath[1000];
            int length, offset;
            printf("Enter file path: ");
            scanf("%s", filepath);
            printf("Enter length of data to be written: ");
            scanf("%d", &length);
            printf("Enter offset of data to be written: ");
            scanf("%d", &offset);
            char data[length];
            printf("Enter data:\n");
            scanf("%s", data);
            if (write_file(filepath, data, length, offset) != 0)
                printf("Error writing to file\n");
            else
                printf("Written to file successfully\n");
        }
        else if (strcmp(cmd, "read_file") == 0)
        {
            char filepath[1000];
            int length, offset;
            printf("Enter file path: ");
            scanf("%s", filepath);
            printf("Enter length of data to be read: ");
            scanf("%d", &length);
            printf("Enter offset of data to be read: ");
            scanf("%d", &offset);
            char data[length];
            if (read_file(filepath, data, length, offset) != 0)
                printf("Error reading file\n");
            else
            {
                printf("File read successfully\n");
                printf("%s\n", data);
            }
        }
        else if (strcmp(cmd, "get_dir") == 0)
        {
            char dirpath[1000];
            printf("Enter directory path: ");
            scanf("%s", dirpath);
            get_dir(dirpath);
        }
        else if (strcmp(cmd, "exit") == 0)
            exit(0);
        else
            printf("invalid command\n");
        printf("\n");
    }
}