#include "text_editor.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

TextEditor *create_table(const char *filename, const char *text, int global_cursor) {
    TextEditor *editor = malloc(sizeof(TextEditor));
    if (!editor) {
        perror("malloc failed");
        exit(1);
    }
    editor->original_text = malloc(sizeof(ArrayList));
    editor->added_text = malloc(sizeof(ArrayList));
    if (filename) {
        editor->filename = strdup(filename);

        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
            perror("open failed");
            exit(1);
        }

        struct stat st;
        if (fstat(fd, &st) < 0) {
            perror("fstat failed");
            exit(1);
        }

        editor->original_text->length = st.st_size;
        editor->original_text->capacity = st.st_size;
        editor->original_text->array = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

        editor->root = create_node(ORIGINAL, 0UL, st.st_size);

        close(fd);

    } else {
        editor->filename = "file.txt";

        editor->original_text->array = strdup(text);
        long len = (long)strlen(text);
        editor->original_text->capacity = len;
        editor->original_text->length = len;

        editor->root = create_node(ORIGINAL, 0UL, len);
    }

    editor->added_text->length = 0;
    editor->added_text->capacity = 32;
    editor->added_text->array = malloc(sizeof(char) * editor->added_text->capacity);

    editor->cursor_position = (long)global_cursor;
    // printf("\ntable created\n");
    return editor;
}

void advance_cursor(TextEditor *editor, int advance) {
    editor->cursor_position += advance;
    if (editor->cursor_position >= editor->original_text->length + editor->added_text->length + 1) {
        printf("cursor position exceeds original text\n");
    }
}

int show_global_cursor(TextEditor *editor) {
    return (int)editor->cursor_position;
}

int show_total_len(TextEditor *editor) {
    return (int)(editor->original_text->length + editor->added_text->length);
}

void add_text(TextEditor *editor, char *text) {
    long len = strlen(text);
    long array_position = editor->added_text->length;

    arraylist_add(editor->added_text, text, len);
    // print_tree_structure(editor->root);
    add_text_tree(&editor->root, text, len, array_position, editor->cursor_position);
    balance_tree(&editor->root);
    editor->cursor_position += len;
}

void delete_text(TextEditor *editor, int length) {
    long len = length;
    long low = editor->cursor_position - len;
    long high = editor->cursor_position;
    while (len) {
        len -= delete_text_tree(&editor->root, low, high + len - length);
        // print_tree_structure(editor->root);
        balance_tree(&editor->root);
    }
    editor->cursor_position -= (long)length;
}

char *extract_current_text(TextEditor *editor) {
    ArrayList a = malloc(sizeof(ArrayList));
    a->length = 0;
    a->capacity = 32;
    a->array = calloc(a->capacity, sizeof(char));
    extract_text_tree(a, editor->root, editor);
    char* s = a->array;
    free(a);
    return s;
}

void save_current_text(TextEditor *editor, const char *filename, char **text, int *global_cursor) {
    char *full_content = extract_current_text(editor);

    if (!full_content) {
        fprintf(stderr, "Error: Failed to extract text from editor.\n");
        exit(1);
    }

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file for saving");
        free(full_content);
        exit(1);
    }

    fputs(full_content, file);
    fclose(file);
    free(full_content);
}

void arraylist_add(ArrayList a, char* source, long text_length) {
    char *array = a->array;
    long length = a->length;
    long capacity = a->capacity;

    for (long i = 0; i < text_length; i++) {
        if (length == capacity) {
            capacity *= 2;
            char *aux = realloc(array, capacity);
            if (aux == NULL) {
                free(array);
                perror("realloc failed");
                exit(1);
            }
            array = aux;
        }
        array[length] = source[i];
        length++;
    }

    if (length == capacity) {
        capacity *= 2;
        char *aux = realloc(array, capacity);
        if (aux == NULL) {
            free(array);
            perror("realloc failed");
            exit(1);
        }
        array = aux;
    }
    array[length] = 0;

    a->array = array;
    a->length = length;
    a->capacity = capacity;
}

void extract_text_tree(ArrayList a, PieceTree root, TextEditor *editor) {
    if (!root)
        return;
    extract_text_tree(a, root->left, editor);
    if (root->data.source == ORIGINAL) {
        arraylist_add(a, editor->original_text->array + root->data.start, root->data.length);
    } else {
        arraylist_add(a, editor->added_text->array + root->data.start, root->data.length);
    }
    extract_text_tree(a, root->right, editor);
}

PieceTree create_node(PieceSource source, long start, long length) {
    if (length <= 0)
        return NULL;
    PieceTree node = malloc(sizeof(PieceNode));

    if (!node) {
        perror("malloc failed");
        exit(1);
    }
    node->data.source = source;
    node->data.start = start;
    node->data.length = length;
    node->left_child_length = 0;

    node->left = NULL;
    node->right = NULL;
    node->depth = 0;
    return node;
}

void add_text_tree(PieceTree *root, char *text, long length, long array_position, long cursor) {
    if (*root == NULL) {
        *root = create_node(ADD, 0UL, length);
        return;
    }

    long left_child_length = (*root)->left_child_length;
    long data_length = (*root)->data.length;
    long total_length = left_child_length + data_length;
    long data_start = (*root)->data.start;
    PieceSource data_source = (*root)->data.source;
    long split_position = cursor - left_child_length;

    if (cursor < left_child_length) {
        add_text_tree(&(*root)->left, text, length, array_position, cursor);
        (*root)->left_child_length += length;
        balance_tree(&(*root)->left);
        return;
    }
    if (cursor > total_length) {
        add_text_tree(&(*root)->right, text, length, array_position, cursor - total_length);
        balance_tree(&(*root)->right);
        return;
    }
    PieceTree root_node = create_node(ADD, array_position, length);
    PieceTree left_node = create_node(data_source, data_start, split_position);
    PieceTree right_node = create_node(data_source, data_start + split_position, data_length - split_position);

    root_node->left = (*root)->left;
    root_node->right = (*root)->right;
    free(*root);
    *root = root_node;
    add_to_right(&root_node->left, left_node);
    add_to_left(&root_node->right, right_node);
    root_node->left_child_length = cursor;
}

long delete_text_tree(PieceTree *root, long low, long high) {
    long data_length = (*root)->data.length;
    long left_child_length = (*root)->left_child_length;
    long total_length = data_length + left_child_length;
    long data_start = (*root)->data.start;
    PieceSource data_source = (*root)->data.source;
    long removed_length = 0;

    if (high <= left_child_length) {
        removed_length = delete_text_tree(&(*root)->left, low, high);
        (*root)->left_child_length -= removed_length;
        balance_tree(&(*root)->left);
        return removed_length;
    }
    if (low >= total_length) {
        removed_length = delete_text_tree(&(*root)->right, low - total_length, high - total_length);
        balance_tree(&(*root)->right);
        return removed_length;
    }
    PieceTree left_node = create_node(data_source, data_start,  low - left_child_length);
    PieceTree root_node = create_node(data_source, data_start + high - left_child_length, total_length - high);

    removed_length = (*root)->data.length;

    if (root_node) {
        root_node->left = (*root)->left;
        root_node->right = (*root)->right;
        long left_child_increment = 0;
        if (left_node) {
            left_child_increment = left_node->data.length;
            removed_length -= left_node->data.length;
        }
        root_node->left_child_length = (*root)->left_child_length + left_child_increment;
        free(*root);
        *root = root_node;
        add_to_right(&root_node->left, left_node);
        removed_length -= root_node->data.length;
    }
    else if (left_node) {
        left_node->left = (*root)->left;
        left_node->right = (*root)->right;
        left_node->left_child_length = (*root)->left_child_length;
        free(*root);
        *root = left_node;
        removed_length -= left_node->data.length;
    }
    else {
        if ((*root)->left) {
            PieceTree new_root = pop_rightmost_node(&(*root)->left);
            new_root->left = (*root)->left;
            new_root->right = (*root)->right;
            new_root->left_child_length = (*root)->left_child_length - new_root->data.length;
            free(*root);
            *root = new_root;
        }
        else {
            free(*root);
            *root = (*root)->right;
        }
    }
    return removed_length;
}

void update_depth(PieceTree root) {
    int right_depth1 = 0;
    int left_depth1 = 0;

    if (root->right)
        right_depth1 = root->right->depth;
    if (root->left)
        left_depth1 = root->left->depth;

    root->depth = 1 + max(right_depth1, left_depth1);
}

void balance_tree(PieceTree *root) {
    if (!*root)
        return;
    int right_depth = 0;
    int left_depth = 0;

    if ((*root)->right)
        right_depth = (*root)->right->depth;
    if ((*root)->left)
        left_depth = (*root)->left->depth;

    if (left_depth - right_depth > 1) {
        int left_right_depth = 0;
        int left_left_depth = 0;

        if ((*root)->left->right)
            left_right_depth = (*root)->left->right->depth;
        if ((*root)->left->left)
            left_left_depth = (*root)->left->left->depth;
        if (left_right_depth - left_left_depth > 0) {
            rotate_left(&(*root)->left);
            update_depth((*root)->left);
        }

        rotate_right(root);
    }

    if (right_depth - left_depth > 1) {
        int right_left_depth = 0;
        int right_right_depth = 0;

        if ((*root)->right->left)
            right_left_depth = (*root)->right->left->depth;
        if ((*root)->right->right)
            right_right_depth = (*root)->right->right->depth;
        if (right_right_depth - right_left_depth > 0) {
            rotate_right(&(*root)->right);
            update_depth((*root)->right);
        }

        rotate_left(root);
    }
    update_depth(*root);
}

void rotate_right(PieceTree *root) {
    PieceTree left_node = (*root)->left;
    (*root)->left = left_node->right;

    update_depth(*root);
    (*root)->left_child_length -= left_node->left_child_length + left_node->data.length;

    left_node->right = *root;
    *root = left_node;
}

void rotate_left(PieceTree *root) {
    PieceTree right_node = (*root)->right;
    (*root)->right = right_node->left;

    update_depth(*root);

    right_node->left = *root;
    *root = right_node;
    right_node->left_child_length += right_node->left->left_child_length + right_node->left->data.length;
}

void add_to_left(PieceTree *root, PieceTree node) {
    if (!*root) {
        *root = node;
        return;
    }
    add_to_left(&(*root)->left, node);
    long length_increment = node->left_child_length + node->data.length;
    (*root)->left_child_length += length_increment;
    balance_tree(root);
}

void add_to_right(PieceTree *root, PieceTree node) {
    if (!*root) {
        *root = node;
        return;
    }
    add_to_right(&(*root)->right, node);
    balance_tree(root);
}

PieceTree pop_rightmost_node(PieceTree *root) {
    if (!(*root)->right) {
        *root = (*root)->left;
        return *root;
    }
    if (!(*root)->right->right) {
        PieceTree right_node = (*root)->right;
        (*root)->right = (*root)->right->left;
        return right_node;
    }
    PieceTree return_value = pop_rightmost_node(&(*root)->right);
    balance_tree(root);
    return return_value;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}
