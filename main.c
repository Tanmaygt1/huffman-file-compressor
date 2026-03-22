/* ============================================================
   main.c  –  Huffman File Compressor
   Entry point for the program.

   Usage:
     ./compressor <input_file> <output_file>

   Example:
     ./compressor input.txt output.huff

   Works for ANY file type: text, image, audio, video, binary.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"
#include "file_io.h"

/* Helper: get file size in bytes */
static long getFileSize(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

/* ── main ──────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    /* ── 1. Check command-line arguments ─────────────────── */
    if (argc != 3) {
        printf("\nUsage: %s <input_file> <output_file>\n", argv[0]);
        printf("Example:\n");
        printf("  %s input.txt output.huff\n\n", argv[0]);
        return 1;
    }

    const char *inputPath  = argv[1];
    const char *outputPath = argv[2];

    printf("\n====================================\n");
    printf("   Huffman File Compressor\n");
    printf("====================================\n");
    printf("Input  : %s\n", inputPath);
    printf("Output : %s\n", outputPath);

    /* ── 2. Count byte frequencies (Step 1 & 2) ─────────── */
    printf("\n[Step 1] Reading input file and counting byte frequencies...\n");

    unsigned int freq[BYTE_RANGE];
    long originalSize = countFrequencies(inputPath, freq);
    if (originalSize < 0) return 1;   /* Error already printed */
    if (originalSize == 0) {
        fprintf(stderr, "Error: input file is empty.\n");
        return 1;
    }

    printf("         File size     : %ld bytes\n", originalSize);

    /* Count how many distinct byte values appear */
    int distinct = 0;
    for (int i = 0; i < BYTE_RANGE; i++)
        if (freq[i] > 0) distinct++;
    printf("         Distinct bytes: %d / 256\n", distinct);

    /* ── 3. Build Huffman Tree (Steps 3 & 4) ─────────────── */
    printf("\n[Step 2] Building Huffman Tree...\n");

    HuffmanNode *root = buildHuffmanTree(freq);
    if (!root) return 1;

    printf("         Tree built successfully.\n");

    /* ── 4. Generate Huffman Codes (Step 5) ──────────────── */
    printf("\n[Step 3] Generating Huffman codes...\n");

    HuffmanCode codes[BYTE_RANGE];
    memset(codes, 0, sizeof(codes));

    int path[MAX_CODE_BITS];
    generateCodes(root, codes, path, 0);

    /* Print the code table so the concept is visible */
    printCodes(codes, freq);

    /* ── 5. Compress and write output file (Steps 6 & 7) ─── */
    printf("[Step 4] Compressing file...\n");

    if (compressFile(inputPath, outputPath, codes, freq, originalSize) != 0) {
        freeTree(root);
        return 1;
    }

    /* ── 6. Print statistics ──────────────────────────────── */
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
            printf("Space saved        : 0%% (file grew due to header overhead)\n");
    }

    printf("====================================\n\n");

    /* ── 7. Clean up ──────────────────────────────────────── */
    freeTree(root);

    printf("Done. Compressed file written to: %s\n\n", outputPath);
    return 0;
}
