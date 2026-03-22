/* ============================================================
   main.c  –  Huffman File Compressor & Decompressor
   Entry point for the program.

   Usage:
     Compress:
       compressor.exe c <input_file> <output.huff>

     Decompress:
       compressor.exe d <input.huff> <output_file>

   Examples:
     compressor.exe c input.txt       output.huff
     compressor.exe d output.huff     restored.txt
     compressor.exe c photo.bmp       photo.huff
     compressor.exe d photo.huff      photo_restored.bmp

   Works for ANY file type: text, image, audio, video, binary.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"
#include "file_io.h"

/* ── Helper: get file size in bytes ───────────────────────── */
static long getFileSize(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

/* ── Helper: print usage instructions ─────────────────────── */
static void printUsage(const char *prog)
{
    printf("\n====================================\n");
    printf("   Huffman File Compressor\n");
    printf("====================================\n");
    printf("Usage:\n");
    printf("  Compress  : %s c <input_file>  <output.huff>\n", prog);
    printf("  Decompress: %s d <input.huff>  <output_file>\n", prog);
    printf("\nExamples:\n");
    printf("  %s c input.txt    output.huff\n", prog);
    printf("  %s d output.huff  restored.txt\n", prog);
    printf("  %s c photo.bmp    photo.huff\n", prog);
    printf("  %s d photo.huff   photo_restored.bmp\n\n", prog);
}

/* ── COMPRESS mode ─────────────────────────────────────────── */
static int runCompress(const char *inputPath, const char *outputPath)
{
    printf("\n====================================\n");
    printf("   MODE: COMPRESSION\n");
    printf("====================================\n");
    printf("Input  : %s\n", inputPath);
    printf("Output : %s\n", outputPath);

    /* Step 1 & 2: Count byte frequencies */
    printf("\n[Step 1] Reading file and counting byte frequencies...\n");

    unsigned int freq[BYTE_RANGE];
    long originalSize = countFrequencies(inputPath, freq);
    if (originalSize < 0) return 1;
    if (originalSize == 0) {
        fprintf(stderr, "Error: input file is empty.\n");
        return 1;
    }

    printf("         File size     : %ld bytes\n", originalSize);

    int distinct = 0, i;
    for (i = 0; i < BYTE_RANGE; i++)
        if (freq[i] > 0) distinct++;
    printf("         Distinct bytes: %d / 256\n", distinct);

    /* Step 3 & 4: Build Huffman Tree */
    printf("\n[Step 2] Building Huffman Tree...\n");
    HuffmanNode *root = buildHuffmanTree(freq);
    if (!root) return 1;
    printf("         Tree built successfully.\n");

    /* Step 5: Generate codes */
    printf("\n[Step 3] Generating Huffman codes...\n");
    HuffmanCode codes[BYTE_RANGE];
    memset(codes, 0, sizeof(codes));
    int path[MAX_CODE_BITS];
    generateCodes(root, codes, path, 0);
    printCodes(codes, freq);

    /* Step 6 & 7: Compress and write */
    printf("[Step 4] Writing compressed file...\n");
    if (compressFile(inputPath, outputPath, codes, freq, originalSize) != 0) {
        freeTree(root);
        return 1;
    }

    /* Print results */
    long compressedSize = getFileSize(outputPath);
    printf("\n====================================\n");
    printf("   Compression Results\n");
    printf("====================================\n");
    printf("Original size   : %ld bytes\n", originalSize);
    printf("Compressed size : %ld bytes\n", compressedSize);

    if (originalSize > 0 && compressedSize > 0) {
        double ratio   = (double)compressedSize / (double)originalSize * 100.0;
        double savings = 100.0 - ratio;
        printf("Compression ratio  : %.2f%%\n", ratio);
        if (savings > 0)
            printf("Space saved        : %.2f%%\n", savings);
        else
            printf("Space saved        : 0%% (overhead from header)\n");
    }

    printf("====================================\n");
    printf("Done. Compressed → %s\n\n", outputPath);

    freeTree(root);
    return 0;
}

/* ── DECOMPRESS mode ───────────────────────────────────────── */
static int runDecompress(const char *inputPath, const char *outputPath)
{
    printf("\n====================================\n");
    printf("   MODE: DECOMPRESSION\n");
    printf("====================================\n");
    printf("Input  : %s\n", inputPath);
    printf("Output : %s\n", outputPath);

    printf("\n[Step 1] Reading header and rebuilding Huffman Tree...\n");
    printf("[Step 2] Decoding bit stream...\n");

    if (decompressFile(inputPath, outputPath) != 0) {
        fprintf(stderr, "Decompression failed.\n");
        return 1;
    }

    /* Print results */
    long restoredSize = getFileSize(outputPath);
    printf("\n====================================\n");
    printf("   Decompression Results\n");
    printf("====================================\n");
    printf("Compressed size  : %ld bytes\n", getFileSize(inputPath));
    printf("Restored size    : %ld bytes\n", restoredSize);
    printf("====================================\n");
    printf("Done. Restored → %s\n\n", outputPath);

    return 0;
}

/* ── main ──────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    /* Need exactly 4 arguments: program mode input output */
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    const char *mode       = argv[1];
    const char *inputPath  = argv[2];
    const char *outputPath = argv[3];

    /* Route to compress or decompress based on mode flag */
    if (strcmp(mode, "c") == 0 || strcmp(mode, "C") == 0) {
        return runCompress(inputPath, outputPath);

    } else if (strcmp(mode, "d") == 0 || strcmp(mode, "D") == 0) {
        return runDecompress(inputPath, outputPath);

    } else {
        fprintf(stderr, "\nUnknown mode '%s'. Use 'c' to compress or 'd' to decompress.\n\n", mode);
        printUsage(argv[0]);
        return 1;
    }
}