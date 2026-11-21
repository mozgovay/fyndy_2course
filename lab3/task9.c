#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ST_OK = 0,
    ST_ERR_ARG,
    ST_ERR_ALLOC,
    ST_ERR_OVERFLOW,
    ST_ERR_FMT,
    ST_ERR_IO,
    ST_ERR_NOT_FOUND,
    ST_ERR_EMPTY
} Status;

typedef struct {
    char *data;
    int length;
} String;

static Status str_init(String *s, const char *cstr) {
    if (!s || !cstr) return ST_ERR_ARG;
    size_t n = strlen(cstr);
    if (n > (size_t)INT_MAX) return ST_ERR_OVERFLOW;
    char *p = (char *)malloc(n + 1U);
    if (!p) return ST_ERR_ALLOC;
    memcpy(p, cstr, n + 1U);
    s->data = p;
    s->length = (int)n;
    return ST_OK;
}

static void str_free(String *s) {
    if (!s) return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

static int str_cmp(const String *a, const String *b) {
    int n = a->length < b->length ? a->length : b->length;
    int r = memcmp(a->data, b->data, (size_t)n);
    if (r != 0) return r < 0 ? -1 : 1;
    if (a->length == b->length) return 0;
    return a->length < b->length ? -1 : 1;
}

typedef struct Node {
    String word;
    long count;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    Node *root;
    size_t n_words;
} Tree;

static void tree_init(Tree *t) {
    if (!t) return;
    t->root = NULL;
    t->n_words = 0U;
}

static void tree_free_node(Node *n) {
    if (!n) return;
    tree_free_node(n->left);
    tree_free_node(n->right);
    str_free(&n->word);
    free(n);
}

static void tree_free(Tree *t) {
    if (!t) return;
    tree_free_node(t->root);
    t->root = NULL;
    t->n_words = 0U;
}

static Status tree_insert(Tree *t, const char *word) {
    if (!t || !word) return ST_ERR_ARG;
    Node **link = &t->root;
    while (*link) {
        int c = strcmp((*link)->word.data, word);
        if (c == 0) {
            (*link)->count += 1;
            return ST_OK;
        }
        link = (c > 0) ? &(*link)->left : &(*link)->right;
    }
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) return ST_ERR_ALLOC;
    memset(n, 0, sizeof(*n));
    Status st = str_init(&n->word, word);
    if (st != ST_OK) { free(n); return st; }
    n->count = 1;
    *link = n;
    t->n_words += 1U;
    return ST_OK;
}

static Node *tree_find(const Tree *t, const char *word) {
    if (!t || !word) return NULL;
    Node *c = t->root;
    while (c) {
        int r = strcmp(c->word.data, word);
        if (r == 0) return c;
        c = (r > 0) ? c->left : c->right;
    }
    return NULL;
}

static int tree_depth_node(const Node *n) {
    if (!n) return 0;
    int l = tree_depth_node(n->left);
    int r = tree_depth_node(n->right);
    return 1 + (l > r ? l : r);
}

static int tree_depth(const Tree *t) {
    return t ? tree_depth_node(t->root) : 0;
}

static bool is_separator(char c, const char *seps, size_t nseps) {
    for (size_t i = 0; i < nseps; ++i)
        if (seps[i] == c) return true;
    return false;
}

static Status process_file(Tree *t, const char *path, const char *seps, size_t nseps) {
    if (!t || !path || !seps || nseps == 0U) return ST_ERR_ARG;
    FILE *f = fopen(path, "rb");
    if (!f) return ST_ERR_IO;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return ST_ERR_IO; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return ST_ERR_IO; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return ST_ERR_IO; }
    char *buf = (char *)malloc((size_t)sz + 1U);
    if (!buf) { fclose(f); return ST_ERR_ALLOC; }
    size_t rd = fread(buf, 1U, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';

    size_t start = 0;
    bool in_word = false;
    char wordbuf[4096];
    size_t wl = 0;
    Status st = ST_OK;
    for (size_t i = 0; i <= rd; ++i) {
        char c = (i < rd) ? buf[i] : '\0';
        bool sep = (i == rd) || is_separator(c, seps, nseps);
        if (sep) {
            if (in_word) {
                wordbuf[wl] = '\0';
                st = tree_insert(t, wordbuf);
                if (st != ST_OK) { free(buf); return st; }
                wl = 0U;
                in_word = false;
            }
            start = i + 1U;
        } else {
            if (wl + 1U >= sizeof(wordbuf)) { free(buf); return ST_ERR_OVERFLOW; }
            wordbuf[wl++] = c;
            in_word = true;
        }
        (void)start;
    }
    free(buf);
    return ST_OK;
}

static void inorder_collect(const Node *n, Node **arr, size_t *idx) {
    if (!n) return;
    inorder_collect(n->left, arr, idx);
    arr[(*idx)++] = (Node *)n;
    inorder_collect(n->right, arr, idx);
}

static int cmp_by_count_desc(const void *pa, const void *pb) {
    const Node *a = *(const Node * const *)pa;
    const Node *b = *(const Node * const *)pb;
    if (a->count != b->count) return (a->count < b->count) ? 1 : -1;
    return strcmp(a->word.data, b->word.data);
}

static int cmp_by_len_asc(const void *pa, const void *pb) {
    const Node *a = *(const Node * const *)pa;
    const Node *b = *(const Node * const *)pb;
    if (a->word.length != b->word.length)
        return (a->word.length < b->word.length) ? -1 : 1;
    return strcmp(a->word.data, b->word.data);
}

static Status tree_to_array(const Tree *t, Node ***out, size_t *out_n) {
    if (!t || !out || !out_n) return ST_ERR_ARG;
    *out = NULL;
    *out_n = 0U;
    if (t->n_words == 0U) return ST_OK;
    Node **arr = (Node **)malloc(t->n_words * sizeof(Node *));
    if (!arr) return ST_ERR_ALLOC;
    size_t idx = 0U;
    inorder_collect(t->root, arr, &idx);
    *out = arr;
    *out_n = idx;
    return ST_OK;
}

static Status save_tree_rec(FILE *f, const Node *n) {
    if (!n) {
        if (fprintf(f, "#\n") < 0) return ST_ERR_IO;
        return ST_OK;
    }
    if (fprintf(f, "%s\t%ld\n", n->word.data, n->count) < 0) return ST_ERR_IO;
    Status st = save_tree_rec(f, n->left);
    if (st != ST_OK) return st;
    return save_tree_rec(f, n->right);
}

static Status save_tree(const Tree *t, const char *path) {
    if (!t || !path) return ST_ERR_ARG;
    FILE *f = fopen(path, "wb");
    if (!f) return ST_ERR_IO;
    Status st = save_tree_rec(f, t->root);
    fclose(f);
    return st;
}

static Status load_tree_rec(FILE *f, Node **out) {
    char line[8192];
    if (!fgets(line, sizeof(line), f)) return ST_ERR_IO;
    size_t l = strlen(line);
    while (l > 0U && (line[l - 1U] == '\n' || line[l - 1U] == '\r')) line[--l] = '\0';
    if (l == 1U && line[0] == '#') { *out = NULL; return ST_OK; }
    char *tab = strchr(line, '\t');
    if (!tab) return ST_ERR_FMT;
    *tab = '\0';
    char *num = tab + 1;
    char *end = NULL;
    errno = 0;
    long cnt = strtol(num, &end, 10);
    if (errno || end == num || *end || cnt < 0) return ST_ERR_FMT;
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) return ST_ERR_ALLOC;
    memset(n, 0, sizeof(*n));
    Status st = str_init(&n->word, line);
    if (st != ST_OK) { free(n); return st; }
    n->count = cnt;
    st = load_tree_rec(f, &n->left);
    if (st == ST_OK) st = load_tree_rec(f, &n->right);
    if (st != ST_OK) { tree_free_node(n); return st; }
    *out = n;
    return ST_OK;
}

static Status load_tree(Tree *t, const char *path) {
    if (!t || !path) return ST_ERR_ARG;
    FILE *f = fopen(path, "rb");
    if (!f) return ST_ERR_IO;
    Node *root = NULL;
    Status st = load_tree_rec(f, &root);
    fclose(f);
    if (st != ST_OK) { tree_free_node(root); return st; }
    tree_free(t);
    t->root = root;
    t->n_words = 0U;
    if (root) {
        size_t idx = 0U;
        Node **tmp = NULL;
        (void)tmp;
        Node *stack[8192];
        size_t sp = 0U;
        Node *c = root;
        while (c || sp > 0U) {
            while (c) { if (sp < 8192U) stack[sp++] = c; c = c->left; }
            if (sp == 0U) break;
            c = stack[--sp];
            idx += 1U;
            c = c->right;
        }
        t->n_words = idx;
    }
    return ST_OK;
}

static void print_top(const Tree *t, int n) {
    Node **arr = NULL;
    size_t cnt = 0U;
    Status st = tree_to_array(t, &arr, &cnt);
    if (st != ST_OK) { printf("error %d\n", (int)st); return; }
    if (cnt == 0U) { printf("(empty)\n"); free(arr); return; }
    qsort(arr, cnt, sizeof(Node *), cmp_by_count_desc);
    size_t lim = (n > 0 && (size_t)n < cnt) ? (size_t)n : cnt;
    for (size_t i = 0; i < lim; ++i)
        printf("%s %ld\n", arr[i]->word.data, arr[i]->count);
    free(arr);
}

static void print_longest(const Tree *t) {
    Node **arr = NULL;
    size_t cnt = 0U;
    Status st = tree_to_array(t, &arr, &cnt);
    if (st != ST_OK) { printf("error %d\n", (int)st); return; }
    if (cnt == 0U) { printf("(empty)\n"); free(arr); return; }
    qsort(arr, cnt, sizeof(Node *), cmp_by_len_asc);
    printf("%s\n", arr[cnt - 1U]->word.data);
    free(arr);
}

static void print_shortest(const Tree *t) {
    Node **arr = NULL;
    size_t cnt = 0U;
    Status st = tree_to_array(t, &arr, &cnt);
    if (st != ST_OK) { printf("error %d\n", (int)st); return; }
    if (cnt == 0U) { printf("(empty)\n"); free(arr); return; }
    qsort(arr, cnt, sizeof(Node *), cmp_by_len_asc);
    printf("%s\n", arr[0]->word.data);
    free(arr);
}

static void print_all(const Tree *t) {
    Node **arr = NULL;
    size_t cnt = 0U;
    Status st = tree_to_array(t, &arr, &cnt);
    if (st != ST_OK) { printf("error %d\n", (int)st); return; }
    for (size_t i = 0; i < cnt; ++i)
        printf("%s %ld\n", arr[i]->word.data, arr[i]->count);
    free(arr);
}

static bool read_line(char *buf, size_t sz) {
    if (!buf || sz < 2U || !fgets(buf, (int)sz, stdin)) return false;
    size_t l = strlen(buf);
    while (l > 0U && (buf[l - 1U] == '\n' || buf[l - 1U] == '\r')) buf[--l] = '\0';
    return true;
}

static bool read_int(int *v) {
    char b[32], *e = NULL;
    if (!read_line(b, sizeof(b)) || !b[0]) return false;
    errno = 0;
    long x = strtol(b, &e, 10);
    if (errno || e == b || *e || x < 0 || x > INT_MAX) return false;
    *v = (int)x;
    return true;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <file> <sep1> [sep2 ...]\n", argv[0]);
        return 1;
    }
    const char *path = argv[1];
    size_t nseps = (size_t)(argc - 2);
    char *seps = (char *)malloc(nseps);
    if (!seps) { fprintf(stderr, "alloc\n"); return 1; }
    for (size_t i = 0; i < nseps; ++i) {
        if (strlen(argv[i + 2U]) != 1U) {
            fprintf(stderr, "separator must be single char: %s\n", argv[i + 2U]);
            free(seps);
            return 1;
        }
        seps[i] = argv[i + 2U][0];
    }

    Tree t;
    tree_init(&t);
    Status st = process_file(&t, path, seps, nseps);
    free(seps);
    if (st != ST_OK) {
        fprintf(stderr, "file error %d\n", (int)st);
        tree_free(&t);
        return 1;
    }

    bool run = true;
    while (run) {
        printf("1:count 2:top 3:longest 4:shortest 5:depth 6:all 7:save 8:load 0:exit\n");
        int cmd = -1;
        if (!read_int(&cmd)) break;
        switch (cmd) {
            case 0: run = false; break;
            case 1: {
                char word[4096];
                printf("word: ");
                if (!read_line(word, sizeof(word))) { printf("(input error)\n"); break; }
                Node *n = tree_find(&t, word);
                if (n) printf("%ld\n", n->count);
                else printf("0\n");
                break;
            }
            case 2: {
                int n = 0;
                printf("n: ");
                if (!read_int(&n)) { printf("(input error)\n"); break; }
                print_top(&t, n);
                break;
            }
            case 3: print_longest(&t); break;
            case 4: print_shortest(&t); break;
            case 5: printf("%d\n", tree_depth(&t)); break;
            case 6: print_all(&t); break;
            case 7: {
                char p[512];
                printf("path: ");
                if (!read_line(p, sizeof(p))) { printf("(input error)\n"); break; }
                Status s = save_tree(&t, p);
                printf("%d\n", (int)s);
                break;
            }
            case 8: {
                char p[512];
                printf("path: ");
                if (!read_line(p, sizeof(p))) { printf("(input error)\n"); break; }
                Status s = load_tree(&t, p);
                printf("%d\n", (int)s);
                break;
            }
            default: printf("unknown\n"); break;
        }
    }

    tree_free(&t);
    return 0;
}
