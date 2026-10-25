#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 4096

double get_time()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void low_level_copy(const char *sourceFile, const char *destFile)
{
    int source, destination;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead, bytesWritten;

    source = open(sourceFile, O_RDONLY);

    if (source == -1)
    {
        perror("open source");
        exit(1);
    }

    destination = open(destFile,
                       O_WRONLY | O_CREAT | O_TRUNC,
                       0644);

    if (destination == -1)
    {
        perror("open destination");
        close(source);
        exit(1);
    }

    while ((bytesRead = read(source, buffer, BUFFER_SIZE)) > 0)
    {
        bytesWritten = write(destination, buffer, bytesRead);

        if (bytesWritten != bytesRead)
        {
            perror("write");
            break;
        }
    }

    close(source);
    close(destination);
}

void standard_copy(const char *sourceFile, const char *destFile)
{
    FILE *source, *destination;
    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    source = fopen(sourceFile, "rb");

    if (source == NULL)
    {
        perror("fopen source");
        exit(1);
    }

    destination = fopen(destFile, "wb");

    if (destination == NULL)
    {
        perror("fopen destination");
        fclose(source);
        exit(1);
    }

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, source)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destination);
    }

    fclose(source);
    fclose(destination);
}

int main()
{
    double start, end;

    printf("Performance Comparison\n");
    printf("======================\n\n");

    // Low-level I/O
    start = get_time();

    low_level_copy("source.txt", "lowlevel.txt");

    end = get_time();

    printf("Low-level read/write time : %.6f seconds\n",
           end - start);

    // Standard library I/O
    start = get_time();

    standard_copy("source.txt", "standard.txt");

    end = get_time();

    printf("Standard fread/fwrite time : %.6f seconds\n",
           end - start);

    return 0;
}
