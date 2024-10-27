#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dict.h"

#define CHUNK_N 2
#define BOUNDS_CHARS " -_;,.?^%$@!~`&*+=|'\"\t\r\n\\"

struct thread_state {
    string chunk_start;
    size_t chunk_size;

    Dict*  dict ;
};

int has_entry(Dict* dict, string word) {
    for (size_t e = 0; e < dict->len; e++) {
       entry ent = Dict_get(dict, e);
       if (memcmp(ent.word, word, strlen(word)) == 0)
           return 1;
    }
    return 0;
}

int is_word_boundary_marker(char c) {
    int    found;
    size_t     i;

    found = 0;
    for (i = 0; i < strlen(BOUNDS_CHARS); i++) {
        if (c == BOUNDS_CHARS[i]) {
            found = 1;
            break;
        }
    }
    return found;
}

string nearest_word_boundary(string buf, size_t offset) {
    size_t i = offset;
    for (; i >= 0; i--) {
        if (is_word_boundary_marker(buf[i])) {
            if (i > 0 && isalpha(buf[i-1])) {
                return buf + i;
            }
        }
    }
    return buf;
}

void *count_words(void* args) {
    char                 c;
    size_t               i, wordlen;
    entry*               word_ent;
    string               word, start, token;
    struct thread_state* tstate;

    tstate = (struct thread_state*)args;

    start = NULL;
    char tempbuf[tstate->chunk_size + 1];
    memcpy(tempbuf, tstate->chunk_start, tstate->chunk_size + 1);

    printf("[thread%lx] word: '%s'\n", pthread_self(), tempbuf);
    for (i = 0; i < tstate->chunk_size; i++) {
        c = tstate->chunk_start[i];
        if (isalpha(c)) {
            if (!start) {
                start = tstate->chunk_start + i;
            }

            if (i + 1 == tstate->chunk_size) {
                wordlen = tstate->chunk_start + i - start + 1;
                word = calloc(wordlen, sizeof(char));
                memcpy(word, start, sizeof(char)*wordlen);

                printf("[thread%lx] found: '%s'\n", pthread_self(), word);

                word_ent = entry_new();
                word_ent->word = word;
                Dict_append(tstate->dict, word_ent);

                start = NULL;
            }
        } else if (is_word_boundary_marker(c)) {
            if (!start) continue;

            wordlen = tstate->chunk_start + i - start;
            word = calloc(wordlen, sizeof(char));
            memcpy(word, start, sizeof(char)*wordlen);

            printf("[thread%lx] found: '%s'\n", pthread_self(), word);

            word_ent = entry_new();
            word_ent->word = word;
            Dict_append(tstate->dict, word_ent);

            start = NULL;
        }
    }

    return tstate;
}

int main(void) {
    int                   s;
    Dict*                 dict;
    FILE*                 fp;
    entry*                word_ent;
    size_t                bufsz, tnum, start, chunk_sz, CHUNK_SZ_APPROX;
    string                buf;
    pthread_t             threads[CHUNK_N];
    struct thread_state   tstates[CHUNK_N];
    struct thread_state   tstate;

    fp = fopen("problem", "r");
    if (!fp) {
        fprintf(stderr, "WTF!!\n");
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    bufsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = (string)malloc(bufsz * sizeof(char));
    if (fread(buf, sizeof(*buf), bufsz, fp) != bufsz) {
        fprintf(stderr, "fread() failed\n");
        exit(1);
    }

    start = 0;
    CHUNK_SZ_APPROX = bufsz / CHUNK_N;
    // tstates = malloc(CHUNK_N * sizeof(struct thread_state));
    for (tnum = 0; tnum < CHUNK_N; tnum++) {

        chunk_sz = nearest_word_boundary(buf, start + CHUNK_SZ_APPROX) - (buf + start);
        if (tnum == CHUNK_N - 1) chunk_sz = bufsz - start;

        tstates[tnum].chunk_size = chunk_sz;
        tstates[tnum].chunk_start = buf + start;

        tstates[tnum].dict = Dict_new();

        // char tempbuf[chunk_sz+1];
        // memset(tempbuf, 0, chunk_sz+1);
        // memcpy(tempbuf, buf + start, chunk_sz);

        // printf("start: %zd,  buf: '%s'\n", start, tempbuf);
        // printf("start: %zd,  chunksz: %d\n", start, (int)chunk_sz);
        // printf("thread[%zu] buf: '%s'\n", tnum, tempbuf);

        s = pthread_create(&threads[tnum], NULL, count_words, (void*)&tstates[tnum]);
        if (s != 0) {
           fprintf(stderr, "pthread_create: could not create thread\n");
           exit(1);
        }
        // printf("thread[%zu] created\n", threads[tnum]);
        start += chunk_sz;
    }
    if (start != bufsz) {
        fprintf(stderr, "WHEREAREYOU!!\n");
        abort();
    }

    dict = malloc(sizeof(Dict));
    for (tnum = 0; tnum < CHUNK_N; tnum++) {
        s = pthread_join(threads[tnum], NULL);
        if (s != 0) {
            fprintf(stderr, "pthread_join: could not join thread\n");
            exit(1);
        }
        printf("here\n");
        Dict_print(dict);
        // printf("freed sth: %x\n", threads[tnum]);
    }
    free(buf);

    return 0;
}
