#ifndef HUFFMAN_H
#define HUFFMAN_H

/* ============================================================
   huffman.h
   Declares the Huffman Node, Min Heap, and all functions
   needed to build the Huffman tree and generate codes.
   ============================================================ */

#define BYTE_RANGE 256          /* Total possible byte values: 0-255     */
#define MAX_CODE_BITS 256       /* Maximum bits in a single Huffman code  */

/* ------------------------------------------------------------
   HuffmanNode
   Each node in the Huffman tree stores:
   - byte       : the actual byte value (only meaningful in leaf nodes)
   - frequency  : how often this byte appeared in the input file
   - left/right : pointers to child nodes (NULL in leaf nodes)
   ------------------------------------------------------------ */
typedef struct HuffmanNode {
    unsigned char byte;
    unsigned int  frequency;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

/* ------------------------------------------------------------
   MinHeap
   A simple min-heap (priority queue) used to always pick the
   two nodes with the smallest frequencies during tree building.
   - nodes    : array of pointers to HuffmanNode
   - size     : current number of elements in the heap
   - capacity : maximum number of elements the heap can hold
   ------------------------------------------------------------ */
typedef struct {
    HuffmanNode **nodes;
    int size;
    int capacity;
} MinHeap;

/* ------------------------------------------------------------
   HuffmanCode
   Stores the variable-length bit pattern for one byte value.
   - bits : array of 0s and 1s representing the code
   - len  : how many bits long the code is
   ------------------------------------------------------------ */
typedef struct {
    int bits[MAX_CODE_BITS];
    int len;
} HuffmanCode;

/* ── Function Prototypes ───────────────────────────────────── */

/* Min Heap operations */
MinHeap    *createMinHeap(int capacity);
void        insertMinHeap(MinHeap *heap, HuffmanNode *node);
HuffmanNode *extractMin(MinHeap *heap);
void        freeMinHeap(MinHeap *heap);

/* Huffman node helpers */
HuffmanNode *createNode(unsigned char byte, unsigned int frequency);
void         freeTree(HuffmanNode *root);

/* Tree construction */
HuffmanNode *buildHuffmanTree(unsigned int freq[BYTE_RANGE]);

/* Code generation */
void generateCodes(HuffmanNode *root,
                   HuffmanCode codes[BYTE_RANGE],
                   int path[], int depth);

/* Display helper (prints codes to terminal) */
void printCodes(HuffmanCode codes[BYTE_RANGE],
                unsigned int freq[BYTE_RANGE]);

#endif /* HUFFMAN_H */
