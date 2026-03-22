/* ============================================================
   file_io.c
   Handles all file input/output for the Huffman compressor.

   countFrequencies  – reads the input file and tallies byte counts.
   compressFile      – encodes the file and writes the .huff output.

   Output file format (.huff)
   ──────────────────────────
   [ 8 bytes ] original file size  (unsigned long, little-endian)
   [1024 bytes] frequency table    (256 x unsigned int, 4 bytes each)
   [variable ] compressed bit stream, packed 8 bits per byte;
               the final byte may be padded with 0-bits on the right.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"

/* ============================================================
   STEP 1 & 2 — Read the file and count byte frequencies
   ============================================================ */

/*
 * countFrequencies
 *
 * Opens 'filepath' in binary mode ("rb") so it works correctly
 * for ALL file types: text, images, audio, video, executables, etc.
 *
 * Reads the file one byte at a time and increments freq[byte].
 * Returns the total byte count, or -1 if the file cannot be opened.
 */
long countFrequencies(const char *filepath, unsigned int freq[BYTE_RANGE])
{
    /* Clear the frequency table */
    memset(freq, 0, sizeof(unsigned int) * BYTE_RANGE);

    FILE *fp = fopen(filepath, "rb");   /* "rb" = read binary */
    if (!fp) {
        fprintf(stderr, "Error: cannot open input file '%s'\n", filepath);
        return -1;
    }

    long totalBytes = 0;
    int  ch;

    /* fgetc returns each byte as an int (0-255), or EOF */
    while ((ch = fgetc(fp)) != EOF) {
        freq[(unsigned char)ch]++;   /* Increment count for this byte value */
        totalBytes++;
    }

    fclose(fp);
    return totalBytes;
}

/* ============================================================
   HELPER — Bit-level writer
   ============================================================ */

/*
 * We need to pack individual bits (0 or 1) into bytes before
 * writing to disk.  These two small helpers manage a 1-byte buffer.
 *
 *   writeBit  – adds one bit to the buffer; flushes when full.
 *   flushBits – writes any leftover bits (zero-padded) at the end.
 */
static unsigned char bitBuffer   = 0;  /* Current byte being assembled  */
static int           bitCount    = 0;  /* How many bits are filled so far */

static void writeBit(FILE *fp, int bit)
{
    /* Place the bit into the buffer from the most-significant side */
    bitBuffer = (unsigned char)((bitBuffer << 1) | (bit & 1));
    bitCount++;

    /* When we have 8 bits, write the complete byte to disk */
    if (bitCount == 8) {
        fwrite(&bitBuffer, 1, 1, fp);
        bitBuffer = 0;
        bitCount  = 0;
    }
}

static void flushBits(FILE *fp)
{
    /* If there are leftover bits that didn't fill a full byte,
       shift them to the high side (pad with 0s on the right)
       and write the partial byte. */
    if (bitCount > 0) {
        bitBuffer = (unsigned char)(bitBuffer << (8 - bitCount));
        fwrite(&bitBuffer, 1, 1, fp);
        bitBuffer = 0;
        bitCount  = 0;
    }
}

/* ============================================================
   PORTABLE LITTLE-ENDIAN WRITERS
   ============================================================ */

/* Write a 4-byte unsigned int in little-endian order */
static void writeU32(FILE *fp, unsigned int v)
{
    unsigned char b[4];
    b[0] = (unsigned char)( v        & 0xFF);
    b[1] = (unsigned char)((v >>  8) & 0xFF);
    b[2] = (unsigned char)((v >> 16) & 0xFF);
    b[3] = (unsigned char)((v >> 24) & 0xFF);
    fwrite(b, 1, 4, fp);
}

/* Write an 8-byte unsigned long in little-endian order */
static void writeU64(FILE *fp, unsigned long v)
{
    unsigned char b[8];
    int i;
    for (i = 0; i < 8; i++) {
        b[i] = (unsigned char)(v & 0xFF);
        v >>= 8;
    }
    fwrite(b, 1, 8, fp);
}

/* ============================================================
   STEPS 5–7 — Encode and write the compressed file
   ============================================================ */

/*
 * compressFile
 *
 * Reads the input file a second time.  For each byte read, it
 * looks up the Huffman code and writes those bits one by one.
 *
 * Header written first (so decompression is possible):
 *   - 8 bytes  : original file size
 *   - 1024 bytes : frequency table (256 entries × 4 bytes each)
 *
 * Then the compressed bit stream follows.
 */
int compressFile(const char *inputPath,
                 const char *outputPath,
                 HuffmanCode codes[BYTE_RANGE],
                 unsigned int freq[BYTE_RANGE],
                 long originalSize)
{
    /* ── Open input (binary read) ─────────────────────────── */
    FILE *in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Error: cannot open input file '%s'\n", inputPath);
        return -1;
    }

    /* ── Open output (binary write) ─────────────────────────*/
    FILE *out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Error: cannot create output file '%s'\n", outputPath);
        fclose(in);
        return -1;
    }

    /* ── Write header: original size ────────────────────────*/
    writeU64(out, (unsigned long)originalSize);

    /* ── Write header: frequency table ─────────────────────*/
    for (int i = 0; i < BYTE_RANGE; i++)
        writeU32(out, freq[i]);

    /* ── Write compressed bit stream ────────────────────────*/
    /* Reset the bit-buffer state before writing */
    bitBuffer = 0;
    bitCount  = 0;

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        unsigned char sym = (unsigned char)ch;

        /* Write each bit of this byte's Huffman code */
        for (int i = 0; i < codes[sym].len; i++)
            writeBit(out, codes[sym].bits[i]);
    }

    /* Flush any remaining bits (padded to a full byte) */
    flushBits(out);

    fclose(in);
    fclose(out);
    return 0;
}
