#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME_LEN 64
#define EXAMS_COUNT 5

typedef enum {
    STATUS_OK = 0,
    STATUS_MEMORY_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_IO_ERROR,
    STATUS_NOT_FOUND
} Status;

typedef struct {
    unsigned int id;
    char first_name[MAX_NAME_LEN];
    char last_name[MAX_NAME_LEN];
    char group[MAX_NAME_LEN];
    unsigned char *grades;
} Student;

typedef struct {
    Student *data;
    size_t size;
    size_t capacity;
} StudentArray;

static int is_valid_name(const char *str) {
    if (!str || !*str) return 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalpha((unsigned char)str[i]) || !isascii((unsigned char)str[i])) {
            return 0;
        }
    }
    return 1;
}

static double get_average(const Student *s) {
    unsigned int sum = 0;
    for (int i = 0; i < EXAMS_COUNT; i++) {
        sum += s->grades[i];
    }
    return (double)sum / EXAMS_COUNT;
}

static void free_student_array(StudentArray *arr) {
    if (arr && arr->data) {
        for (size_t i = 0; i < arr->size; i++) {
            free(arr->data[i].grades);
        }
        free(arr->data);
        arr->data = NULL;
        arr->size = 0;
        arr->capacity = 0;
    }
}

static Status append_student(StudentArray *arr, const Student *s) {
    if (arr->size >= arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? 16 : arr->capacity * 2;
        Student *new_data = realloc(arr->data, new_cap * sizeof(Student));
        if (!new_data) return STATUS_MEMORY_ERROR;
        arr->data = new_data;
        arr->capacity = new_cap;
    }
    arr->data[arr->size] = *s;
    arr->size++;
    return STATUS_OK;
}

static Status load_students_from_file(const char *path, StudentArray *arr) {
    FILE *f = fopen(path, "r");
    if (!f) return STATUS_IO_ERROR;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        Student s;
        memset(&s, 0, sizeof(Student));

        char first[MAX_NAME_LEN], last[MAX_NAME_LEN], group[MAX_NAME_LEN];
        int g1, g2, g3, g4, g5;

        int parsed = sscanf(line, "%u %63s %63s %63s %d %d %d %d %d",
                            &s.id, first, last, group, &g1, &g2, &g3, &g4, &g5);

        if (parsed != 9) {
            fclose(f);
            free_student_array(arr);
            return STATUS_INVALID_INPUT;
        }

        if (!is_valid_name(first) || !is_valid_name(last) || group[0] == '\0') {
            fclose(f);
            free_student_array(arr);
            return STATUS_INVALID_INPUT;
        }

        strcpy(s.first_name, first);
        strcpy(s.last_name, last);
        strcpy(s.group, group);

        int grades[] = {g1, g2, g3, g4, g5};
        s.grades = malloc(EXAMS_COUNT * sizeof(unsigned char));
        if (!s.grades) {
            fclose(f);
            free_student_array(arr);
            return STATUS_MEMORY_ERROR;
        }

        for (int i = 0; i < EXAMS_COUNT; i++) {
            if (grades[i] < 0 || grades[i] > 255) {
                free(s.grades);
                fclose(f);
                free_student_array(arr);
                return STATUS_INVALID_INPUT;
            }
            s.grades[i] = (unsigned char)grades[i];
        }

        Status st = append_student(arr, &s);
        if (st != STATUS_OK) {
            free(s.grades);
            fclose(f);
            free_student_array(arr);
            return st;
        }
    }

    if (ferror(f)) {
        fclose(f);
        free_student_array(arr);
        return STATUS_IO_ERROR;
    }

    fclose(f);
    return STATUS_OK;
}

static int cmp_by_id(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    if (sa->id < sb->id) return -1;
    if (sa->id > sb->id) return 1;
    return 0;
}

static int cmp_by_last_name(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->last_name, sb->last_name);
}

static int cmp_by_first_name(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->first_name, sb->first_name);
}

static int cmp_by_group(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->group, sb->group);
}

static void search_by_id(const StudentArray *arr, unsigned int id) {
    int found = 0;
    for (size_t i = 0; i < arr->size; i++) {
        if (arr->data[i].id == id) {
            printf("Found: %s %s, group: %s\n",
                   arr->data[i].first_name, arr->data[i].last_name, arr->data[i].group);
            found = 1;
        }
    }
    if (!found) printf("Student with id %u not found.\n", id);
}

static void search_by_string(const StudentArray *arr, const char *query, int field) {
    int found = 0;
    for (size_t i = 0; i < arr->size; i++) {
        const char *val = (field == 0) ? arr->data[i].last_name :
                          (field == 1) ? arr->data[i].first_name : arr->data[i].group;
        if (strcmp(val, query) == 0) {
            printf("Found: %u %s %s, group: %s\n",
                   arr->data[i].id, arr->data[i].first_name, arr->data[i].last_name, arr->data[i].group);
            found = 1;
        }
    }
    if (!found) printf("Students not found.\n");
}

static Status trace_student_by_id(const StudentArray *arr, unsigned int id, const char *trace_path) {
    FILE *f = fopen(trace_path, "w");
    if (!f) return STATUS_IO_ERROR;

    int found = 0;
    for (size_t i = 0; i < arr->size; i++) {
        if (arr->data[i].id == id) {
            fprintf(f, "Full name: %s %s\n", arr->data[i].first_name, arr->data[i].last_name);
            fprintf(f, "Group: %s\n", arr->data[i].group);
            fprintf(f, "Average grade: %.2f\n", get_average(&arr->data[i]));
            found = 1;
            break;
        }
    }
    fclose(f);
    return found ? STATUS_OK : STATUS_NOT_FOUND;
}

static Status trace_above_average(const StudentArray *arr, const char *trace_path) {
    if (arr->size == 0) return STATUS_INVALID_INPUT;

    double total_avg = 0.0;
    for (size_t i = 0; i < arr->size; i++) {
        total_avg += get_average(&arr->data[i]);
    }
    total_avg /= arr->size;

    FILE *f = fopen(trace_path, "w");
    if (!f) return STATUS_IO_ERROR;

    fprintf(f, "Average grade for all students: %.2f\n", total_avg);
    fprintf(f, "Students with above average grade:\n");
    for (size_t i = 0; i < arr->size; i++) {
        if (get_average(&arr->data[i]) > total_avg) {
            fprintf(f, "%s %s\n", arr->data[i].last_name, arr->data[i].first_name);
        }
    }
    fclose(f);
    return STATUS_OK;
}

static void print_menu(void) {
    printf("\n--- MENU ---\n");
    printf("1. Search by ID\n");
    printf("2. Search by last name\n");
    printf("3. Search by first name\n");
    printf("4. Search by group\n");
    printf("5. Sort by ID\n");
    printf("6. Sort by last name\n");
    printf("7. Sort by first name\n");
    printf("8. Sort by group\n");
    printf("9. Print all students\n");
    printf("10. Trace student by ID to file\n");
    printf("11. Trace above-average students to file\n");
    printf("0. Exit\n");
    printf("Choice: ");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <students_file>\n", argv[0]);
        return 1;
    }

    StudentArray arr = {NULL, 0, 0};
    Status st = load_students_from_file(argv[1], &arr);
    if (st != STATUS_OK) {
        fprintf(stderr, "Error loading file (code %d)\n", st);
        return 1;
    }

    int choice = -1;
    while (choice != 0) {
        print_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        unsigned int id;
        char query[MAX_NAME_LEN];
        char trace_path[256];

        switch (choice) {
            case 1:
                printf("Enter ID: ");
                if (scanf("%u", &id) == 1) search_by_id(&arr, id);
                break;
            case 2:
                printf("Enter last name: ");
                scanf("%63s", query);
                search_by_string(&arr, query, 0);
                break;
            case 3:
                printf("Enter first name: ");
                scanf("%63s", query);
                search_by_string(&arr, query, 1);
                break;
            case 4:
                printf("Enter group: ");
                scanf("%63s", query);
                search_by_string(&arr, query, 2);
                break;
            case 5:
                qsort(arr.data, arr.size, sizeof(Student), cmp_by_id);
                printf("Sorted by ID.\n");
                break;
            case 6:
                qsort(arr.data, arr.size, sizeof(Student), cmp_by_last_name);
                printf("Sorted by last name.\n");
                break;
            case 7:
                qsort(arr.data, arr.size, sizeof(Student), cmp_by_first_name);
                printf("Sorted by first name.\n");
                break;
            case 8:
                qsort(arr.data, arr.size, sizeof(Student), cmp_by_group);
                printf("Sorted by group.\n");
                break;
            case 9:
                for (size_t i = 0; i < arr.size; i++) {
                    printf("%u %s %s %s\n", arr.data[i].id, arr.data[i].first_name, arr.data[i].last_name, arr.data[i].group);
                }
                break;
            case 10:
                printf("Enter ID: ");
                if (scanf("%u", &id) == 1) {
                    printf("Enter trace file path: ");
                    scanf("%255s", trace_path);
                    if (trace_student_by_id(&arr, id, trace_path) == STATUS_OK)
                        printf("Written to %s\n", trace_path);
                    else
                        printf("Student not found or write error.\n");
                }
                break;
            case 11:
                printf("Enter trace file path: ");
                scanf("%255s", trace_path);
                if (trace_above_average(&arr, trace_path) == STATUS_OK)
                    printf("Written to %s\n", trace_path);
                else
                    printf("Write error.\n");
                break;
            case 0:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice. Try again.\n");
        }
    }

    free_student_array(&arr);
    return 0;
}
