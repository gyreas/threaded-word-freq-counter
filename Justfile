default: wft

wft:
    c89 -ggdb -o wft word_freq_threaded.c dict.c dict.h
    ./wft

dict:
    c89 -o dict dict.c dict.h
    ./dict

