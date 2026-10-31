#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[])
{
    int source, destination;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead, bytesWritten;

    if (argc != 3)
    {
        printf("Usage: %s <source_file> <destination_file>\n", argv[0]);
        return 1;
    }

    // Open source file for reading
    source = open(argv[1], O_RDONLY);

    if (source == -1)
    {
        perror("Error opening source file");
        return 1;
    }

    // Open/create destination file
    destination = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (destination == -1)
    {
        perror("Error opening destination file");
        close(source);
        return 1;
    }

    // Copy contents
    while ((bytesRead = read(source, buffer, BUFFER_SIZE)) > 0)
    {
        bytesWritten = write(destination, buffer, bytesRead);

        if (bytesWritten != bytesRead)
        {
            perror("Error writing to destination");
            close(source);
            close(destination);
            return 1;
        }
    }

    if (bytesRead == -1)
    {
        perror("Error reading source file");
    }

    // Demonstrate lseek()
    if (lseek(source, 0, SEEK_SET) == -1)
    {
        perror("lseek failed");
    }

    close(source);
    close(destination);

    printf("File copied successfully.\n");

    return 0;
}
