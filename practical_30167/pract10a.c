#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main()
{
    struct stat fileInfo;

    printf("INODE AND LINK DEMONSTRATION\n");
    printf("============================\n\n");

    // Get information about original file
    if (stat("original.txt", &fileInfo) == -1)
    {
        perror("stat");
        return 1;
    }

    printf("Original file information:\n");
    printf("Inode number : %lu\n",
           (unsigned long)fileInfo.st_ino);

    printf("Number of links : %lu\n\n",
           (unsigned long)fileInfo.st_nlink);

    // Create hard link
    if (link("original.txt", "hardlink.txt") == -1)
    {
        perror("Hard link creation failed");
    }
    else
    {
        printf("Hard link created successfully.\n");
    }

    // Create symbolic link
    if (symlink("original.txt", "symlink.txt") == -1)
    {
        perror("Symbolic link creation failed");
    }
    else
    {
        printf("Symbolic link created successfully.\n");
    }

    // Display original file information again
    if (stat("original.txt", &fileInfo) == -1)
    {
        perror("stat");
        return 1;
    }

    printf("\nAfter creating hard link:\n");

    printf("Original inode : %lu\n",
           (unsigned long)fileInfo.st_ino);

    printf("Number of hard links : %lu\n",
           (unsigned long)fileInfo.st_nlink);

    printf("\nUse these commands to investigate:\n");
    printf("ls -i original.txt hardlink.txt symlink.txt\n");
    printf("stat original.txt\n");
    printf("find . -inum <inode_number>\n");

    return 0;
}
