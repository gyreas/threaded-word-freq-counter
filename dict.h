#ifndef dict__inc
#define dict__inc

#include <stdlib.h>
#include <stdint.h>

#define MAX_WORD_LEN 256
#define ARRAY_INIT_CAP 256

typedef char* string;

typedef struct {
    uint64_t freq;
    string word;
} entry;

entry entry_new(string);

typedef struct {
    size_t len;
    size_t capacity;
    entry  entries[];
} Dict;

Dict* Dict_new();
entry Dict_get(Dict* dict, string);
int Dict_find(Dict* dict, entry);
void Dict_add(Dict* dict, entry);
void Dict_free(Dict* dict);
void Dict_resize(Dict* dict, size_t hint);
void Dict_print(Dict* dict);
void Dict_sort(Dict* dict);

// void Dict_insert_at(Dict* dict, entry* entry, size_t idx);
// void Dict_insert_many_at(Dict* dict, entry* entries, size_t sz, size_t idx);
// void Dict_append_many(Dict* dict, entry* entrys, size_t sz);
#endif
