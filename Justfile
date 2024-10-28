default: wft

wft:
    c89 -ggdb -Wall -Wpedantic -Werror -o wft word_freq_threaded.c dict.c dict.h
    ./wft

dict:
    c89 -o dict dict.c dict.h
    ./dict

