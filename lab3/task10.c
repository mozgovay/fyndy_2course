#include <ctype.h>
#include <errno.h>
#include <limits.h>
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
    ST_ERR_IO
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

typedef struct Node {
    String name;
    struct Node **children;
    size_t n_children;
    size_t cap;
} Node;

static void node_free(Node *n) {
    if (!n) return;
    for (size_t i = 0; i < n->n_children; ++i) node_free(n->children[i]);
    free(n->children);
    str_free(&n->name);
    free(n);
}

static Node *node_new(const char *name) {
    if (!name || !*name) return NULL;
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) return NULL;
    memset(n, 0, sizeof(*n));
    if (str_init(&n->name, name) != ST_OK) { free(n); return NULL; }
    return n;
}

static Status node_add_child(Node *parent, Node *child) {
    if (!parent || !child) return ST_ERR_ARG;
    if (parent->n_children == parent->cap) {
        size_t ncap = parent->cap ? parent->cap * 2U : 4U;
        if (ncap > SIZE_MAX / sizeof(Node *)) return ST_ERR_OVERFLOW;
        Node **np = (Node **)realloc(parent->children, ncap * sizeof(Node *));
        if (!np) return ST_ERR_ALLOC;
        parent->children = np;
        parent->cap = ncap;
    }
    parent->children[parent->n_children++] = child;
    return ST_OK;
}

typedef struct {
    const char *s;
    size_t pos;
    size_t len;
} Parser;

static void skip_ws(Parser *p) {
    while (p->pos < p->len && isspace((unsigned char)p->s[p->pos])) p->pos += 1U;
}

static Status parse_node(Parser *p, Node **out);

static Status parse_list(Parser *p, Node *parent) {
    skip_ws(p);
    if (p->pos >= p->len) return ST_ERR_FMT;
    if (p->s[p->pos] == ')') return ST_OK;
    for (;;) {
        Node *child = NULL;
        Status st = parse_node(p, &child);
        if (st != ST_OK) return st;
        st = node_add_child(parent, child);
        if (st != ST_OK) { node_free(child); return st; }
        skip_ws(p);
        if (p->pos >= p->len) return ST_ERR_FMT;
        if (p->s[p->pos] == ',') { p->pos += 1U; continue; }
        if (p->s[p->pos] == ')') return ST_OK;
        return ST_ERR_FMT;
    }
}

static Status parse_node(Parser *p, Node **out) {
    if (!p || !out) return ST_ERR_ARG;
    skip_ws(p);
    size_t start = p->pos;
    while (p->pos < p->len &&
           !isspace((unsigned char)p->s[p->pos]) &&
           p->s[p->pos] != '(' &&
           p->s[p->pos] != ')' &&
           p->s[p->pos] != ',') {
        p->pos += 1U;
    }
    size_t name_len = p->pos - start;
    if (name_len == 0U) return ST_ERR_FMT;
    char *name = (char *)malloc(name_len + 1U);
    if (!name) return ST_ERR_ALLOC;
    memcpy(name, p->s + start, name_len);
    name[name_len] = '\0';
    Node *n = node_new(name);
    free(name);
    if (!n) return ST_ERR_ALLOC;

    skip_ws(p);
    if (p->pos < p->len && p->s[p->pos] == '(') {
        p->pos += 1U;
        Status st = parse_list(p, n);
        if (st != ST_OK) { node_free(n); return st; }
        skip_ws(p);
        if (p->pos >= p->len || p->s[p->pos] != ')') { node_free(n); return ST_ERR_FMT; }
        p->pos += 1U;
    }
    *out = n;
    return ST_OK;
}

static Status parse_expr(const char *line, Node **out) {
    if (!line || !out) return ST_ERR_ARG;
    Parser p;
    p.s = line;
    p.pos = 0;
    p.len = strlen(line);
    skip_ws(&p);
    if (p.pos >= p.len) return ST_ERR_FMT;
    Status st = parse_node(&p, out);
    if (st != ST_OK) return st;
    skip_ws(&p);
    if (p.pos != p.len) { node_free(*out); *out = NULL; return ST_ERR_FMT; }
    return ST_OK;
}

static void print_indent(FILE *f, int depth) {
    for (int i = 0; i < depth; ++i) fputs("  ", f);
}

static void print_tree_rec(FILE *f, const Node *n, int depth) {
    if (!n) return;
    print_indent(f, depth);
    fputs(n->name.data, f);
    fputc('\n', f);
    for (size_t i = 0; i < n->n_children; ++i)
        print_tree_rec(f, n->children[i], depth + 1);
}

static void print_bracket_rec(FILE *f, const Node *n) {
    if (!n) return;
    fputs(n->name.data, f);
    if (n->n_children > 0U) {
        fputc('(', f);
        for (size_t i = 0; i < n->n_children; ++i) {
            if (i > 0U) fputc(',', f);
            print_bracket_rec(f, n->children[i]);
        }
        fputc(')', f);
    }
}

static Status process_file(const char *in_path, const char *out_path) {
    if (!in_path || !out_path) return ST_ERR_ARG;
    FILE *in = fopen(in_path, "r");
    if (!in) return ST_ERR_IO;
    FILE *out = fopen(out_path, "w");
    if (!out) { fclose(in); return ST_ERR_IO; }

    char line[8192];
    size_t idx = 0;
    Status rc = ST_OK;
    while (fgets(line, sizeof(line), in)) {
        size_t l = strlen(line);
        while (l > 0U && (line[l - 1U] == '\n' || line[l - 1U] == '\r')) line[--l] = '\0';
        size_t k = 0;
        while (k < l && isspace((unsigned char)line[k])) k += 1U;
        if (k == l) continue;
        idx += 1U;
        fprintf(out, "=== %zu: %s ===\n", idx, line + k);
        Node *root = NULL;
        Status st = parse_expr(line + k, &root);
        if (st != ST_OK) {
            fprintf(out, "error %d\n\n", (int)st);
            rc = st;
            continue;
        }
        print_tree_rec(out, root, 0);
        fputc('\n', out);
        print_bracket_rec(out, root);
        fputs("\n\n", out);
        node_free(root);
    }
    fclose(in);
    if (fclose(out) != 0) return ST_ERR_IO;
    return rc;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <input> <output>\n", argv[0]);
        return 1;
    }
    Status st = process_file(argv[1], argv[2]);
    if (st != ST_OK) {
        fprintf(stderr, "error %d\n", (int)st);
        return 1;
    }
    return 0;
}
