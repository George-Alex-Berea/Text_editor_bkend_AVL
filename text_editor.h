#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

// Source of the text in the node
typedef enum p_source {
    ORIGINAL,
    ADD
} PieceSource;

typedef struct piece {
    long start; // the index of the first character in the piece
    PieceSource source; // the text source 
    long length; // the length of the text refered to by the piece
} Piece;

// the piece tree, intended to be navigated like a rope data structure, by knowing the length of the left child
// the depth field is needed for the AVL balancing
typedef struct p_node {
    Piece data;
    long left_child_length;
    int depth;
    struct p_node *left;
    struct p_node *right;
} *PieceTree, PieceNode;

// like c++ vector, or java ArrayList
typedef struct array_list {
    long length;
    long capacity;
    char *array;
} *ArrayList;

// the structure bringing everything together
typedef struct TextEditor {
    const char *filename;
    long cursor_position;
    ArrayList original_text;
    ArrayList added_text;
    PieceTree root;
} TextEditor;

// Create a new text editor
// filename: if not NULL, load content from this file
// text: if not NULL (and filename is NULL), initialize with this text
// global_cursor: initial cursor position
TextEditor *create_table(const char *filename, const char *text, int global_cursor);

// Move cursor by 'advance' positions (can be negative for backwards)
void advance_cursor(TextEditor *editor, int advance);

// Get current cursor position
int show_global_cursor(TextEditor *editor);

// Get total text length
int show_total_len(TextEditor *editor);

// Insert text at current cursor position
void add_text(TextEditor *editor, char *text);

// Delete 'length' characters before cursor
void delete_text(TextEditor *editor, int length);

// Extract full text as allocated string (caller must free)
char *extract_current_text(TextEditor *editor);

// Save text to file or return via text pointer
void save_current_text(TextEditor *editor, const char *filename, char **text, int *global_cursor);

void arraylist_add(ArrayList a, char* source, long text_length);

// exctracts the displayed text and saves it in a
void extract_text_tree(ArrayList a, PieceTree root, TextEditor *editor);

PieceTree create_node(PieceSource source, long start, long length);

void add_text_tree(PieceTree *root, char *text, long length, long array_position, long cursor);

long delete_text_tree(PieceTree *root, long low, long high);

void add_to_left(PieceTree *root, PieceTree node);

void add_to_right(PieceTree *root, PieceTree node);

void update_depth(PieceTree root);

void balance_tree(PieceTree *root);

void rotate_right(PieceTree *root);

void rotate_left(PieceTree *root);

PieceTree pop_rightmost_node(PieceTree *root);

int max(int a, int b);
#endif // TEXT_EDITOR_H
