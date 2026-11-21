#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ALPHABET_SIZE 256
#define MAX_CODE_LEN 256

typedef enum {
    STATUS_OK = 0,
    STATUS_INVALID_ARGUMENTS,
    STATUS_IO_ERROR,
    STATUS_MEMORY_ERROR
} Status;

typedef struct Node {
    uint64_t freq;
    int byte;
    int is_leaf;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    Node **data;
    size_t size;
    size_t capacity;
} MinHeap;

static void swap_nodes(Node **a, Node **b) {
    Node *temp = *a;
    *a = *b;
    *b = temp;
}

static Status heap_init(MinHeap *heap, size_t capacity) {
    heap->data = malloc(capacity * sizeof(Node *));
    if (!heap->data) return STATUS_MEMORY_ERROR;
    heap->size = 0;
    heap->capacity = capacity;
    return STATUS_OK;
}

static void heap_free(MinHeap *heap) {
    if (heap && heap->data) {
        free(heap->data);
        heap->data = NULL;
    }
}

static void sift_up(MinHeap *heap, size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (heap->data[index]->freq >= heap->data[parent]->freq) {
            break;
        }
        swap_nodes(&heap->data[index], &heap->data[parent]);
        index = parent;
    }
}

static void sift_down(MinHeap *heap, size_t index) {
    while (1) {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t smallest = index;

        if (left < heap->size && heap->data[left]->freq < heap->data[smallest]->freq) {
            smallest = left;
        }
        if (right < heap->size && heap->data[right]->freq < heap->data[smallest]->freq) {
            smallest = right;
        }
        if (smallest == index) break;

        swap_nodes(&heap->data[index], &heap->data[smallest]);
        index = smallest;
    }
}

static Status heap_push(MinHeap *heap, Node *node) {
    if (heap->size >= heap->capacity) return STATUS_MEMORY_ERROR;
    heap->data[heap->size] = node;
    heap->size++;
    sift_up(heap, heap->size - 1);
    return STATUS_OK;
}

static Node* heap_pop(MinHeap *heap) {
    if (heap->size == 0) return NULL;
    Node *root = heap->data[0];
    heap->size--;
    if (heap->size > 0) {
        heap->data[0] = heap->data[heap->size];
        sift_down(heap, 0);
    }
    return root;
}

static void free_huffman_tree(Node *root) {
    if (root == NULL) return;
    free_huffman_tree(root->left);
    free_huffman_tree(root->right);
    free(root);
}

static void generate_codes_recursive(Node *node, char *current_code, int depth, char codes[ALPHABET_SIZE][MAX_CODE_LEN]) {
    if (node->is_leaf) {
        current_code[depth] = '\0';
        if (depth == 0) {
            strcpy(codes[node->byte], "0");
        } else {
            strcpy(codes[node->byte], current_code);
        }
        return;
    }
    if (node->left) {
        current_code[depth] = '0';
        generate_codes_recursive(node->left, current_code, depth + 1, codes);
    }
    if (node->right) {
        current_code[depth] = '1';
        generate_codes_recursive(node->right, current_code, depth + 1, codes);
    }
}

static Status read_file_bytes(const char *path, uint64_t freq[ALPHABET_SIZE]) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return STATUS_IO_ERROR;
    }

    unsigned char buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            freq[buffer[i]]++;
        }
    }

    if (ferror(file)) {
        fclose(file);
        return STATUS_IO_ERROR;
    }

    fclose(file);
    return STATUS_OK;
}

static Status build_tree_and_codes(uint64_t freq[ALPHABET_SIZE], Node **root, char codes[ALPHABET_SIZE][MAX_CODE_LEN]) {
    MinHeap heap;
    size_t unique_count = 0;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (freq[i] > 0) unique_count++;
    }

    if (unique_count == 0) {
        *root = NULL;
        return STATUS_OK;
    }

    Status status = heap_init(&heap, unique_count);
    if (status != STATUS_OK) return status;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (freq[i] > 0) {
            Node *node = malloc(sizeof(Node));
            if (!node) {
                heap_free(&heap);
                return STATUS_MEMORY_ERROR;
            }
            node->freq = freq[i];
            node->byte = i;
            node->is_leaf = 1;
            node->left = NULL;
            node->right = NULL;
            
            status = heap_push(&heap, node);
            if (status != STATUS_OK) {
                free(node);
                heap_free(&heap);
                return status;
            }
        }
    }

    while (heap.size > 1) {
        Node *left = heap_pop(&heap);
        Node *right = heap_pop(&heap);

        Node *parent = malloc(sizeof(Node));
        if (!parent) {
            free_huffman_tree(left);
            free_huffman_tree(right);
            heap_free(&heap);
            return STATUS_MEMORY_ERROR;
        }

        parent->freq = left->freq + right->freq;
        parent->byte = -1;
        parent->is_leaf = 0;
        parent->left = left;
        parent->right = right;

        status = heap_push(&heap, parent);
        if (status != STATUS_OK) {
            free_huffman_tree(parent);
            heap_free(&heap);
            return status;
        }
    }

    *root = heap_pop(&heap);
    heap_free(&heap);

    if (*root) {
        char current_code[MAX_CODE_LEN];
        generate_codes_recursive(*root, current_code, 0, codes);
    }

    return STATUS_OK;
}

static void print_codes_table(const uint64_t freq[ALPHABET_SIZE], char codes[ALPHABET_SIZE][MAX_CODE_LEN]) {
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (freq[i] > 0) {
            printf("%d\t%llu\t%s\n", i, (unsigned long long)freq[i], codes[i]);
        }
    }
}

static void calculate_and_print_metrics(const uint64_t freq[ALPHABET_SIZE], char codes[ALPHABET_SIZE][MAX_CODE_LEN]) {
    uint64_t total_bytes = 0;
    uint64_t total_bits = 0;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (freq[i] > 0) {
            total_bytes += freq[i];
            total_bits += freq[i] * strlen(codes[i]);
        }
    }

    if (total_bytes == 0) {
        printf("0\n0\n0\n0.00%%\n");
        return;
    }

    uint64_t compressed_bytes = (total_bits + 7) / 8;
    double saving = 100.0 * (1.0 - (double)compressed_bytes / (double)total_bytes);

    printf("%llu\n", (unsigned long long)total_bytes);
    printf("%llu\n", (unsigned long long)total_bits);
    printf("%llu\n", (unsigned long long)compressed_bytes);
    printf("%.2f%%\n", saving);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    uint64_t freq[ALPHABET_SIZE] = {0};
    char codes[ALPHABET_SIZE][MAX_CODE_LEN] = {{0}};
    Node *root = NULL;

    Status status = read_file_bytes(argv[1], freq);
    if (status != STATUS_OK) {
        fprintf(stderr, "Error reading file\n");
        return 1;
    }

    status = build_tree_and_codes(freq, &root, codes);
    if (status != STATUS_OK) {
        fprintf(stderr, "Error building Huffman tree\n");
        return 1;
    }

    print_codes_table(freq, codes);
    calculate_and_print_metrics(freq, codes);

    free_huffman_tree(root);
    return 0;
}
