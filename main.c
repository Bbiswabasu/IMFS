#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "sfs.h"
#include "string.h"

int main()
{
    disk *my_disk = create_disk((NUM_BLOCKS + 1) * BLOCK_SIZE);
    printf("Disk created successfully\n");
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
                printf("Error while formatting disk for SFS\n\n");
                exit(1);
            }
            printf("Disk formatted successfully for SFS\n\n");
        }
        else if (strcmp(cmd, "mount_disk") == 0)
        {
            if (mount(my_disk) != 0)
            {
                printf("Disk mount error\n\n");
                exit(1);
            }
            printf("Disk mounted successfully\n\n");
        }
        else if (strcmp(cmd, "create_file") == 0)
        {
            int inumber = create_file();
            if (inumber == -1)
            {
                printf("No free inode\n\n");
            }
            printf("Inode %d allocated\n\n", inumber);
        }
        else if (strcmp(cmd, "remove_file") == 0)
        {
            int inumber;
            printf("Enter inode number: ");
            scanf("%d", &inumber);
            if (remove_file(inumber) == -1)
            {
                printf("Failed to remove file\n\n");
            }
            printf("File removed\n\n");
        }
        else if (strcmp(cmd, "write_file") == 0)
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
            {
                printf("Error while writing to file\n\n");
            }
            printf("%d bytes of data written successfully\n\n", bytes_written);
        }
        else if (strcmp(cmd, "read_file") == 0)
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
            {
                printf("Error while reading to file\n\n");
            }
            printf("%d bytes of data read successfully:\n%s\n\n", bytes_read, data);
        }
        else if (strcmp(cmd, "exit") == 0)
            exit(0);
        else
            printf("invalid command\n\n");
    }
}