#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
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
    ST_ERR_FMT,
    ST_ERR_IO
} Status;

typedef enum {
    STOP_MID = 0,
    STOP_START,
    STOP_END
} StopKind;

typedef struct {
    char *data;
    int length;
} String;

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

void str_free(String *s) {
    if (!s) return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

int str_cmp(const String *a, const String *b) {
    int n = a->length < b->length ? a->length : b->length;
    int r = memcmp(a->data, b->data, (size_t)n);
    if (r != 0) return r < 0 ? -1 : 1;
    if (a->length == b->length) return 0;
    return a->length < b->length ? -1 : 1;
}

bool str_eq(const String *a, const String *b) {
    return a->length == b->length &&
           memcmp(a->data, b->data, (size_t)a->length) == 0;
}

typedef struct {
    String vehicle;
    String stop_time;
    String depart_time;
    StopKind kind;
    double x;
    double y;
    time_t t_stop;
    time_t t_depart;
} Stop;

typedef struct StopNode {
    Stop stop;
    struct StopNode *next;
} StopNode;

typedef struct {
    String vehicle;
    StopNode *head;
    StopNode *tail;
    size_t n_stops;
} Route;

typedef struct RouteNode {
    Route route;
    struct RouteNode *next;
} RouteNode;

typedef struct {
    RouteNode *head;
    RouteNode *tail;
    size_t n_routes;
} Store;

static bool digits_str(const char *s) {
    if (!s || !*s) return false;
    for (const char *p = s; *p; ++p)
        if (!isdigit((unsigned char)*p)) return false;
    return true;
}

static Status parse_dt(const char *s, time_t *out) {
    if (!s || !out) return ST_ERR_ARG;
    int d = 0, mo = 0, y = 0, h = 0, mi = 0, se = 0;
    char tail;
    if (sscanf(s, "%2d.%2d.%4d %2d:%2d:%2d%c",
               &d, &mo, &y, &h, &mi, &se, &tail) != 6) return ST_ERR_FMT;
    if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31 ||
        h > 23 || mi > 59 || se > 59) return ST_ERR_FMT;
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_mday = d; t.tm_mon = mo - 1; t.tm_year = y - 1900;
    t.tm_hour = h; t.tm_min = mi; t.tm_sec = se; t.tm_isdst = -1;
    time_t r = mktime(&t);
    if (r == (time_t)-1) return ST_ERR_FMT;
    *out = r;
    return ST_OK;
}

static Status parse_kind(const char *s, StopKind *out) {
    if (!s || !out) return ST_ERR_ARG;
    if (strcmp(s, "middle") == 0 || strcmp(s, "mid") == 0 ||
        strcmp(s, "промежуточная") == 0) { *out = STOP_MID; return ST_OK; }
    if (strcmp(s, "start") == 0 || strcmp(s, "начальная") == 0) { *out = STOP_START; return ST_OK; }
    if (strcmp(s, "end") == 0 || strcmp(s, "конечная") == 0) { *out = STOP_END; return ST_OK; }
    return ST_ERR_FMT;
}

static Status parse_dbl(const char *s, double *out) {
    if (!s || !out) return ST_ERR_ARG;
    char *e = NULL;
    errno = 0;
    double v = strtod(s, &e);
    if (errno || e == s || *e || !isfinite(v)) return ST_ERR_FMT;
    *out = v;
    return ST_OK;
}

static void stop_free(Stop *s) {
    if (!s) return;
    str_free(&s->vehicle);
    str_free(&s->stop_time);
    str_free(&s->depart_time);
}

static Status stop_copy(Stop *dst, const Stop *src) {
    if (!dst || !src) return ST_ERR_ARG;
    memset(dst, 0, sizeof(*dst));
    dst->kind = src->kind;
    dst->x = src->x;
    dst->y = src->y;
    dst->t_stop = src->t_stop;
    dst->t_depart = src->t_depart;
    Status st = str_init(&dst->vehicle, src->vehicle.data);
    if (st == ST_OK) st = str_init(&dst->stop_time, src->stop_time.data);
    if (st == ST_OK) st = str_init(&dst->depart_time, src->depart_time.data);
    if (st != ST_OK) stop_free(dst);
    return st;
}

static Status route_init(Route *r, const char *vehicle) {
    if (!r || !vehicle || !*vehicle) return ST_ERR_ARG;
    memset(r, 0, sizeof(*r));
    return str_init(&r->vehicle, vehicle);
}

static void route_free(Route *r) {
    if (!r) return;
    StopNode *p = r->head;
    while (p) {
        StopNode *n = p->next;
        stop_free(&p->stop);
        free(p);
        p = n;
    }
    str_free(&r->vehicle);
    memset(r, 0, sizeof(*r));
}

static Status route_push(Route *r, const Stop *s) {
    if (!r || !s) return ST_ERR_ARG;
    StopNode *node = (StopNode *)malloc(sizeof(StopNode));
    if (!node) return ST_ERR_ALLOC;
    memset(node, 0, sizeof(*node));
    Status st = stop_copy(&node->stop, s);
    if (st != ST_OK) { free(node); return st; }
    if (!r->head) r->head = node;
    else r->tail->next = node;
    r->tail = node;
    r->n_stops += 1U;
    return ST_OK;
}

static Status route_sort_by_time(Route *r) {
    if (!r) return ST_ERR_ARG;
    if (r->n_stops < 2U) return ST_OK;
    StopNode **arr = (StopNode **)malloc(r->n_stops * sizeof(StopNode *));
    if (!arr) return ST_ERR_ALLOC;
    size_t i = 0;
    for (StopNode *p = r->head; p; p = p->next) arr[i++] = p;
    for (size_t a = 1; a < r->n_stops; ++a) {
        StopNode *key = arr[a];
        size_t b = a;
        while (b > 0 && arr[b - 1]->stop.t_stop > key->stop.t_stop) {
            arr[b] = arr[b - 1];
            b -= 1U;
        }
        arr[b] = key;
    }
    for (i = 0; i + 1U < r->n_stops; ++i) arr[i]->next = arr[i + 1U];
    arr[r->n_stops - 1U]->next = NULL;
    r->head = arr[0];
    r->tail = arr[r->n_stops - 1U];
    free(arr);
    return ST_OK;
}

static Status store_init(Store *s) {
    if (!s) return ST_ERR_ARG;
    memset(s, 0, sizeof(*s));
    return ST_OK;
}

static void store_free(Store *s) {
    if (!s) return;
    RouteNode *p = s->head;
    while (p) {
        RouteNode *n = p->next;
        route_free(&p->route);
        free(p);
        p = n;
    }
    memset(s, 0, sizeof(*s));
}

static Status store_find(Store *s, const char *vehicle, Route **out) {
    if (!s || !vehicle || !out) return ST_ERR_ARG;
    for (RouteNode *p = s->head; p; p = p->next) {
        if (strcmp(p->route.vehicle.data, vehicle) == 0) {
            *out = &p->route;
            return ST_OK;
        }
    }
    return ST_ERR_NOT_FOUND;
}

static Status store_add_route(Store *s, const char *vehicle, Route **out) {
    if (!s || !vehicle || !*vehicle || !out) return ST_ERR_ARG;
    Route *existing = NULL;
    if (store_find(s, vehicle, &existing) == ST_OK) { *out = existing; return ST_OK; }
    RouteNode *node = (RouteNode *)malloc(sizeof(RouteNode));
    if (!node) return ST_ERR_ALLOC;
    memset(node, 0, sizeof(*node));
    Status st = route_init(&node->route, vehicle);
    if (st != ST_OK) { free(node); return st; }
    if (!s->head) s->head = node;
    else s->tail->next = node;
    s->tail = node;
    s->n_routes += 1U;
    *out = &node->route;
    return ST_OK;
}

static Status store_add_stop(Store *s, const Stop *stop) {
    if (!s || !stop) return ST_ERR_ARG;
    Route *r = NULL;
    Status st = store_add_route(s, stop->vehicle.data, &r);
    if (st != ST_OK) return st;
    st = route_push(r, stop);
    if (st != ST_OK) return st;
    return route_sort_by_time(r);
}

static bool read_line_f(FILE *f, char *buf, size_t sz) {
    if (!f || !buf || sz < 2U) return false;
    if (!fgets(buf, (int)sz, f)) return false;
    size_t l = strlen(buf);
    while (l > 0U && (buf[l - 1U] == '\n' || buf[l - 1U] == '\r')) buf[--l] = '\0';
    return true;
}

static Status parse_stop_line(const char *line, double x, double y, Stop *out) {
    if (!line || !out) return ST_ERR_ARG;
    char veh[128], st_t[32], dp_t[32], kind[32];
    if (sscanf(line, "%127s %31s %31s %31s", veh, st_t, dp_t, kind) != 4)
        return ST_ERR_FMT;
    memset(out, 0, sizeof(*out));
    out->x = x;
    out->y = y;
    Status st = str_init(&out->vehicle, veh);
    if (st == ST_OK) st = str_init(&out->stop_time, st_t);
    if (st == ST_OK) st = str_init(&out->depart_time, dp_t);
    if (st == ST_OK) st = parse_dt(st_t, &out->t_stop);
    if (st == ST_OK) st = parse_dt(dp_t, &out->t_depart);
    if (st == ST_OK) st = parse_kind(kind, &out->kind);
    if (st == ST_OK && out->t_depart < out->t_stop) st = ST_ERR_FMT;
    if (st != ST_OK) stop_free(out);
    return st;
}

static Status load_file(Store *s, const char *path) {
    if (!s || !path) return ST_ERR_ARG;
    FILE *f = fopen(path, "r");
    if (!f) return ST_ERR_IO;
    char line[512];
    if (!read_line_f(f, line, sizeof(line))) { fclose(f); return ST_ERR_FMT; }
    char kw[16], xs[32], ys[32];
    if (sscanf(line, "%15s %31s %31s", kw, xs, ys) != 3 ||
        strcmp(kw, "coord") != 0) { fclose(f); return ST_ERR_FMT; }
    double x = 0.0, y = 0.0;
    Status st = parse_dbl(xs, &x);
    if (st == ST_OK) st = parse_dbl(ys, &y);
    if (st != ST_OK) { fclose(f); return st; }
    while (read_line_f(f, line, sizeof(line))) {
        if (!line[0]) continue;
        Stop stop;
        st = parse_stop_line(line, x, y, &stop);
        if (st != ST_OK) { fclose(f); return st; }
        st = store_add_stop(s, &stop);
        stop_free(&stop);
        if (st != ST_OK) { fclose(f); return st; }
    }
    fclose(f);
    return ST_OK;
}

static size_t route_len(const Route *r) { return r->n_stops; }

static double route_path_len(const Route *r) {
    if (!r || !r->head) return 0.0;
    double total = 0.0;
    for (StopNode *p = r->head; p && p->next; p = p->next) {
        double dx = p->next->stop.x - p->stop.x;
        double dy = p->next->stop.y - p->stop.y;
        total += sqrt(dx * dx + dy * dy);
    }
    return total;
}

static double route_span_time(const Route *r) {
    if (!r || !r->head) return 0.0;
    time_t first = r->head->stop.t_stop;
    time_t last = r->tail->stop.t_stop;
    return last >= first ? (double)(last - first) : 0.0;
}

static double route_idle(const Route *r) {
    if (!r) return 0.0;
    double idle = 0.0;
    for (StopNode *p = r->head; p; p = p->next) {
        if (p->stop.t_depart >= p->stop.t_stop)
            idle += (double)(p->stop.t_depart - p->stop.t_stop);
    }
    return idle;
}

static double stop_duration(const Stop *s) {
    if (!s || s->t_depart < s->t_stop) return 0.0;
    return (double)(s->t_depart - s->t_stop);
}

typedef enum {
    Q_MOST_ROUTES,
    Q_LEAST_ROUTES,
    Q_LONGEST_PATH,
    Q_SHORTEST_PATH,
    Q_LONGEST_ROUTE,
    Q_SHORTEST_ROUTE,
    Q_LONGEST_STOP,
    Q_SHORTEST_STOP,
    Q_LONGEST_IDLE,
    Q_SHORTEST_IDLE
} QueryKind;

static void print_vehicle(const char *title, const String *veh) {
    printf("%s: %s\n", title, veh ? veh->data : "(none)");
}

static Status query_most_least_routes(const Store *s, bool most) {
    if (!s || !s->head) return ST_ERR_NOT_FOUND;
    RouteNode *best = s->head;
    for (RouteNode *p = s->head->next; p; p = p->next) {
        if (most) { if (p->route.n_stops > best->route.n_stops) best = p; }
        else { if (p->route.n_stops < best->route.n_stops) best = p; }
    }
    print_vehicle(most ? "most routes" : "least routes", &best->route.vehicle);
    return ST_OK;
}

static Status query_path(const Store *s, bool longest) {
    if (!s || !s->head) return ST_ERR_NOT_FOUND;
    RouteNode *best = s->head;
    double bv = route_path_len(&best->route);
    for (RouteNode *p = s->head->next; p; p = p->next) {
        double v = route_path_len(&p->route);
        if (longest ? (v > bv) : (v < bv)) { bv = v; best = p; }
    }
    printf("%s: %s (%.6f)\n", longest ? "longest path" : "shortest path",
           best->route.vehicle.data, bv);
    return ST_OK;
}

static Status query_span(const Store *s, bool longest) {
    if (!s || !s->head) return ST_ERR_NOT_FOUND;
    RouteNode *best = s->head;
    double bv = route_span_time(&best->route);
    for (RouteNode *p = s->head->next; p; p = p->next) {
        double v = route_span_time(&p->route);
        if (longest ? (v > bv) : (v < bv)) { bv = v; best = p; }
    }
    printf("%s: %s (%.0f s)\n", longest ? "longest route" : "shortest route",
           best->route.vehicle.data, bv);
    return ST_OK;
}

static Status query_stop_duration(const Store *s, bool longest) {
    if (!s || !s->head) return ST_ERR_NOT_FOUND;
    bool found = false;
    String best_veh = {NULL, 0};
    double bv = 0.0;
    for (RouteNode *p = s->head; p; p = p->next) {
        for (StopNode *q = p->route.head; q; q = q->next) {
            double v = stop_duration(&q->stop);
            if (!found || (longest ? (v > bv) : (v < bv))) {
                found = true;
                bv = v;
                best_veh = p->route.vehicle;
            }
        }
    }
    if (!found) return ST_ERR_NOT_FOUND;
    printf("%s: %s (%.0f s)\n", longest ? "longest stop" : "shortest stop",
           best_veh.data, bv);
    return ST_OK;
}

static Status query_idle(const Store *s, bool longest) {
    if (!s || !s->head) return ST_ERR_NOT_FOUND;
    RouteNode *best = s->head;
    double bv = route_idle(&best->route);
    for (RouteNode *p = s->head->next; p; p = p->next) {
        double v = route_idle(&p->route);
        if (longest ? (v > bv) : (v < bv)) { bv = v; best = p; }
    }
    printf("%s: %s (%.0f s)\n", longest ? "longest idle" : "shortest idle",
           best->route.vehicle.data, bv);
    return ST_OK;
}

static Status run_query(const Store *s, QueryKind q) {
    switch (q) {
        case Q_MOST_ROUTES:    return query_most_least_routes(s, true);
        case Q_LEAST_ROUTES:   return query_most_least_routes(s, false);
        case Q_LONGEST_PATH:   return query_path(s, true);
        case Q_SHORTEST_PATH:  return query_path(s, false);
        case Q_LONGEST_ROUTE:  return query_span(s, true);
        case Q_SHORTEST_ROUTE: return query_span(s, false);
        case Q_LONGEST_STOP:   return query_stop_duration(s, true);
        case Q_SHORTEST_STOP:  return query_stop_duration(s, false);
        case Q_LONGEST_IDLE:   return query_idle(s, true);
        case Q_SHORTEST_IDLE:  return query_idle(s, false);
        default: return ST_ERR_ARG;
    }
}

static void print_all(const Store *s) {
    if (!s) return;
    printf("routes: %zu\n", s->n_routes);
    for (RouteNode *p = s->head; p; p = p->next) {
        printf("%s (%zu stops)\n", p->route.vehicle.data, p->route.n_stops);
        for (StopNode *q = p->route.head; q; q = q->next) {
            printf("  %s -> %s\n", q->stop.stop_time.data, q->stop.depart_time.data);
        }
    }
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
    if (argc < 2) { fprintf(stderr, "usage: %s file1 [file2 ...]\n", argv[0]); return 1; }
    Store store;
    if (store_init(&store) != ST_OK) return 1;
    for (int i = 1; i < argc; ++i) {
        Status st = load_file(&store, argv[i]);
        if (st != ST_OK) {
            fprintf(stderr, "load error %s: %d\n", argv[i], (int)st);
            store_free(&store);
            return 1;
        }
    }

    bool run = true;
    while (run) {
        printf("1:mostRoutes 2:leastRoutes 3:longestPath 4:shortestPath "
               "5:longestRoute 6:shortestRoute 7:longestStop 8:shortestStop "
               "9:longestIdle 10:shortestIdle 11:all 0:exit\n");
        int cmd = -1;
        if (!read_int(&cmd)) break;
        Status st = ST_OK;
        switch (cmd) {
            case 0: run = false; break;
            case 1: st = run_query(&store, Q_MOST_ROUTES); break;
            case 2: st = run_query(&store, Q_LEAST_ROUTES); break;
            case 3: st = run_query(&store, Q_LONGEST_PATH); break;
            case 4: st = run_query(&store, Q_SHORTEST_PATH); break;
            case 5: st = run_query(&store, Q_LONGEST_ROUTE); break;
            case 6: st = run_query(&store, Q_SHORTEST_ROUTE); break;
            case 7: st = run_query(&store, Q_LONGEST_STOP); break;
            case 8: st = run_query(&store, Q_SHORTEST_STOP); break;
            case 9: st = run_query(&store, Q_LONGEST_IDLE); break;
            case 10: st = run_query(&store, Q_SHORTEST_IDLE); break;
            case 11: print_all(&store); break;
            default: st = ST_ERR_ARG; break;
        }
        if (st != ST_OK) printf("status: %d\n", (int)st);
    }

    store_free(&store);
    return 0;
}
