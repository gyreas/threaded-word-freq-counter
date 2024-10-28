#include <ctype.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dict.h"

#define CHUNK_N 5
#define BOUNDS_CHARS " -_;,.?^()[]{}<>%$@!~`&*+=|'\"\t\r\n\\"

struct thread_state {
    string chunk_start;
    size_t chunk_size;

    Dict*  dict ;
};

int is_word_boundary_char(char c) {
    size_t i;
    for (i = 0; i < strlen(BOUNDS_CHARS); i++) {
        if (c == BOUNDS_CHARS[i]) {
            return 1;
        }
    }
    return 0;
}

/** Determine the nearest point backward that marks a word */
string nearest_word_boundary(string buf, size_t offset) {
    size_t i;
    /* we're at a word boundary */
    if (is_word_boundary_char(buf[offset])) return buf + offset;

    /* we're sitting on an alphabetic character */
    /* so the word it's a part of is incomplete, backtack */
    for (i = offset; i != -1; i--) {
        if (is_word_boundary_char(buf[i])) {
            if (i > 0 && isalpha(buf[i-1])) {
                return buf + i;
            }
        }
    }
    return buf;
}

/*
 *  This is basically a lexer but into 'words'
 *  A 'word' is a sequence of alphabet characters surrounded by anything that is
 *  either EOF/EOL or is not a alphabetic character
 * 
 */
void *count_words(void* args) {
    char                 c;
    size_t               i, wordlen, chunk_sz;
    entry                word_ent;
    string               word_start, chunk_start;
    Dict*                tdict;
    struct thread_state* tstate;

    tstate = (struct thread_state*)args;
    tdict = tstate->dict;
    chunk_start = tstate->chunk_start;
    chunk_sz = tstate->chunk_size;
    word_start = NULL;

    for (i = 0; i < chunk_sz; i++) {
        c = chunk_start[i];

        if (!isalpha(c)) continue;

        /* read it as a word */
        word_start = chunk_start + i;
        wordlen = 1; /* include the current char */
        while (i + 1 < chunk_sz && !is_word_boundary_char(chunk_start[i+1])) {
            i++;
            wordlen++;
        }

        word_ent = entry_newn(word_start, wordlen);
        Dict_add(tdict, word_ent);

        word_start = NULL;
    }

    return tstate;
}

void threaded_freq(struct thread_state* tstates, string buf, size_t bufsz) {
    int                   s;
    Dict*                 dict;
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

int main(void)
{
    int                   s;
    FILE*                 fp;
    size_t                bufsz, tnum;
    string                buf;
    struct thread_state   tstate, tstates[CHUNK_N];

    fp = fopen("word_freq_threaded.c", "r");
    if (!fp) {
        fprintf(stderr, "WTF!!\n");
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

    Dict* big_dict = Dict_new();
    for (tnum = 0; tnum < CHUNK_N; tnum++) {
        tstate = tstates[tnum];
        printf("Results from thread%d:\n", tnum);
        /* Dict_print(tstate.dict); */
        Dict_merge(big_dict, tstate.dict);
        Dict_free(tstate.dict);
    }
    Dict_print(big_dict);

    free(buf);

    return 0;
}
