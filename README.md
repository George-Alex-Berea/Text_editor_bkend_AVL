# High-Performance Text Editor Core

A high-performance text buffer backend implemented in C (POSIX), designed to handle large documents with minimal memory copying. The engine couples an append-only **Piece Table** memory model with a self-balancing, augmented **AVL Tree (Rope-like structure)** to maintain logarithmic insertion, deletion, and cursor-offset resolution.

---

## Architectural Overview

Standard dynamic buffers incur $O(N)$ memory copies for insertions and deletions, while basic array/linked-list piece tables degrade offset navigation to $O(N)$. 

This architecture maintains continuous virtual text by decoupling the raw character storage from text structure via a balanced binary tree:

1. **Zero-Copy File Loading (`mmap`):**
   * Files are mapped into virtual memory using `mmap(..., PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)`.
   * Avoids reading entire files into heap memory upfront, relying on kernel page paging on demand.
2. **Append-Only Modification Buffer (`ArrayList`):**
   * All user insertions are appended to a dynamically resized character buffer (`realloc` exponential doubling strategy).
   * Existing text buffers remain immutable, preventing fragmentation and data shifting.
3. **Augmented AVL Piece Tree (`PieceTree`):**
   * Each node represents a slice descriptor: `Piece { source, start, length }`.
   * **Subtree Weight Augmentation (`left_child_length`):** Nodes track the total character count of their left subtree, enabling $O(\log N)$ conversion from linear cursor coordinates to specific tree nodes (similar to a Rope data structure).
   * **Self-Balancing Invariant:** Rotations (`rotate_left`, `rotate_right`) maintain strict AVL height balance ($|\Delta \text{depth}| \le 1$) and update `left_child_length` to reflect topological shifts.

---

## Operations & Algorithmic Complexity

| Operation | Implementation Mechanism | Time Complexity |
| :--- | :--- | :--- |
| **Initial File Load** | POSIX `open` + `fstat` + `mmap` | $O(1)$ page-fault driven |
| **Offset Lookup** | Traversal via augmented `left_child_length` | $O(\log N)$ |
| **Arbitrary Insert** | Buffer append + Node split + AVL rebalancing | $O(\log N)$ |
| **Range Deletion** | Subtree trimming/pruning + AVL rebalancing | $O(K \log N)$ |
| **Export / Save** | In-order tree traversal (`extract_text_tree`) | $O(N)$ |

---

## Core Algorithms

### 1. Cursor-Based Insertion (`add_text`)
* Appends new text to the dynamic `added_text` buffer.
* Traverses the tree comparing the cursor against `left_child_length` and `data.length`.
* Replaces the targeted piece node with a new node representing the inserted content, splitting the original node into left/right prefix and suffix pieces.
* Slices are re-anchored using `add_to_right` and `add_to_left`, followed by AVL balancing passes (`balance_tree`).

### 2. Range Deletion (`delete_text`)
* Deletes a range `[cursor - length, cursor]` relative to the cursor.
* Recursively locates intersecting piece nodes:
  * Truncates boundary pieces by updating `data.start` and `data.length`.
  * Drops fully encompassed piece nodes, restructuring the tree via `pop_rightmost_node`.
* Updates prefix character counts and balances ancestors via AVL rotations.

### 3. AVL Rotations with Offset Bookkeeping
Standard tree rotations must preserve character index resolution. When performing rotations, the engine recalculates augmented sizes:
* **Right Rotation:** Decrements the displaced root's `left_child_length` by the moving child's total weight (`left_node->left_child_length + left_node->data.length`).
* **Left Rotation:** Increments the new root's `left_child_length` by the left child's total weight.

---

## Data Structures (`text_editor.h`)

```c
typedef enum p_source {
    ORIGINAL,
    ADD
} PieceSource;

typedef struct piece {
    long start;          // Offset into source buffer
    PieceSource source;  // ORIGINAL (mmap) or ADD (dynamic buffer)
    long length;         // Character length of piece
} Piece;

typedef struct p_node {
    Piece data;
    long left_child_length; // Character count in left subtree (for Rope-like traversal)
    int depth;              // AVL balancing factor
    struct p_node *left;
    struct p_node *right;
} *PieceTree, PieceNode;
