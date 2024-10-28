#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dict.h"

#ifndef CHUNK_N
  #define CHUNK_N 4
#endif

#define BOUNDS_CHARS " -_:;,.?^()[]{}<>%$@!~`&*+=|'\"\t\r\n\\"
#define WORD_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

struct thread_state {
    string chunk_start;
    size_t chunk_size;

    Dict*  dict ;
};

int is_word_boundary_char(char c) {
    char* idx = strchr(BOUNDS_CHARS, c);

    return idx != NULL;
}

int is_word_char(char c) {
    char* idx = strchr(WORD_CHARS, c);

    return idx != NULL;
}

/** Determine the nearest point backward that marks a word */
string nearest_word_boundary(string buf, size_t offset) {
    size_t i;
    /* we're at a word boundary */
    if (is_word_boundary_char(buf[offset])) return buf + offset;

    /* we're sitting on an alphabetic character */
    /* so the word it's a part of is incomplete, backtack */
    for (i = offset; i != -1; i--) {
        if (is_word_boundary_char(buf[i]) && i > 0 && isalpha(buf[i-1])) {
            return buf + i;
        }
    }
    return buf;
}

void count_words_(Dict** dict, char* buf, size_t buflen) {
    size_t i, wordlen;
    entry  word_ent;
    char   c, *word_start;

    word_start = NULL;
    for (i = 0; i < buflen; i++) {
        c = buf[i];

        if (!isalpha(c)) continue;

        /* read it as a word */
        word_start = buf + i;
        wordlen = 1; /* include the current char */
        while (i + 1 < buflen && is_word_char(buf[i+1])) {
            i++;
            wordlen++;
        }

        word_ent = entry_newn(word_start, wordlen);
        Dict_add(*dict, word_ent);

        word_start = NULL;
    }
}

/*
 *  This is basically a lexer but into 'words'
 *  A 'word' is a sequence of alphabet characters surrounded by anything that is
 *  either EOF/EOL or is not a alphabetic character
 * 
 */
void *count_words(void* args) {
    size_t               chunk_sz;
    string               chunk_start;
    Dict*                tdict;
    struct thread_state* tstate;

    tstate = (struct thread_state*)args;
    tdict = tstate->dict;
    chunk_start = tstate->chunk_start;
    chunk_sz = tstate->chunk_size;

    count_words_(&tdict, chunk_start, chunk_sz);
    return tstate;
}

void threaded_freq(struct thread_state* tstates, string buf, size_t bufsz) {
    int                   s;
    size_t                tnum, offset, chunk_sz, CHUNK_SZ_APPROX;
    pthread_t             threads[CHUNK_N];

    offset = 0;
    CHUNK_SZ_APPROX = bufsz / CHUNK_N;
    for (tnum = 0; tnum < CHUNK_N; tnum++) {
        chunk_sz =  nearest_word_boundary(buf, offset + CHUNK_SZ_APPROX) - (buf + offset);
        if (tnum == CHUNK_N - 1) chunk_sz = bufsz - offset;

        tstates[tnum].chunk_size = chunk_sz;
        tstates[tnum].chunk_start = buf + offset;
        tstates[tnum].dict = Dict_new();

        s = pthread_create(&threads[tnum], NULL, count_words, (void*)&tstates[tnum]);
        if (s != 0) {
           fprintf(stderr, "pthread_create: could not create thread\n");
           exit(1);
        }
        offset += chunk_sz;
    }
    if (offset != bufsz) {
        fprintf(stderr, "WHEREAREYOU!!\n");
        abort();
    }

    for (tnum = 0; tnum < CHUNK_N; tnum++) {
        s = pthread_join(threads[tnum], NULL);
        if (s != 0) {
            fprintf(stderr, "pthread_join: could not join thread\n");
            exit(1);
        }
    }
}

int main(int arglen, char* args[])
{
    FILE*                 fp;
    size_t                bufsz, tnum;
    Dict*                 big_dict;
    string                buf, file_path;
    struct thread_state   tstate, tstates[CHUNK_N];

    if (arglen == 1) {
        fprintf(stderr, "usage: ./wft <textfile>\n");
        exit(1);
    }

    file_path = args[1];
    fp = fopen(file_path, "r");
    if (!fp) {
        fprintf(stderr, "open: could not open file: %s\n", file_path);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    bufsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = (string)malloc(bufsz * sizeof(char));
    if (fread(buf, sizeof(char), bufsz, fp) != bufsz) {
        fprintf(stderr, "fread() failed\n");
        exit(1);
    }

    threaded_freq(tstates, buf, bufsz);

    big_dict = Dict_new();
    for (tnum = 0; tnum < CHUNK_N; tnum++) {
        tstate = tstates[tnum];
        Dict_merge(big_dict, tstate.dict);
        Dict_free(tstate.dict);
    }
    Dict_print(big_dict);

    free(buf);

    return 0;
}
