#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int inputFile, outputFile;

    // Open input file
    inputFile = open("input.txt", O_RDONLY);

    if (inputFile == -1)
    {
        perror("Error opening input.txt");
        return 1;
    }

    // Open output file
    outputFile = open("output.txt",
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);

    if (outputFile == -1)
    {
        perror("Error opening output.txt");
        close(inputFile);
        return 1;
    }

    // Redirect standard input to input.txt
    if (dup2(inputFile, STDIN_FILENO) == -1)
    {
        perror("dup2 input");
        return 1;
    }

    // Redirect standard output to output.txt
    if (dup2(outputFile, STDOUT_FILENO) == -1)
    {
        perror("dup2 output");
        return 1;
    }

    close(inputFile);
    close(outputFile);

    // This output goes into output.txt
    printf("This text is redirected to output.txt\n");

    printf("Standard input and output were successfully redirected.\n");

    return 0;
}
