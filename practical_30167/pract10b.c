#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

int main()
{
    int fd;
    struct stat fileInfo;
    char *mappedFile;

    // Open file
    fd = open("mmap.txt", O_RDWR);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    // Get file information
    if (fstat(fd, &fileInfo) == -1)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (fileInfo.st_size == 0)
    {
        printf("File is empty.\n");
        close(fd);
        return 1;
    }

    // Map file into memory
    mappedFile = mmap(NULL,
                      fileInfo.st_size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      fd,
                      0);

    if (mappedFile == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    printf("Original file content:\n");
    printf("-----------------------\n");
    write(STDOUT_FILENO, mappedFile, fileInfo.st_size);

    printf("\n\nModifying file using mmap...\n");

    // Modify the mapped memory
    if (fileInfo.st_size >= 5)
    {
        memcpy(mappedFile, "HELLO", 5);
    }

    // Synchronize changes with file
    if (msync(mappedFile,
              fileInfo.st_size,
              MS_SYNC) == -1)
    {
        perror("msync");
    }

    printf("File modified successfully.\n");

    // Unmap memory
    munmap(mappedFile, fileInfo.st_size);

    close(fd);

    return 0;
}
