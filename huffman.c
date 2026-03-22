/* ============================================================
   huffman.c
   Implements the Min Heap and Huffman Tree used for compression.

   How it works:
   1. We put every byte (that appears in the file) into a node.
   2. We push all nodes into a Min Heap (smallest freq at top).
   3. Repeatedly pull the two smallest nodes, merge them into
      a parent node, and push the parent back.
   4. When only one node remains, that is the root of the tree.
   5. We walk the tree to assign a unique bit-string (code) to
      every leaf (every byte that appeared in the file).
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"

/* ============================================================
   MIN HEAP  (Priority Queue)
   ============================================================ */

/* Create an empty min-heap with the given capacity. */
MinHeap *createMinHeap(int capacity)
{
    MinHeap *heap = (MinHeap *)malloc(sizeof(MinHeap));
    if (!heap) { fprintf(stderr, "Memory error: createMinHeap\n"); exit(1); }

    heap->nodes    = (HuffmanNode **)malloc(sizeof(HuffmanNode *) * capacity);
    if (!heap->nodes) { fprintf(stderr, "Memory error: heap nodes\n"); exit(1); }

    heap->size     = 0;
    heap->capacity = capacity;
    return heap;
}

/* Swap two node pointers inside the heap array. */
static void swapNodes(HuffmanNode **a, HuffmanNode **b)
{
    HuffmanNode *tmp = *a;
    *a = *b;
    *b = tmp;
}

/*
 * siftUp  –  After inserting at the end, bubble the new node
 *            upward until the heap property is restored.
 *            (parent always has smaller or equal frequency)
 */
static void siftUp(MinHeap *heap, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap->nodes[parent]->frequency > heap->nodes[i]->frequency) {
            swapNodes(&heap->nodes[parent], &heap->nodes[i]);
            i = parent;
        } else {
            break;
        }
    }
}

/*
 * siftDown  –  After removing the root, push the replacement
 *              node downward until the heap property is restored.
 */
static void siftDown(MinHeap *heap, int i)
{
    while (1) {
        int smallest = i;
        int left     = 2 * i + 1;
        int right    = 2 * i + 2;

        if (left  < heap->size &&
            heap->nodes[left]->frequency  < heap->nodes[smallest]->frequency)
            smallest = left;

        if (right < heap->size &&
            heap->nodes[right]->frequency < heap->nodes[smallest]->frequency)
            smallest = right;

        if (smallest == i) break;   /* Heap property satisfied */

        swapNodes(&heap->nodes[i], &heap->nodes[smallest]);
        i = smallest;
    }
}

/* Insert a node into the min-heap. */
void insertMinHeap(MinHeap *heap, HuffmanNode *node)
{
    if (heap->size == heap->capacity) {
        fprintf(stderr, "Error: heap is full.\n");
        exit(1);
    }
    heap->nodes[heap->size] = node;
    heap->size++;
    siftUp(heap, heap->size - 1);
}

/*
 * extractMin  –  Remove and return the node with the
 *                lowest frequency (the root of the heap).
 */
HuffmanNode *extractMin(MinHeap *heap)
{
    if (heap->size == 0) return NULL;

    HuffmanNode *min   = heap->nodes[0];      /* Save the minimum         */
    heap->nodes[0]     = heap->nodes[heap->size - 1]; /* Move last to root */
    heap->size--;
    siftDown(heap, 0);                        /* Restore heap property    */
    return min;
}

/* Free the heap structure (does NOT free the nodes themselves). */
void freeMinHeap(MinHeap *heap)
{
    if (!heap) return;
    free(heap->nodes);
    free(heap);
}

/* ============================================================
   HUFFMAN NODE HELPERS
   ============================================================ */

/* Create a new leaf node for a given byte and its frequency. */
HuffmanNode *createNode(unsigned char byte, unsigned int frequency)
{
    HuffmanNode *node = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    if (!node) { fprintf(stderr, "Memory error: createNode\n"); exit(1); }

    node->byte      = byte;
    node->frequency = frequency;
    node->left      = NULL;
    node->right     = NULL;
    return node;
}

/* Recursively free every node in the Huffman tree. */
void freeTree(HuffmanNode *root)
{
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

/* ============================================================
   BUILDING THE HUFFMAN TREE
   ============================================================ */

/*
 * buildHuffmanTree
 *
 * Given a frequency table (freq[0..255]), builds and returns
 * the root of the Huffman tree.
 *
 * Algorithm:
 *   - Insert every byte that has frequency > 0 as a leaf node.
 *   - While more than one node remains in the heap:
 *       a. Extract the two nodes with the lowest frequencies.
 *       b. Create an internal (parent) node whose frequency
 *          is the sum of the two children.
 *       c. Insert the parent back into the heap.
 *   - The last remaining node is the tree root.
 */
HuffmanNode *buildHuffmanTree(unsigned int freq[BYTE_RANGE])
{
    /* Count how many distinct bytes actually appear */
    int distinct = 0;
    for (int i = 0; i < BYTE_RANGE; i++)
        if (freq[i] > 0) distinct++;

    if (distinct == 0) {
        fprintf(stderr, "Error: input file is empty.\n");
        return NULL;
    }

    /* Create the min-heap large enough for all nodes */
    MinHeap *heap = createMinHeap(BYTE_RANGE * 2);

    /* Insert a leaf node for every byte that appears */
    for (int i = 0; i < BYTE_RANGE; i++) {
        if (freq[i] > 0) {
            insertMinHeap(heap, createNode((unsigned char)i, freq[i]));
        }
    }

    /*
     * Edge case: only one distinct byte in the file.
     * Wrap it in a root so the tree still has two levels.
     */
    if (heap->size == 1) {
        HuffmanNode *only   = extractMin(heap);
        HuffmanNode *root   = createNode(0, only->frequency);
        root->left          = only;
        freeMinHeap(heap);
        return root;
    }

    /* Main loop: merge two smallest until one node remains */
    while (heap->size > 1) {
        HuffmanNode *left  = extractMin(heap);  /* smallest          */
        HuffmanNode *right = extractMin(heap);  /* second smallest   */

        /* Internal node: byte field is unused (set to 0) */
        HuffmanNode *parent = createNode(0, left->frequency + right->frequency);
        parent->left  = left;
        parent->right = right;

        insertMinHeap(heap, parent);
    }

    HuffmanNode *root = extractMin(heap);
    freeMinHeap(heap);
    return root;
}

/* ============================================================
   GENERATING HUFFMAN CODES
   ============================================================ */

/*
 * generateCodes  (recursive)
 *
 * Walks the Huffman tree depth-first.
 * - Going LEFT  appends a 0 to the current path.
 * - Going RIGHT appends a 1 to the current path.
 * - At a LEAF, we save the accumulated path as the code
 *   for that leaf's byte value.
 *
 * Parameters:
 *   root   – current node being visited
 *   codes  – output array indexed by byte value
 *   path   – array recording the 0/1 choices made so far
 *   depth  – how many levels deep we currently are
 */
void generateCodes(HuffmanNode *root,
                   HuffmanCode codes[BYTE_RANGE],
                   int path[], int depth)
{
    if (!root) return;

    /* ── Leaf node: record the code for this byte ── */
    if (!root->left && !root->right) {
        /* Handle single-symbol edge case: assign code "0" */
        if (depth == 0) {
            codes[root->byte].bits[0] = 0;
            codes[root->byte].len     = 1;
        } else {
            codes[root->byte].len = depth;
            for (int i = 0; i < depth; i++)
                codes[root->byte].bits[i] = path[i];
        }
        return;
    }

    /* ── Internal node: recurse left (0) and right (1) ── */
    if (root->left) {
        path[depth] = 0;
        generateCodes(root->left,  codes, path, depth + 1);
    }
    if (root->right) {
        path[depth] = 1;
        generateCodes(root->right, codes, path, depth + 1);
    }
}

/* ============================================================
   DISPLAY HELPER
   ============================================================ */

/*
 * printCodes
 * Prints the Huffman code for every byte that appears in the
 * file. Useful for understanding and demonstrating the project.
 */
void printCodes(HuffmanCode codes[BYTE_RANGE],
                unsigned int freq[BYTE_RANGE])
{
    printf("\n%-10s %-12s %-10s %s\n", "Byte", "Char", "Frequency", "Code");
    printf("----------------------------------------------\n");

    for (int i = 0; i < BYTE_RANGE; i++) {
        if (freq[i] == 0) continue;   /* Skip bytes that never appeared */

        /* Print the byte value */
        printf("%-10d ", i);

        /* Print a human-readable character if printable */
        if (i >= 32 && i <= 126)
            printf("'%c'%-9s", (char)i, "");
        else
            printf("%-12s", "(non-print)");

        /* Print frequency */
        printf("%-10u ", freq[i]);

        /* Print the bit code */
        for (int j = 0; j < codes[i].len; j++)
            printf("%d", codes[i].bits[j]);
        printf("\n");
    }
    printf("\n");
}
