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

/* ============================================================
   DECOMPRESSION — Read .huff file and restore original file
   ============================================================ */

/*
 * readU32
 * Reads 4 bytes from the file and assembles them into an
 * unsigned int using little-endian order (same as writeU32).
 */
static unsigned int readU32(FILE *fp)
{
    unsigned char b[4];
    if (fread(b, 1, 4, fp) != 4) return 0;
    return (unsigned int)b[0]
         | ((unsigned int)b[1] << 8)
         | ((unsigned int)b[2] << 16)
         | ((unsigned int)b[3] << 24);
}

/*
 * readU64
 * Reads 8 bytes from the file and assembles them into an
 * unsigned long using little-endian order (same as writeU64).
 */
static unsigned long readU64(FILE *fp)
{
    unsigned char b[8];
    if (fread(b, 1, 8, fp) != 8) return 0;
    unsigned long v = 0;
    int i;
    for (i = 7; i >= 0; i--)
        v = (v << 8) | b[i];
    return v;
}

/*
 * readBit
 * Reads one bit at a time from the file by buffering a full byte
 * and serving its bits one by one from the most-significant side.
 * Returns 0 or 1 on success, -1 on end of file.
 *
 * Uses static variables so state is preserved between calls.
 * Call resetReadBit() before starting a new decompression.
 */
static unsigned char readBuf   = 0;  /* Byte currently being consumed    */
static int           readCount = 0;  /* Bits remaining in readBuf        */

static void resetReadBit(void)
{
    readBuf   = 0;
    readCount = 0;
}

static int readBit(FILE *fp)
{
    /* Load the next byte when the current one is exhausted */
    if (readCount == 0) {
        if (fread(&readBuf, 1, 1, fp) != 1)
            return -1;   /* End of file */
        readCount = 8;
    }

    /* Extract the most-significant bit */
    int bit = (readBuf >> 7) & 1;
    readBuf   = (unsigned char)(readBuf << 1);
    readCount--;
    return bit;
}

/*
 * decompressFile
 *
 * How decompression works:
 *
 * 1. Open the .huff file and read the header:
 *      - 8 bytes  → original file size
 *      - 1024 bytes → frequency table (256 × 4 bytes)
 *
 * 2. Rebuild the exact same Huffman tree using the frequency
 *    table (buildHuffmanTree gives the same tree every time
 *    for the same frequency table).
 *
 * 3. Read the compressed bit stream bit by bit.
 *    Walk the tree: bit=0 → go left, bit=1 → go right.
 *    When a leaf node is reached → write that byte to output.
 *    Reset back to root and repeat.
 *
 * 4. Stop when we have written exactly originalSize bytes
 *    (this handles the padding bits at the end correctly).
 */
int decompressFile(const char *inputPath, const char *outputPath)
{
    /* ── 1. Open the compressed file ────────────────────── */
    FILE *in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Error: cannot open compressed file '%s'\n", inputPath);
        return -1;
    }

    /* ── 2. Read header: original file size ─────────────── */
    unsigned long originalSize = readU64(in);
    if (originalSize == 0) {
        fprintf(stderr, "Error: compressed file is empty or corrupt.\n");
        fclose(in);
        return -1;
    }

    /* ── 3. Read header: frequency table ────────────────── */
    unsigned int freq[BYTE_RANGE];
    int i;
    for (i = 0; i < BYTE_RANGE; i++)
        freq[i] = readU32(in);

    /* ── 4. Rebuild the Huffman tree ─────────────────────── */
    HuffmanNode *root = buildHuffmanTree(freq);
    if (!root) {
        fprintf(stderr, "Error: could not rebuild Huffman tree.\n");
        fclose(in);
        return -1;
    }

    /* ── 5. Open output file ─────────────────────────────── */
    FILE *out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Error: cannot create output file '%s'\n", outputPath);
        freeTree(root);
        fclose(in);
        return -1;
    }

    /* ── 6. Decode the bit stream ────────────────────────── */
    resetReadBit();

    unsigned long decoded = 0;       /* How many bytes written so far  */
    HuffmanNode *current  = root;    /* Start walking from the root    */

    while (decoded < originalSize) {
        int bit = readBit(in);
        if (bit < 0) {
            /* Unexpected end of file before restoring all bytes */
            fprintf(stderr, "Error: compressed data ended too early.\n");
            break;
        }

        /* Walk left for 0, right for 1 */
        if (bit == 0)
            current = current->left;
        else
            current = current->right;

        /* Safety check for corrupt data */
        if (!current) {
            fprintf(stderr, "Error: corrupt compressed file.\n");
            break;
        }

        /* Reached a leaf node — this is a decoded byte */
        if (!current->left && !current->right) {
            fputc(current->byte, out);   /* Write the original byte    */
            decoded++;
            current = root;              /* Reset to root for next byte */
        }
    }

    /* ── 7. Clean up ─────────────────────────────────────── */
    fclose(in);
    fclose(out);
    freeTree(root);

    if (decoded != originalSize) {
        fprintf(stderr, "Warning: expected %lu bytes but decoded %lu.\n",
                originalSize, decoded);
        return -1;
    }

    return 0;
}