#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"

entry entry_new(string word) {
    entry e;

    e.word = strndup(word, MAX_WORD_LEN);
    e.freq = 1;

    return e;
}
entry entry_newn(string word, size_t len) {
    entry e;

    e.word = strndup(word, len);
    e.freq = 1;

    return e;
}
void entry_print(entry ent) {
    printf("{'%s': %lu}\n", ent.word, ent.freq);
}

Dict* Dict_new() {
    Dict* dict = (Dict*)malloc(sizeof(Dict));
    dict->entries = (entry*)malloc(ARRAY_INIT_CAP*sizeof(entry));
    dict->len = 0;
    dict->capacity = ARRAY_INIT_CAP;

    return dict;
}
entry Dict_get(Dict* dict, string ent) {
    return dict->entries[0];
}
void Dict_print(Dict* dict) {
    size_t i;
    printf("[\n");
    for (i = 0; i < dict->len; i++) {
        printf("  ");
        entry_print(dict->entries[i]);
    }
    printf("]\n");

}
int Dict_find(Dict* dict, entry ent) {
    int entlen = strlen(ent.word);
    size_t i;
    for (i = 0; i < dict->len; i++) {
        entry e = dict->entries[i];
        if (strlen(e.word) != entlen) continue;
        if (memcmp(e.word, ent.word, entlen) != 0) continue;

        return i;
    }
    return -1;
}
void Dict_add(Dict* dict, entry ent) {
    int idx = Dict_find(dict, ent);
    if (idx ==  -1) {
        Dict_resize(dict, 0);
        dict->entries[dict->len] = ent;
        dict->len++;
    } else {
        dict->entries[idx].freq++;
    }
}
void Dict_merge(Dict* this, Dict* that) {
    size_t i;
    entry  that_ent, this_ent;
    for (i = 0; i < that->len; i++) {
        that_ent = that->entries[i];
        this_ent = entry_new(that_ent.word);
        this_ent.freq = that_ent.freq;
        Dict_add(this, this_ent);
    } 
}
/** `hint` of 0 means use default */
void Dict_resize(Dict* dict, size_t hint) {
    entry* tmp;

    if (dict->len + hint < dict->capacity) return;

    dict->capacity += (hint == 0) ? dict->capacity : dict->len + hint;
    tmp = realloc(dict->entries, dict->capacity * sizeof(entry));
    if (tmp == NULL) {
        fprintf(stderr, "realloc: not enough memory\n");
    }
    dict->entries = tmp;
}
void Dict_free(Dict* dict) {
    /**
    size_t i;
    for (i = 0; i < dict->len; i++) {
        free(dict->entries[i].word);
    } */
    free(dict->entries);
    free(dict);
}

int cmp(const void* a, const void* b) {
    return *((int*) a) - *((int*) b);
}
void Dict_sort(Dict* dict) {
    qsort(dict->entries, dict->len, sizeof(dict->entries[0]), cmp);
} 

/*
void Dict_insert_at(Dict* dict, entry* entry, size_t idx) {
    Dict_resize(dict, 1);
    memmove(dict->entries + idx + 1, dict->entries + idx, (dict->len - idx)*sizeof(dict->entries[0]));
    dict->entries[idx] = *entry;
    dict->len++;
}
void Dict_append_many(Dict* dict, int* entries, size_t sz) {
    Dict_resize(dict, sz);
    memcpy(dict->entries + dict->len, entries, sz  * sizeof(dict->entries[0]));
    dict->len += sz;
}
void Dict_insert_many_at(Dict* dict, int *entries, size_t sz, size_t idx) {
    if (idx > dict->len) {
        fprintf(stdout, "%s:%d: error: index %zu outside dict of length %zu\n", __FILE_NAME__, __LINE__, idx, dict->len);
        exit(EXIT_FAILURE);
    }

    Dict_resize(dict, sz);

    int* new_pos = dict->entries + idx + sz;
    int* prev_pos = dict->entries + idx;
    size_t movelen = (dict->len - idx + 1)*sizeof(dict->entries[0]);

    memmove(new_pos, prev_pos, movelen);
    memcpy(prev_pos, entries, sz*sizeof(dict->entries[0]));
    dict->len += sz;

} */

/*
int main_(void) {
    entry ent = entry_new("Saheed");
    entry entS = entry_new("Adeleye");
    Dict *D = Dict_new();
    Dict_add(D, ent);
    Dict_add(D, ent);
    Dict_add(D, ent);
    Dict_add(D, ent);
    Dict_add(D, ent);
    printf("D: ");
    Dict_print(D);

    Dict *A = Dict_new();
    Dict_add(A, entS);
    Dict_add(A, entS);
    Dict_add(A, entS);
    printf("A: ");
    Dict_print(A);

    Dict *B = Dict_new();
    Dict_merge(B, A);
    Dict_merge(B, D);
    printf("B: ");
    Dict_print(B);

    Dict_free(D);
    Dict_free(A);
    Dict_free(B);
}
*/
