#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    ST_OK = 0,
    ST_ERR_ARG,
    ST_ERR_ALLOC,
    ST_ERR_OVERFLOW,
    ST_ERR_NOT_FOUND,
    ST_ERR_DUP,
    ST_ERR_FMT
} Status;

typedef struct {
    char *data;
    int length;
} String;

typedef struct {
    String city;
    String street;
    int house;
    String building;
    int apartment;
    String postal_code;
} Address;

typedef struct {
    Address recipient;
    double weight;
    String mail_id;
    String created_at;
    String delivered_at;
} Mail;

typedef struct {
    Address *office_address;
    Mail *items;
    size_t count;
    size_t cap;
} Post;

Status str_init(String *s, const char *cstr) {
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

Status str_free(String *s) {
    if (!s) return ST_ERR_ARG;
    free(s->data);
    s->data = NULL;
    s->length = 0;
    return ST_OK;
}

Status str_cmp_order(const String *a, const String *b, int *out) {
    if (!a || !b || !out || !a->data || !b->data || a->length < 0 || b->length < 0) return ST_ERR_ARG;
    if (a->length != b->length) {
        *out = (a->length < b->length) ? -1 : 1;
        return ST_OK;
    }
    int r = memcmp(a->data, b->data, (size_t)a->length);
    *out = (r < 0) ? -1 : (r > 0 ? 1 : 0);
    return ST_OK;
}

Status str_eq(const String *a, const String *b, bool *out) {
    if (!a || !b || !out || !a->data || !b->data || a->length < 0 || b->length < 0) return ST_ERR_ARG;
    *out = (a->length == b->length) && (memcmp(a->data, b->data, (size_t)a->length) == 0);
    return ST_OK;
}

Status str_copy_to(const String *src, String *dst) {
    if (!src || !dst || !src->data || src->length < 0) return ST_ERR_ARG;
    char *p = (char *)malloc((size_t)src->length + 1U);
    if (!p) return ST_ERR_ALLOC;
    memcpy(p, src->data, (size_t)src->length + 1U);
    free(dst->data);
    dst->data = p;
    dst->length = src->length;
    return ST_OK;
}

Status str_clone(const String *src, String **out) {
    if (!src || !out || !src->data || src->length < 0) return ST_ERR_ARG;
    String *c = (String *)malloc(sizeof(String));
    if (!c) return ST_ERR_ALLOC;
    c->data = NULL;
    c->length = 0;
    Status st = str_copy_to(src, c);
    if (st != ST_OK) {
        free(c);
        return st;
    }
    *out = c;
    return ST_OK;
}

Status str_concat(String *dst, const String *src) {
    if (!dst || !src || !dst->data || !src->data || dst->length < 0 || src->length < 0) return ST_ERR_ARG;
    if (src->length > INT_MAX - dst->length) return ST_ERR_OVERFLOW;
    int total = dst->length + src->length;
    char *p = (char *)realloc(dst->data, (size_t)total + 1U);
    if (!p) return ST_ERR_ALLOC;
    memcpy(p + dst->length, src->data, (size_t)src->length + 1U);
    dst->data = p;
    dst->length = total;
    return ST_OK;
}

static bool digits_only(const String *s, int len) {
    if (!s || !s->data || s->length != len) return false;
    for (int i = 0; i < len; ++i) {
        if (!isdigit((unsigned char)s->data[i])) return false;
    }
    return true;
}

static Status parse_ts(const String *s, time_t *out, bool *unset) {
    if (!s || !s->data || !out) return ST_ERR_ARG;
    if (unset) *unset = false;
    if (strcmp(s->data, "00:00:0000 00:00:00") == 0) {
        if (unset) *unset = true;
        *out = 0;
        return ST_OK;
    }
    if (s->length != 19) return ST_ERR_FMT;
    int d = 0, mo = 0, y = 0, h = 0, mi = 0, se = 0;
    if (sscanf(s->data, "%2d:%2d:%4d %2d:%2d:%2d", &d, &mo, &y, &h, &mi, &se) != 6) return ST_ERR_FMT;
    if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59 || se > 59) return ST_ERR_FMT;
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_mday = d;
    t.tm_mon = mo - 1;
    t.tm_year = y - 1900;
    t.tm_hour = h;
    t.tm_min = mi;
    t.tm_sec = se;
    t.tm_isdst = -1;
    time_t res = mktime(&t);
    if (res == (time_t)-1) return ST_ERR_FMT;
    *out = res;
    return ST_OK;
}

static void addr_zero(Address *a) {
    memset(a, 0, sizeof(*a));
}

Status addr_free(Address *a) {
    if (!a) return ST_ERR_ARG;
    str_free(&a->city);
    str_free(&a->street);
    str_free(&a->building);
    str_free(&a->postal_code);
    a->house = 0;
    a->apartment = 0;
    return ST_OK;
}

Status addr_valid(const Address *a) {
    if (!a || !a->city.data || a->city.length <= 0 || !a->street.data || a->street.length <= 0) return ST_ERR_ARG;
    if (a->house < 1 || a->apartment < 1 || !a->building.data) return ST_ERR_ARG;
    if (!digits_only(&a->postal_code, 6)) return ST_ERR_FMT;
    return ST_OK;
}

Status addr_copy(const Address *src, Address *dst) {
    if (!src || !dst) return ST_ERR_ARG;
    Status st = addr_valid(src);
    if (st != ST_OK) return st;
    Address t;
    addr_zero(&t);
    t.house = src->house;
    t.apartment = src->apartment;
    st = str_init(src->city.data, &t.city);
    if (st == ST_OK) st = str_init(src->street.data, &t.street);
    if (st == ST_OK) st = str_init(src->building.data, &t.building);
    if (st == ST_OK) st = str_init(src->postal_code.data, &t.postal_code);
    if (st != ST_OK) {
        addr_free(&t);
        return st;
    }
    addr_free(dst);
    *dst = t;
    return ST_OK;
}

static void mail_zero(Mail *m) {
    memset(m, 0, sizeof(*m));
}

Status mail_free(Mail *m) {
    if (!m) return ST_ERR_ARG;
    addr_free(&m->recipient);
    str_free(&m->mail_id);
    str_free(&m->created_at);
    str_free(&m->delivered_at);
    m->weight = 0.0;
    return ST_OK;
}

Status mail_valid(const Mail *m, double eps) {
    if (!m || eps < 0.0) return ST_ERR_ARG;
    Status st = addr_valid(&m->recipient);
    if (st != ST_OK) return st;
    if (!isfinite(m->weight) || (0.0 - m->weight) > eps) return ST_ERR_ARG;
    if (!digits_only(&m->mail_id, 14)) return ST_ERR_FMT;
    time_t tc = 0, td = 0;
    bool unset = false;
    if (parse_ts(&m->created_at, &tc, &unset) != ST_OK || unset) return ST_ERR_FMT;
    if (parse_ts(&m->delivered_at, &td, &unset) != ST_OK) return ST_ERR_FMT;
    if (!unset && td < tc) return ST_ERR_ARG;
    return ST_OK;
}

Status mail_copy(const Mail *src, Mail *dst, double eps) {
    if (!src || !dst) return ST_ERR_ARG;
    Status st = mail_valid(src, eps);
    if (st != ST_OK) return st;
    Mail t;
    mail_zero(&t);
    t.weight = src->weight;
    st = addr_copy(&src->recipient, &t.recipient);
    if (st == ST_OK) st = str_init(src->mail_id.data, &t.mail_id);
    if (st == ST_OK) st = str_init(src->created_at.data, &t.created_at);
    if (st == ST_OK) st = str_init(src->delivered_at.data, &t.delivered_at);
    if (st != ST_OK) {
        mail_free(&t);
        return st;
    }
    mail_free(dst);
    *dst = t;
    return ST_OK;
}

static int cmp_mail_key(const void *pa, const void *pb) {
    const Mail *a = (const Mail *)pa, *b = (const Mail *)pb;
    int c = 0;
    if (str_cmp_order(&a->recipient.postal_code, &b->recipient.postal_code, &c) == ST_OK && c != 0) return c;
    if (str_cmp_order(&a->mail_id, &b->mail_id, &c) == ST_OK) return c;
    return 0;
}

Status post_sort(Post *p) {
    if (!p || (!p->items && p->count > 0U)) return ST_ERR_ARG;
    if (p->count > 1U) qsort(p->items, p->count, sizeof(Mail), cmp_mail_key);
    return ST_OK;
}

Status post_init(Post *p, Address *office) {
    if (!p || !office) return ST_ERR_ARG;
    Status st = addr_valid(office);
    if (st != ST_OK) return st;
    p->office_address = office;
    p->count = 0U;
    p->cap = 4U;
    p->items = (Mail *)calloc(p->cap, sizeof(Mail));
    if (!p->items) {
        p->cap = 0U;
        return ST_ERR_ALLOC;
    }
    return ST_OK;
}

Status post_free(Post *p) {
    if (!p) return ST_ERR_ARG;
    for (size_t i = 0; i < p->count; ++i) mail_free(&p->items[i]);
    free(p->items);
    memset(p, 0, sizeof(*p));
    return ST_OK;
}

Status post_find_idx(const Post *p, const String *id, size_t *idx) {
    if (!p || !id || !idx || !digits_only(id, 14)) return ST_ERR_ARG;
    for (size_t i = 0; i < p->count; ++i) {
        bool eq = false;
        if (str_eq(&p->items[i].mail_id, id, &eq) == ST_OK && eq) {
            *idx = i;
            return ST_OK;
        }
    }
    return ST_ERR_NOT_FOUND;
}

Status post_find(const Post *p, const String *id, Mail *out, double eps) {
    size_t i = 0U;
    Status st = post_find_idx(p, id, &i);
    if (st != ST_OK) return st;
    return mail_copy(&p->items[i], out, eps);
}

Status post_add(Post *p, const Mail *m, double eps) {
    if (!p || !m) return ST_ERR_ARG;
    Status st = mail_valid(m, eps);
    if (st != ST_OK) return st;
    size_t tmp = 0U;
    if (post_find_idx(p, &m->mail_id, &tmp) == ST_OK) return ST_ERR_DUP;
    if (p->count == p->cap) {
        if (p->cap > SIZE_MAX / 2U) return ST_ERR_OVERFLOW;
        size_t ncap = p->cap ? p->cap * 2U : 4U;
        Mail *np = (Mail *)realloc(p->items, ncap * sizeof(Mail));
        if (!np) return ST_ERR_ALLOC;
        for (size_t i = p->count; i < ncap; ++i) mail_zero(&np[i]);
        p->items = np;
        p->cap = ncap;
    }
    mail_zero(&p->items[p->count]);
    st = mail_copy(m, &p->items[p->count], eps);
    if (st != ST_OK) return st;
    p->count += 1U;
    return post_sort(p);
}

Status post_remove(Post *p, const String *id) {
    size_t idx = 0U;
    Status st = post_find_idx(p, id, &idx);
    if (st != ST_OK) return st;
    mail_free(&p->items[idx]);
    for (size_t i = idx; i + 1U < p->count; ++i) p->items[i] = p->items[i + 1U];
    p->count -= 1U;
    if (p->cap > 0U) mail_zero(&p->items[p->count]);
    return post_sort(p);
}

static int cmp_created(const void *pa, const void *pb) {
    const Mail *a = (const Mail *)pa, *b = (const Mail *)pb;
    time_t ta = 0, tb = 0;
    if (parse_ts(&a->created_at, &ta, NULL) != ST_OK || parse_ts(&b->created_at, &tb, NULL) != ST_OK) return 0;
    return (ta < tb) ? -1 : (ta > tb ? 1 : 0);
}

static bool is_overdue(time_t tc, time_t now) {
    if (now <= tc) return false;
    double diff = difftime(now, tc);
    return diff > 7.0 * 86400.0;
}

Status post_query(const Post *p, bool delivered, time_t now, Mail **out, size_t *out_n, double eps) {
    if (!p || !out || !out_n || eps < 0.0) return ST_ERR_ARG;
    *out = NULL;
    *out_n = 0U;
    if (p->count == 0U) return ST_OK;
    Mail *arr = (Mail *)calloc(p->count, sizeof(Mail));
    if (!arr) return ST_ERR_ALLOC;
    size_t n = 0U;
    for (size_t i = 0; i < p->count; ++i) {
        time_t tc = 0, td = 0;
        bool unset = false;
        if (parse_ts(&p->items[i].created_at, &tc, NULL) != ST_OK ||
            parse_ts(&p->items[i].delivered_at, &td, &unset) != ST_OK) continue;
        bool ok = delivered ? !unset : (unset && is_overdue(tc, now));
        if (ok) {
            Status st = mail_copy(&p->items[i], &arr[n], eps);
            if (st != ST_OK) {
                for (size_t k = 0; k < n; ++k) mail_free(&arr[k]);
                free(arr);
                return st;
            }
            n += 1U;
        }
    }
    if (n > 1U) qsort(arr, n, sizeof(Mail), cmp_created);
    *out = arr;
    *out_n = n;
    return ST_OK;
}

static void list_free(Mail *a, size_t n) {
    for (size_t i = 0; i < n; ++i) mail_free(&a[i]);
    free(a);
}

static bool read_line(char *buf, size_t sz) {
    if (!buf || sz < 2U || !fgets(buf, (int)sz, stdin)) return false;
    size_t l = strlen(buf);
    while (l > 0U && (buf[l - 1U] == '\n' || buf[l - 1U] == '\r')) buf[--l] = '\0';
    return true;
}

static bool read_int_pos(int *v) {
    char b[32], *e = NULL;
    if (!read_line(b, sizeof(b)) || !b[0]) return false;
    errno = 0;
    long x = strtol(b, &e, 10);
    if (errno || e == b || *e || x < 1L || x > INT_MAX) return false;
    *v = (int)x;
    return true;
}

static bool read_dbl_nonneg(double *v, double eps) {
    char b[32], *e = NULL;
    if (!read_line(b, sizeof(b)) || !b[0]) return false;
    double x = strtod(b, &e);
    if (e == b || *e || !isfinite(x) || (0.0 - x) > eps) return false;
    *v = x;
    return true;
}

static Status make_addr(const char *c, const char *s, int h, const char *b, int a, const char *z, Address *out) {
    if (!c || !s || !b || !z || !out) return ST_ERR_ARG;
    addr_zero(out);
    out->house = h;
    out->apartment = a;
    Status st = str_init(c, &out->city);
    if (st == ST_OK) st = str_init(s, &out->street);
    if (st == ST_OK) st = str_init(b, &out->building);
    if (st == ST_OK) st = str_init(z, &out->postal_code);
    if (st == ST_OK) st = addr_valid(out);
    if (st != ST_OK) addr_free(out);
    return st;
}

static Status make_mail(const Address *r, double w, const char *id, const char *cr, const char *dl, Mail *out, double eps) {
    if (!r || !id || !cr || !dl || !out) return ST_ERR_ARG;
    mail_zero(out);
    out->weight = w;
    Status st = addr_copy(r, &out->recipient);
    if (st == ST_OK) st = str_init(id, &out->mail_id);
    if (st == ST_OK) st = str_init(cr, &out->created_at);
    if (st == ST_OK) st = str_init(dl, &out->delivered_at);
    if (st == ST_OK) st = mail_valid(out, eps);
    if (st != ST_OK) mail_free(out);
    return st;
}

static Status seed_data(Post *post, double eps) {
    Address r1, r2;
    Mail m1, m2;
    addr_zero(&r1); addr_zero(&r2);
    mail_zero(&m1); mail_zero(&m2);
    Status st = make_addr("Tartu", "Main", 5, "A", 2, "50090", &r1);
    if (st == ST_OK) st = make_mail(&r1, 1.5, "11111111111111", "01:01:2025 10:00:00", "02:01:2025 12:00:00", &m1, eps);
    if (st == ST_OK) st = post_add(post, &m1, eps);
    if (st == ST_OK) st = make_addr("Narva", "Park", 9, "", 4, "20308", &r2);
    if (st == ST_OK) st = make_mail(&r2, 2.0, "22222222222222", "01:02:2025 08:00:00", "00:00:0000 00:00:00", &m2, eps);
    if (st == ST_OK) st = post_add(post, &m2, eps);
    mail_free(&m1); mail_free(&m2);
    addr_free(&r1); addr_free(&r2);
    return st;
}

static void demo_strings(void) {
    String a = {NULL, 0}, b = {NULL, 0}, c = {NULL, 0}, *d = NULL;
    int ord = 0;
    bool eq = false;
    Status st = str_init("abc", &a);
    if (st == ST_OK) st = str_init("abcd", &b);
    if (st == ST_OK) st = str_init("x", &c);
    if (st == ST_OK) st = str_cmp_order(&a, &b, &ord);
    if (st == ST_OK) st = str_eq(&a, &b, &eq);
    if (st == ST_OK) st = str_copy_to(&a, &c);
    if (st == ST_OK) st = str_clone(&b, &d);
    if (st == ST_OK && d) st = str_concat(&c, d);
    if (st == ST_OK) printf("%s %s %d %d\n", a.data, c.data, ord, eq ? 1 : 0);
    else printf("%d\n", (int)st);
    if (d) { str_free(d); free(d); }
    str_free(&c); str_free(&b); str_free(&a);
}

int main(int argc, char **argv) {
    if (argc > 2 || !argv) return 1;
    double eps = 1e-6;
    if (argc == 2) {
        char *e = NULL;
        double v = strtod(argv[1], &e);
        if (e == argv[1] || *e || !isfinite(v) || v < 0.0 || v > 1.0) return 1;
        eps = v;
    }

    demo_strings();

    Address office;
    Post post;
    addr_zero(&office);
    memset(&post, 0, sizeof(post));
    if (make_addr("Tallinn", "Viru", 1, "", 1, "10111", &office) != ST_OK) return 1;
    if (post_init(&post, &office) != ST_OK) { addr_free(&office); return 1; }
    if (seed_data(&post, eps) != ST_OK) { post_free(&post); addr_free(&office); return 1; }

    char cmd[16];
    bool run = true;
    while (run) {
        printf("1:add 2:del 3:find 4:deliv 5:over 6:all 0:exit\n");
        if (!read_line(cmd, sizeof(cmd))) break;
        if (strcmp(cmd, "0") == 0) run = false;
        else if (strcmp(cmd, "1") == 0) {
            char ci[64], st_s[64], bu[32], zi[32], id[32], cr[32], dl[32];
            int h = 0, ap = 0; double w = 0.0;
            Address r; Mail m;
            addr_zero(&r); mail_zero(&m);
            if (read_line(ci, sizeof(ci)) && read_line(st_s, sizeof(st_s)) && read_int_pos(&h) &&
                read_line(bu, sizeof(bu)) && read_int_pos(&ap) && read_line(zi, sizeof(zi)) &&
                read_dbl_nonneg(&w, eps) && read_line(id, sizeof(id)) &&
                read_line(cr, sizeof(cr)) && read_line(dl, sizeof(dl))) {
                Status s = make_addr(ci, st_s, h, bu, ap, zi, &r);
                if (s == ST_OK) s = make_mail(&r, w, id, cr, dl, &m, eps);
                if (s == ST_OK) s = post_add(&post, &m, eps);
                printf("%d\n", (int)s);
            } else printf("%d\n", (int)ST_ERR_ARG);
            mail_free(&m); addr_free(&r);
        } else if (strcmp(cmd, "2") == 0 || strcmp(cmd, "3") == 0) {
            char buf[64]; String q = {NULL, 0};
            if (read_line(buf, sizeof(buf)) && str_init(buf, &q) == ST_OK) {
                if (cmd[0] == '2') printf("%d\n", (int)post_remove(&post, &q));
                else {
                    Mail f; mail_zero(&f);
                    Status s = post_find(&post, &q, &f, eps);
                    if (s == ST_OK) printf("%s %s %.2f %s %s\n", f.mail_id.data, f.recipient.postal_code.data, f.weight, f.created_at.data, f.delivered_at.data);
                    else printf("%d\n", (int)s);
                    mail_free(&f);
                }
                str_free(&q);
            }
        } else if (strcmp(cmd, "4") == 0 || strcmp(cmd, "5") == 0) {
            Mail *list = NULL; size_t n = 0U; time_t now = time(NULL);
            Status s = (now == (time_t)-1) ? ST_ERR_ARG : post_query(&post, cmd[0] == '4', now, &list, &n, eps);
            if (s == ST_OK) {
                printf("%zu\n", n);
                for (size_t i = 0; i < n; ++i) printf("%s %s %s %s\n", list[i].mail_id.data, list[i].recipient.postal_code.data, list[i].created_at.data, list[i].delivered_at.data);
            } else printf("%d\n", (int)s);
            list_free(list, n);
        } else if (strcmp(cmd, "6") == 0) {
            post_sort(&post);
            printf("%zu\n", post.count);
            for (size_t i = 0; i < post.count; ++i) printf("%s %s %.2f %s %s\n", post.items[i].recipient.postal_code.data, post.items[i].mail_id.data, post.items[i].weight, post.items[i].created_at.data, post.items[i].delivered_at.data);
        }
    }

    post_free(&post);
    addr_free(&office);
    return 0;
}
