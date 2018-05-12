// audio_file_compare.cpp : Defines the entry point for the console application.

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

// Things to help with parsing filenames.
#define DIR_SEPARATORS "\\/"
#define EXT_SEPARATOR "."

// Print the usage text
static void printUsage(char *pExeName) {
    printf("\n%s: take two mono binary audio files containing PCM audio data of a given word-width/endianness and\n", pExeName);
    printf("produce a single stereo PCM audio data file so that the two mono files can be compared side by side in an application\n");
    printf("such as Audacity.  Usage:\n");
    printf("  %s input_file_left endianness word_width input_file_right endianness word_width output_file\n", pExeName);
    printf("where:\n");
    printf("  input_file_left is the name of the PCM input file that will form the left channel,\n");
    printf("  input_file_right is the name of the PCM input file that will form the right channel,\n");
    printf("  endianness is the endianness of the given file, b for big, l for little,\n");
    printf("  word_width is the number of bytes per word for the given file, 1 to 4,\n");
    printf("  output_file is the filename to use for the stereo output (any existing file will be overwritten),\n");
    printf("For example:\n");
    printf("  %s afile l 4 anotherfile b 2 out\n\n", pExeName);
}

// Convert endianness of array
static void reverseEndian(unsigned char *pArray, int width)
{
    char c;

    for (int x = 0; x < width; x++ ) {
        c = *(pArray + x);
        *(pArray + width - x - 1) = c;
    }
}

// Read a word from a file
static int readWord (FILE *pInputFile, int wordWidth, bool isLittleEndian)
{
    unsigned int unsignedNumber = 0;
    unsigned char wordArray[4]; // Must be unsigned or the casts can produce negative numbers

     if (fread(wordArray, wordWidth, 1, pInputFile) == 1) {
        if (!isLittleEndian) {
            reverseEndian(wordArray, wordWidth);
        }
        // Calculate the number
        unsignedNumber = 0;
        for (int x = 0; x < wordWidth; x++) {
            unsignedNumber += ((unsigned int) (wordArray[x])) << (x << 3);
        }
        // Deal with sign
        if (unsignedNumber & (1 << ((wordWidth << 3) - 1))) {
            // Sign extend
            for (int x = (wordWidth << 3); x < sizeof(wordArray) << 3; x++) {
                unsignedNumber |= (1 << x);
            }
        }
     }

     return (int) unsignedNumber;
}

// Parse the input files and write to the output file
static int parse(FILE *pInputFileLeft, int wordWidthLeft, bool isLittleEndianLeft,
                 FILE *pInputFileRight, int wordWidthRight, bool isLittleEndianRight,
                 FILE *pOutputFile)
{
    int numberLeft;
    int numberRight;
    bool finished = false;
    int itemsWritten = 0;

    // Read a word at a time from both files until both are empty
    while (!finished) {
        numberLeft = readWord(pInputFileLeft, wordWidthLeft, isLittleEndianLeft);
        numberRight = readWord(pInputFileRight, wordWidthRight, isLittleEndianRight);
        if (feof(pInputFileLeft) && feof(pInputFileRight)) {
            finished = true;
        } else {
            fwrite((const void *) &numberLeft, sizeof(numberLeft), 1, pOutputFile);
            fwrite((const void *) &numberRight, sizeof(numberRight), 1, pOutputFile);
            itemsWritten++;
        }
    }

    return itemsWritten;
}

// Entry point
int main(int argc, char* argv[])
{
    int retValue = -1;
    bool success = false;
    int x = 0;
    char *pExeName = NULL;
	char *pInputFileNameLeft = NULL;
    FILE *pInputFileLeft = NULL;
    char *pEndiannessLeft = NULL;
	int wordWidthLeft = 0;
	char *pInputFileNameRight = NULL;
    FILE *pInputFileRight = NULL;
    char *pEndiannessRight = NULL;
	int wordWidthRight = 0;
	char *pOutputFileName = NULL;
    FILE *pOutputFile = NULL;
    char *pChar;
    struct stat st = { 0 };

    // Find the exe name in the first argument
    pChar = strtok(argv[x], DIR_SEPARATORS);
    while (pChar != NULL) {
        pExeName = pChar;
        pChar = strtok(NULL, DIR_SEPARATORS);
    }
    if (pExeName != NULL) {
        // Remove the extension
        pChar = strtok(pExeName, EXT_SEPARATOR);
        if (pChar != NULL) {
            pExeName = pChar;
        }
    }
    x++;

    // Look for all the command line parameters
    while (x < argc) {
        switch (x) {
            case 1:
                pInputFileNameLeft = argv[x];
            break;
            case 2:
                pEndiannessLeft = argv[x];
            break;
            case 3:
                wordWidthLeft = atoi(argv[x]);
            break;
            case 4:
                pInputFileNameRight = argv[x];
            break;
            case 5:
                pEndiannessRight = argv[x];
            break;
            case 6:
                wordWidthRight = atoi(argv[x]);
            break;
            case 7:
                pOutputFileName = argv[x];
            break;
        }
        x++;
    }

    // Must have all of the command-line parameters
    if ((pInputFileNameLeft != NULL) &&
        (pEndiannessLeft != NULL) &&
        (wordWidthLeft > 0) &&
        (pInputFileNameRight != NULL) &&
        (pEndiannessRight != NULL) &&
        (wordWidthRight > 0) &&
        (pOutputFileName != NULL)) {
        success = true;
        // Check that endianness makes sense
        if (((strlen (pEndiannessLeft) != 1) ||
             ((*pEndiannessLeft != 'l') &&
              (*pEndiannessLeft != 'b'))) &&
              ((strlen (pEndiannessRight) != 1) ||
             ((*pEndiannessRight != 'l') &&
              (*pEndiannessRight != 'b')))) {
            success = false;
            printf("Endianness must be l for little or b for big.\n");
        }
        // Check that the word width makes sense
        if ((wordWidthLeft > 4) && (wordWidthRight > 4)) {
            success = false;
            printf("Word width must be 1, 2, 3, or 4.\n");
        }
        // Open the left input file
        pInputFileLeft = fopen (pInputFileNameLeft, "rb");
        if (pInputFileLeft == NULL) {
            success = false;
            printf("Cannot open left channel input file %s (%s).\n", pInputFileNameLeft, strerror(errno));
        }
        // Open the right input file
        pInputFileRight = fopen (pInputFileNameRight, "rb");
        if (pInputFileLeft == NULL) {
            success = false;
            printf("Cannot open right channel input file %s (%s).\n", pInputFileNameRight, strerror(errno));
        }
        // Open the output file
        pOutputFile = fopen(pOutputFileName, "wb");
        if (pOutputFile == NULL) {
            success = false;
            printf("Cannot open output file %s (%s).\n", pOutputFileName, strerror(errno));
        }
        if (success) {
            printf("Parsing mono left channel file %s (%s endian with %d byte words) and mono right channel\n",
                   pInputFileNameLeft, *pEndiannessLeft == 'l' ? "little" : "big", wordWidthLeft);
            printf("file %s (%s endian with %d byte words) and writing stereo output to file %s.\n",
                   pInputFileNameRight, *pEndiannessRight == 'l' ? "little" : "big", wordWidthRight,
                   pOutputFileName);                   
            x = parse(pInputFileLeft, *pEndiannessLeft == 'l' ? true : false, wordWidthLeft,
                      pInputFileRight, *pEndiannessRight == 'l' ? true : false, wordWidthRight,
                      pOutputFile);
            printf("Done: %d item(s) written to file.\n", x);
        } else {
            printUsage(pExeName);
        }
    } else {
        printUsage(pExeName);
    }

    if (success) {
        retValue = 0;
    }

    // Clean up
    if (pInputFileLeft != NULL) {
        fclose(pInputFileLeft);
    }
    if (pInputFileRight != NULL) {
        fclose(pInputFileRight);
    }
    if (pOutputFile != NULL) {
        fclose(pOutputFile);
    }

    return retValue;
}