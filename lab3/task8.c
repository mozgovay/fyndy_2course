#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
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
    ST_ERR_NOT_DIVISIBLE,
    ST_ERR_UNKNOWN_CMD
} Status;

typedef struct {
    int coef;
    int deg;
} Monom;

typedef struct PolyNode {
    Monom m;
    struct PolyNode *next;
} PolyNode;

typedef struct {
    PolyNode *head;
} Poly;

static void poly_init(Poly *p) {
    if (p) p->head = NULL;
}

static void poly_free(Poly *p) {
    if (!p) return;
    PolyNode *c = p->head;
    while (c) {
        PolyNode *n = c->next;
        free(c);
        c = n;
    }
    p->head = NULL;
}

static bool poly_is_zero(const Poly *p) {
    return !p || !p->head;
}

static Status poly_copy(const Poly *src, Poly *dst) {
    if (!src || !dst) return ST_ERR_ARG;
    poly_free(dst);
    PolyNode *tail = NULL;
    for (PolyNode *c = src->head; c; c = c->next) {
        PolyNode *n = (PolyNode *)malloc(sizeof(PolyNode));
        if (!n) { poly_free(dst); return ST_ERR_ALLOC; }
        n->m = c->m;
        n->next = NULL;
        if (!tail) dst->head = n;
        else tail->next = n;
        tail = n;
    }
    return ST_OK;
}

static bool ll_to_int(long long v, int *out) {
    if (v < (long long)INT_MIN || v > (long long)INT_MAX) return false;
    *out = (int)v;
    return true;
}

static Status poly_push_sorted(Poly *p, int coef, int deg) {
    if (!p || deg < 0) return ST_ERR_ARG;
    if (coef == 0) return ST_OK;
    PolyNode *prev = NULL;
    PolyNode *cur = p->head;
    while (cur && cur->m.deg > deg) {
        prev = cur;
        cur = cur->next;
    }
    if (cur && cur->m.deg == deg) {
        long long s = (long long)cur->m.coef + (long long)coef;
        int v;
        if (!ll_to_int(s, &v)) return ST_ERR_OVERFLOW;
        if (v == 0) {
            if (prev) prev->next = cur->next;
            else p->head = cur->next;
            free(cur);
        } else {
            cur->m.coef = v;
        }
        return ST_OK;
    }
    PolyNode *n = (PolyNode *)malloc(sizeof(PolyNode));
    if (!n) return ST_ERR_ALLOC;
    n->m.coef = coef;
    n->m.deg = deg;
    n->next = cur;
    if (prev) prev->next = n;
    else p->head = n;
    return ST_OK;
}

static int poly_degree(const Poly *p) {
    return (p && p->head) ? p->head->m.deg : -1;
}

static Status poly_add(const Poly *a, const Poly *b, Poly *out) {
    if (!a || !b || !out) return ST_ERR_ARG;
    Poly t;
    poly_init(&t);
    for (PolyNode *c = a->head; c; c = c->next) {
        Status st = poly_push_sorted(&t, c->m.coef, c->m.deg);
        if (st != ST_OK) { poly_free(&t); return st; }
    }
    for (PolyNode *c = b->head; c; c = c->next) {
        Status st = poly_push_sorted(&t, c->m.coef, c->m.deg);
        if (st != ST_OK) { poly_free(&t); return st; }
    }
    poly_free(out);
    *out = t;
    return ST_OK;
}

static Status poly_neg(const Poly *a, Poly *out) {
    if (!a || !out) return ST_ERR_ARG;
    Poly t;
    poly_init(&t);
    for (PolyNode *c = a->head; c; c = c->next) {
        Status st = poly_push_sorted(&t, -c->m.coef, c->m.deg);
        if (st != ST_OK) { poly_free(&t); return st; }
    }
    poly_free(out);
    *out = t;
    return ST_OK;
}

static Status poly_sub(const Poly *a, const Poly *b, Poly *out) {
    Poly nb;
    poly_init(&nb);
    Status st = poly_neg(b, &nb);
    if (st != ST_OK) return st;
    st = poly_add(a, &nb, out);
    poly_free(&nb);
    return st;
}

static Status poly_mul(const Poly *a, const Poly *b, Poly *out) {
    if (!a || !b || !out) return ST_ERR_ARG;
    Poly t;
    poly_init(&t);
    for (PolyNode *ca = a->head; ca; ca = ca->next) {
        for (PolyNode *cb = b->head; cb; cb = cb->next) {
            long long dd = (long long)ca->m.deg + (long long)cb->m.deg;
            if (dd > (long long)INT_MAX) { poly_free(&t); return ST_ERR_OVERFLOW; }
            long long cc = (long long)ca->m.coef * (long long)cb->m.coef;
            int cv;
            if (!ll_to_int(cc, &cv)) { poly_free(&t); return ST_ERR_OVERFLOW; }
            Status st = poly_push_sorted(&t, cv, (int)dd);
            if (st != ST_OK) { poly_free(&t); return st; }
        }
    }
    poly_free(out);
    *out = t;
    return ST_OK;
}

static Status poly_divmod(const Poly *a, const Poly *b, Poly *quot, Poly *rem) {
    if (!a || !b || !quot || !rem) return ST_ERR_ARG;
    if (poly_is_zero(b)) return ST_ERR_ARG;

    Poly r;
    poly_init(&r);
    Status st = poly_copy(a, &r);
    if (st != ST_OK) return st;

    Poly q;
    poly_init(&q);

    int db = poly_degree(b);
    int cb = b->head->m.coef;

    while (!poly_is_zero(&r) && poly_degree(&r) >= db) {
        int dr = poly_degree(&r);
        int cr = r.head->m.coef;
        if (cr % cb != 0) {
            poly_free(&r);
            poly_free(&q);
            return ST_ERR_NOT_DIVISIBLE;
        }
        int qc = cr / cb;
        int qd = dr - db;

        st = poly_push_sorted(&q, qc, qd);
        if (st != ST_OK) { poly_free(&r); poly_free(&q); return st; }

        Poly term;
        poly_init(&term);
        for (PolyNode *c = b->head; c; c = c->next) {
            long long cc = (long long)qc * (long long)c->m.coef;
            int cv;
            if (!ll_to_int(cc, &cv)) { poly_free(&term); poly_free(&r); poly_free(&q); return ST_ERR_OVERFLOW; }
            long long dd = (long long)qd + (long long)c->m.deg;
            if (dd > (long long)INT_MAX) { poly_free(&term); poly_free(&r); poly_free(&q); return ST_ERR_OVERFLOW; }
            st = poly_push_sorted(&term, cv, (int)dd);
            if (st != ST_OK) { poly_free(&term); poly_free(&r); poly_free(&q); return st; }
        }

        Poly nr;
        poly_init(&nr);
        st = poly_sub(&r, &term, &nr);
        poly_free(&term);
        if (st != ST_OK) { poly_free(&nr); poly_free(&r); poly_free(&q); return st; }
        poly_free(&r);
        r = nr;
    }

    poly_free(quot);
    poly_free(rem);
    *quot = q;
    *rem = r;
    return ST_OK;
}

static Status poly_eval(const Poly *p, int x, int *out) {
    if (!p || !out) return ST_ERR_ARG;
    long long acc = 0;
    for (PolyNode *c = p->head; c; c = c->next) {
        long long v = acc;
        for (int i = 0; i < c->m.deg; ++i) {
            v *= (long long)x;
            if (v > (long long)INT_MAX * 8LL || v < (long long)INT_MIN * 8LL) return ST_ERR_OVERFLOW;
        }
        v += (long long)c->m.coef;
        if (v > (long long)INT_MAX * 8LL || v < (long long)INT_MIN * 8LL) return ST_ERR_OVERFLOW;
        acc = v;
    }
    int res;
    if (!ll_to_int(acc, &res)) return ST_ERR_OVERFLOW;
    *out = res;
    return ST_OK;
}

static Status poly_diff(const Poly *p, Poly *out) {
    if (!p || !out) return ST_ERR_ARG;
    Poly t;
    poly_init(&t);
    for (PolyNode *c = p->head; c; c = c->next) {
        if (c->m.deg == 0) continue;
        long long cc = (long long)c->m.coef * (long long)c->m.deg;
        int cv;
        if (!ll_to_int(cc, &cv)) { poly_free(&t); return ST_ERR_OVERFLOW; }
        Status st = poly_push_sorted(&t, cv, c->m.deg - 1);
        if (st != ST_OK) { poly_free(&t); return st; }
    }
    poly_free(out);
    *out = t;
    return ST_OK;
}

static Status poly_compose(const Poly *a, const Poly *b, Poly *out) {
    if (!a || !b || !out) return ST_ERR_ARG;
    Poly acc;
    poly_init(&acc);
    for (PolyNode *c = a->head; c; c = c->next) {
        Poly term;
        poly_init(&term);
        Status st = poly_push_sorted(&term, 1, 0);
        if (st != ST_OK) { poly_free(&term); poly_free(&acc); return st; }
        for (int i = 0; i < c->m.deg; ++i) {
            Poly tmp;
            poly_init(&tmp);
            st = poly_mul(&term, b, &tmp);
            poly_free(&term);
            if (st != ST_OK) { poly_free(&tmp); poly_free(&acc); return st; }
            term = tmp;
        }
        long long cc = (long long)c->m.coef;
        int cv;
        if (!ll_to_int(cc, &cv)) { poly_free(&term); poly_free(&acc); return ST_ERR_OVERFLOW; }
        for (PolyNode *q = term.head; q; q = q->next) {
            long long pv = (long long)q->m.coef * (long long)cv;
            int pvi;
            if (!ll_to_int(pv, &pvi)) { poly_free(&term); poly_free(&acc); return ST_ERR_OVERFLOW; }
            st = poly_push_sorted(&acc, pvi, q->m.deg);
            if (st != ST_OK) { poly_free(&term); poly_free(&acc); return st; }
        }
        poly_free(&term);
    }
    poly_free(out);
    *out = acc;
    return ST_OK;
}

static void poly_print(const Poly *p, FILE *f) {
    if (!f) return;
    if (poly_is_zero(p)) { fprintf(f, "0"); return; }
    bool first = true;
    for (PolyNode *c = p->head; c; c = c->next) {
        int coef = c->m.coef;
        int deg = c->m.deg;
        bool neg = coef < 0;
        int abs_c = neg ? -coef : coef;
        if (first) {
            if (neg) fprintf(f, "-");
        } else {
            fprintf(f, neg ? "-" : "+");
        }
        if (deg == 0) {
            fprintf(f, "%d", abs_c);
        } else {
            if (abs_c != 1) fprintf(f, "%d", abs_c);
            fprintf(f, "x");
            if (deg > 1) fprintf(f, "^%d", deg);
        }
        first = false;
    }
}

typedef struct {
    const char *s;
    size_t pos;
    size_t len;
} Parser;

static void skip_ws(Parser *p) {
    while (p->pos < p->len && isspace((unsigned char)p->s[p->pos])) p->pos += 1U;
}

static bool parse_uint(Parser *p, int *out) {
    if (p->pos >= p->len || !isdigit((unsigned char)p->s[p->pos])) return false;
    long long v = 0;
    while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) {
        v = v * 10 + (p->s[p->pos] - '0');
        if (v > INT_MAX) return false;
        p->pos += 1U;
    }
    *out = (int)v;
    return true;
}

static Status parse_poly(Parser *p, Poly *out);

static Status parse_monom(Parser *p, int sign, Poly *acc) {
    skip_ws(p);
    if (p->pos >= p->len) return ST_ERR_FMT;
    int coef = 1;
    bool have_coef = false;
    if (isdigit((unsigned char)p->s[p->pos])) {
        int v;
        if (!parse_uint(p, &v)) return ST_ERR_FMT;
        coef = v;
        have_coef = true;
    }
    skip_ws(p);
    int deg = 0;
    if (p->pos < p->len && p->s[p->pos] == 'x') {
        p->pos += 1U;
        deg = 1;
        skip_ws(p);
        if (p->pos < p->len && p->s[p->pos] == '^') {
            p->pos += 1U;
            skip_ws(p);
            if (!parse_uint(p, &deg)) return ST_ERR_FMT;
        }
    } else if (!have_coef) {
        return ST_ERR_FMT;
    }
    if (!have_coef && deg == 0) return ST_ERR_FMT;
    long long cc = (long long)coef * (long long)sign;
    int cv;
    if (!ll_to_int(cc, &cv)) return ST_ERR_OVERFLOW;
    return poly_push_sorted(acc, cv, deg);
}

static Status parse_poly(Parser *p, Poly *out) {
    if (!p || !out) return ST_ERR_ARG;
    Poly acc;
    poly_init(&acc);
    skip_ws(p);
    if (p->pos >= p->len) { poly_free(&acc); return ST_ERR_FMT; }
    int sign = 1;
    if (p->s[p->pos] == '+') { p->pos += 1U; }
    else if (p->s[p->pos] == '-') { sign = -1; p->pos += 1U; }
    Status st = parse_monom(p, sign, &acc);
    if (st != ST_OK) { poly_free(&acc); return st; }
    for (;;) {
        skip_ws(p);
        if (p->pos >= p->len) break;
        char c = p->s[p->pos];
        if (c != '+' && c != '-') break;
        sign = (c == '+') ? 1 : -1;
        p->pos += 1U;
        st = parse_monom(p, sign, &acc);
        if (st != ST_OK) { poly_free(&acc); return st; }
    }
    poly_free(out);
    *out = acc;
    return ST_OK;
}

static Status parse_int(Parser *p, int *out) {
    skip_ws(p);
    int sign = 1;
    if (p->pos < p->len && (p->s[p->pos] == '+' || p->s[p->pos] == '-')) {
        if (p->s[p->pos] == '-') sign = -1;
        p->pos += 1U;
        skip_ws(p);
    }
    int v;
    if (!parse_uint(p, &v)) return ST_ERR_FMT;
    long long r = (long long)sign * (long long)v;
    int ri;
    if (!ll_to_int(r, &ri)) return ST_ERR_OVERFLOW;
    *out = ri;
    return ST_OK;
}

typedef enum {
    CMD_ADD, CMD_SUB, CMD_MULT, CMD_DIV, CMD_MOD,
    CMD_EVAL, CMD_DIFF, CMD_CMPS, CMD_UNKNOWN
} CmdKind;

static CmdKind parse_cmd_name(const char *name) {
    if (strcmp(name, "Add") == 0) return CMD_ADD;
    if (strcmp(name, "Sub") == 0) return CMD_SUB;
    if (strcmp(name, "Mult") == 0) return CMD_MULT;
    if (strcmp(name, "Div") == 0) return CMD_DIV;
    if (strcmp(name, "Mod") == 0) return CMD_MOD;
    if (strcmp(name, "Eval") == 0) return CMD_EVAL;
    if (strcmp(name, "Diff") == 0) return CMD_DIFF;
    if (strcmp(name, "Cmps") == 0) return CMD_CMPS;
    return CMD_UNKNOWN;
}

static Status split_args(const char *inner, size_t len, char *a, size_t asz, char *b, size_t bsz, int *nargs) {
    if (!inner || !a || !b || !nargs) return ST_ERR_ARG;
    int depth = 0;
    size_t comma = (size_t)-1;
    for (size_t i = 0; i < len; ++i) {
        if (inner[i] == '(') depth += 1;
        else if (inner[i] == ')') depth -= 1;
        else if (inner[i] == ',' && depth == 0) {
            if (comma != (size_t)-1) return ST_ERR_FMT;
            comma = i;
        }
    }
    if (depth != 0) return ST_ERR_FMT;
    if (comma == (size_t)-1) {
        if (len + 1U > asz) return ST_ERR_OVERFLOW;
        memcpy(a, inner, len);
        a[len] = '\0';
        b[0] = '\0';
        *nargs = 1;
    } else {
        size_t la = comma;
        size_t lb = len - comma - 1U;
        if (la + 1U > asz || lb + 1U > bsz) return ST_ERR_OVERFLOW;
        memcpy(a, inner, la);
        a[la] = '\0';
        memcpy(b, inner + comma + 1U, lb);
        b[lb] = '\0';
        *nargs = 2;
    }
    return ST_OK;
}

static Status exec_binary(CmdKind k, const Poly *p, const Poly *q, Poly *res) {
    switch (k) {
        case CMD_ADD:  return poly_add(p, q, res);
        case CMD_SUB:  return poly_sub(p, q, res);
        case CMD_MULT: return poly_mul(p, q, res);
        case CMD_DIV: {
            Poly r;
            poly_init(&r);
            Status st = poly_divmod(p, q, res, &r);
            poly_free(&r);
            return st;
        }
        case CMD_MOD: {
            Poly qq;
            poly_init(&qq);
            Status st = poly_divmod(p, q, &qq, res);
            poly_free(&qq);
            return st;
        }
        case CMD_CMPS: return poly_compose(p, q, res);
        default: return ST_ERR_UNKNOWN_CMD;
    }
}

static Status parse_poly_arg(const char *s, Poly *out) {
    Parser p;
    p.s = s;
    p.pos = 0;
    p.len = strlen(s);
    skip_ws(&p);
    if (p.pos < p.len && p.s[p.pos] == '(') {
        p.pos += 1U;
        Poly inner;
        poly_init(&inner);
        Status st = parse_poly(&p, &inner);
        if (st != ST_OK) { poly_free(&inner); return st; }
        skip_ws(&p);
        if (p.pos >= p.len || p.s[p.pos] != ')') { poly_free(&inner); return ST_ERR_FMT; }
        p.pos += 1U;
        skip_ws(&p);
        if (p.pos != p.len) { poly_free(&inner); return ST_ERR_FMT; }
        poly_free(out);
        *out = inner;
        return ST_OK;
    }
    return parse_poly(&p, out);
}

static Status exec_instruction(const char *line, Poly *accum, FILE *out) {
    if (!line || !accum || !out) return ST_ERR_ARG;
    const char *lp = strchr(line, '(');
    if (!lp) return ST_ERR_FMT;
    const char *rp = strrchr(line, ')');
    if (!rp || rp < lp) return ST_ERR_FMT;
    for (const char *q = rp + 1; *q; ++q)
        if (!isspace((unsigned char)*q)) return ST_ERR_FMT;
    char name[16];
    size_t nl = (size_t)(lp - line);
    while (nl > 0U && isspace((unsigned char)line[nl - 1U])) nl -= 1U;
    if (nl == 0U || nl >= sizeof(name)) return ST_ERR_FMT;
    memcpy(name, line, nl);
    name[nl] = '\0';
    CmdKind kind = parse_cmd_name(name);
    if (kind == CMD_UNKNOWN) return ST_ERR_UNKNOWN_CMD;

    size_t inner_len = (size_t)(rp - lp - 1);
    char a[512], b[512];
    int nargs = 0;
    Status st = split_args(lp + 1, inner_len, a, sizeof(a), b, sizeof(b), &nargs);
    if (st != ST_OK) return st;

    if (kind == CMD_EVAL) {
        const char *src = (nargs == 1) ? a : a;
        if (nargs != 1) return ST_ERR_FMT;
        int x;
        Parser pp;
        pp.s = src; pp.pos = 0; pp.len = strlen(src);
        st = parse_int(&pp, &x);
        if (st != ST_OK) return st;
        skip_ws(&pp);
        if (pp.pos != pp.len) return ST_ERR_FMT;
        int v;
        st = poly_eval(accum, x, &v);
        if (st != ST_OK) return st;
        fprintf(out, "%d\n", v);
        return ST_OK;
    }

    if (kind == CMD_DIFF) {
        if (nargs != 1) return ST_ERR_FMT;
        Poly arg;
        poly_init(&arg);
        st = parse_poly_arg(a, &arg);
        if (st != ST_OK) { poly_free(&arg); return st; }
        Poly res;
        poly_init(&res);
        st = poly_diff(&arg, &res);
        poly_free(&arg);
        if (st != ST_OK) { poly_free(&res); return st; }
        poly_free(accum);
        *accum = res;
        poly_print(accum, out);
        fprintf(out, "\n");
        return ST_OK;
    }

    Poly p, q, res;
    poly_init(&p);
    poly_init(&q);
    poly_init(&res);

    if (nargs == 1) {
        st = poly_copy(accum, &p);
        if (st == ST_OK) st = parse_poly_arg(a, &q);
    } else if (nargs == 2) {
        st = parse_poly_arg(a, &p);
        if (st == ST_OK) st = parse_poly_arg(b, &q);
    } else {
        st = ST_ERR_FMT;
    }

    if (st == ST_OK) st = exec_binary(kind, &p, &q, &res);
    poly_free(&p);
    poly_free(&q);
    if (st != ST_OK) { poly_free(&res); return st; }
    poly_free(accum);
    *accum = res;
    poly_print(accum, out);
    fprintf(out, "\n");
    return ST_OK;
}

static char *read_whole_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)sz + 1U);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1U, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

static Status process_file(const char *path, FILE *out) {
    if (!path || !out) return ST_ERR_ARG;
    size_t len = 0;
    char *src = read_whole_file(path, &len);
    if (!src) return ST_ERR_IO;

    Poly accum;
    poly_init(&accum);

    size_t i = 0;
    bool in_block = false;
    char line[1024];
    size_t ll = 0;

    while (i <= len) {
        char c = (i < len) ? src[i] : '\n';
        if (!in_block && c == '%') {
            if (ll > 0U) {
                line[ll] = '\0';
                Status st = exec_instruction(line, &accum, out);
                if (st != ST_OK) fprintf(out, "error %d\n", (int)st);
                ll = 0U;
            }
            while (i < len && src[i] != '\n') i += 1U;
            if (i < len) i += 1U;
            continue;
        }
        if (!in_block && c == '[') { in_block = true; i += 1U; continue; }
        if (in_block) {
            if (c == ']') { in_block = false; i += 1U; continue; }
            i += 1U;
            continue;
        }
        if (c == '\n' || i == len) {
            if (ll > 0U) {
                line[ll] = '\0';
                size_t k = 0;
                while (k < ll && isspace((unsigned char)line[k])) k += 1U;
                if (k < ll) {
                    Status st = exec_instruction(line + k, &accum, out);
                    if (st != ST_OK) fprintf(out, "error %d\n", (int)st);
                }
                ll = 0U;
            }
            i += 1U;
            continue;
        }
        if (ll + 1U < sizeof(line)) line[ll++] = c;
        i += 1U;
    }

    poly_free(&accum);
    free(src);
    return ST_OK;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s file.txt\n", argv[0]);
        return 1;
    }
    Status st = process_file(argv[1], stdout);
    if (st != ST_OK) {
        fprintf(stderr, "error %d\n", (int)st);
        return 1;
    }
    return 0;
}
