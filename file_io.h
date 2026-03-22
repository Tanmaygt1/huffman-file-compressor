#ifndef FILE_IO_H
#define FILE_IO_H

/* ============================================================
   file_io.h
   Declares functions for reading the input file, counting byte
   frequencies, and writing the compressed output file.
   ============================================================ */

#include "huffman.h"

/* ── Function Prototypes ───────────────────────────────────── */

/*
 * countFrequencies
 * Opens the file at 'filepath' in binary mode, reads every byte,
 * and fills the freq[] array with how many times each byte (0-255)
 * appears.  Returns the total number of bytes read, or -1 on error.
 */
long countFrequencies(const char *filepath, unsigned int freq[BYTE_RANGE]);

/*
 * compressFile
 * Reads the input file byte by byte, looks up each byte's Huffman
 * code, and writes the resulting bits (packed into bytes) into the
 * output file.  Also writes a simple header so the file size and
 * frequency table can be stored.
 * Returns 0 on success, -1 on error.
 */
int compressFile(const char *inputPath,
                 const char *outputPath,
                 HuffmanCode codes[BYTE_RANGE],
                 unsigned int freq[BYTE_RANGE],
                 long originalSize);

#endif /* FILE_IO_H */
