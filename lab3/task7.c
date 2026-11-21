#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NAME 64
#define MAX_UNDO 100

typedef enum {
    STATUS_OK = 0,
    STATUS_MEMORY_ERROR,
    STATUS_INVALID_INPUT,
    STATUS_IO_ERROR,
    STATUS_NOT_FOUND,
    STATUS_EMPTY
} Status;

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct {
    char last_name[MAX_NAME];
    char first_name[MAX_NAME];
    char middle_name[MAX_NAME];
    Date birth_date;
    char gender;
    double income;
} Liver;

typedef struct Node {
    Liver data;
    struct Node *next;
} Node;

typedef enum {
    ACTION_ADD,
    ACTION_DELETE,
    ACTION_EDIT
} ActionType;

typedef struct {
    ActionType type;
    Liver old_data;
    Liver new_data;
} UndoAction;

typedef struct {
    UndoAction actions[MAX_UNDO];
    int top;
    int modification_count;
} UndoStack;

static int is_valid_name(const char *str, int allow_empty) {
    if (!str) return 0;
    if (str[0] == '\0') return allow_empty;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalpha((unsigned char)str[i]) || !isascii((unsigned char)str[i])) return 0;
    }
    return 1;
}

static int compare_dates(Date a, Date b) {
    if (a.year != b.year) return b.year - a.year;
    if (a.month != b.month) return b.month - a.month;
    return b.day - a.day;
}

static void print_liver(const Liver *l) {
    printf("%s %s %s, DOB: %02d.%02d.%04d, Gender: %c, Income: %.2f\n",
           l->last_name, l->first_name, l->middle_name[0] ? l->middle_name : "",
           l->birth_date.day, l->birth_date.month, l->birth_date.year,
           l->gender, l->income);
}

static Status insert_sorted(Node **head, const Liver *liver, int record_undo, UndoStack *undo) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) return STATUS_MEMORY_ERROR;
    new_node->data = *liver;
    new_node->next = NULL;

    Node **current = head;
    while (*current != NULL && compare_dates((*current)->data.birth_date, liver->birth_date) < 0) {
        current = &((*current)->next);
    }

    new_node->next = *current;
    *current = new_node;

    if (record_undo && undo) {
        undo->actions[undo->top].type = ACTION_ADD;
        undo->actions[undo->top].new_data = *liver;
        undo->top++;
        undo->modification_count++;
    }
    return STATUS_OK;
}

static Status undo_last(UndoStack *undo, Node **head) {
    if (undo->modification_count == 0) return STATUS_EMPTY;

    int steps_to_undo = undo->modification_count / 2;
    if (steps_to_undo == 0) steps_to_undo = 1;

    for (int i = 0; i < steps_to_undo; i++) {
        if (undo->top == 0) break;
        undo->top--;
        UndoAction act = undo->actions[undo->top];

        if (act.type == ACTION_ADD) {
            Node **curr = head;
            while (*curr && !(strcmp((*curr)->data.last_name, act.new_data.last_name) == 0 && (*curr)->data.birth_date.year == act.new_data.birth_date.year)) {
                curr = &((*curr)->next);
            }
            if (*curr) {
                Node *to_del = *curr;
                *curr = to_del->next;
                free(to_del);
            }
        } else if (act.type == ACTION_DELETE) {
            Node *restore = malloc(sizeof(Node));
            if (restore) {
                restore->data = act.old_data;
                Node **curr = head;
                while (*curr && compare_dates((*curr)->data.birth_date, act.old_data.birth_date) < 0) {
                    curr = &((*curr)->next);
                }
                restore->next = *curr;
                *curr = restore;
            }
        } else if (act.type == ACTION_EDIT) {
            Node *curr = *head;
            while (curr && !(strcmp(curr->data.last_name, act.new_data.last_name) == 0 && curr->data.birth_date.year == act.new_data.birth_date.year)) {
                curr = curr->next;
            }
            if (curr) {
                curr->data = act.old_data;
            }
        }
    }

    undo->modification_count -= steps_to_undo;
    if (undo->modification_count < 0) undo->modification_count = 0;

    return STATUS_OK;
}

static Status load_file(const char *path, Node **head) {
    FILE *f = fopen(path, "r");
    if (!f) return STATUS_IO_ERROR;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        Liver l;
        int d, m, y;
        int parsed = sscanf(line, "%63s %63s %63s %d %d %d %c %lf",
                            l.last_name, l.first_name, l.middle_name,
                            &d, &m, &y, &l.gender, &l.income);

        if (parsed == 7) {
            l.middle_name[0] = '\0';
            parsed = sscanf(line, "%63s %63s %d %d %d %c %lf",
                            l.last_name, l.first_name, &d, &m, &y, &l.gender, &l.income);
        }

        if (parsed < 7 || !is_valid_name(l.last_name, 0) || !is_valid_name(l.first_name, 0) || !is_valid_name(l.middle_name, 1)) {
            fclose(f);
            return STATUS_INVALID_INPUT;
        }

        l.birth_date.day = d;
        l.birth_date.month = m;
        l.birth_date.year = y;

        if (l.income < 0) {
            fclose(f);
            return STATUS_INVALID_INPUT;
        }

        insert_sorted(head, &l, 0, NULL); 
    }
    fclose(f);
    return STATUS_OK;
}

static Status save_file(const char *path, Node *head) {
    FILE *f = fopen(path, "w");
    if (!f) return STATUS_IO_ERROR;

    Node *curr = head;
    while (curr) {
        fprintf(f, "%s %s %s %d %d %d %c %.2f\n",
                curr->data.last_name, curr->data.first_name, 
                curr->data.middle_name,
                curr->data.birth_date.day, curr->data.birth_date.month, curr->data.birth_date.year,
                curr->data.gender, curr->data.income);
        curr = curr->next;
    }
    fclose(f);
    return STATUS_OK;
}

static void free_list(Node *head) {
    while (head) {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    Node *head = NULL;
    UndoStack undo = { .top = 0, .modification_count = 0 };
    
    if (load_file(argv[1], &head) != STATUS_OK) {
        printf("Failed to load file.\n");
        return 1;
    }

    int choice = -1;
    while (choice != 0) {
        printf("\n--- MENU ---\n");
        printf("1. Print all\n2. Add\n3. Delete by last name\n4. Edit\n");
        printf("5. Undo last modifications\n6. Save to file\n0. Exit\nChoice: ");
        scanf("%d", &choice);

        if (choice == 1) {
            Node *curr = head;
            while (curr) { print_liver(&curr->data); curr = curr->next; }
        }
        else if (choice == 2) {
            Liver l;
            int d, m, y;
            printf("Last name: "); scanf("%63s", l.last_name);
            printf("First name: "); scanf("%63s", l.first_name);
            printf("Middle name (or - for empty): "); scanf("%63s", l.middle_name);
            if (strcmp(l.middle_name, "-") == 0) l.middle_name[0] = '\0';
            printf("DOB (d m y): "); scanf("%d %d %d", &d, &m, &y);
            printf("Gender (M/W): "); scanf(" %c", &l.gender);
            printf("Income: "); scanf("%lf", &l.income);
            l.birth_date.day = d; l.birth_date.month = m; l.birth_date.year = y;
            
            insert_sorted(&head, &l, 1, &undo);
            printf("Added.\n");
        }
        else if (choice == 3) {
            char target[MAX_NAME];
            printf("Last name to delete: "); scanf("%63s", target);
            Node **curr = &head;
            int found = 0;
            while (*curr) {
                if (strcmp((*curr)->data.last_name, target) == 0) {
                    Node *to_del = *curr;
                    if (undo.top < MAX_UNDO) {
                        undo.actions[undo.top].type = ACTION_DELETE;
                        undo.actions[undo.top].old_data = to_del->data;
                        undo.top++;
                        undo.modification_count++;
                    }
                    *curr = to_del->next;
                    free(to_del);
                    found = 1;
                    break;
                }
                curr = &((*curr)->next);
            }
            if (found) printf("Deleted.\n"); else printf("Not found.\n");
        }
        else if (choice == 5) {
            if (undo_last(&undo, &head) == STATUS_OK) printf("Undo completed.\n");
            else printf("Nothing to undo.\n");
        }
        else if (choice == 6) {
            char path[256];
            printf("File path: "); scanf("%255s", path);
            if (save_file(path, head) == STATUS_OK) printf("Saved.\n");
            else printf("Error saving.\n");
        }
    }

    free_list(head);
    return 0;
}
