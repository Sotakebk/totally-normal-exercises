#include <stdio.h>

#include "common.h"

const char* CPU_FIRST_NAME = "cpufirst.bin";
const char* USER_FIRST_NAME = "userfirst.bin";

int files_exist() {
    FILE* userfile = fopen(USER_FIRST_NAME, "rb");
    FILE* cpufile = fopen(CPU_FIRST_NAME, "rb");

    int ret = 0;

    ret = (userfile && cpufile);

    if (userfile) fclose(userfile);
    if (cpufile) fclose(cpufile);

    if (ret)
        return 1;
    return 0;
}

int id_to_nth(int id, board* b) {
    int counter = 0;
    for (int x = 0; x < id; x++) {
        if (b->fields[x]) // if is not empty
            counter++;
    }
    return (id - counter);
}

int nth_to_id(int n, board* b) {
    int counter = 0;
    for (int x = 0; x < 9; x++) {
        if (!b->fields[x]) {
            if (counter == n)
                return x;
            counter++;
        }
    }
    return 0; // unreachable
}

status test(board* b) {
    for (int x = 0; x < 3; x++) {
        // vertical
        int first = b->fields[P(x, 0)];
        if (first == b->fields[P(x, 1)] && first == b->fields[P(x, 2)] && first != 0)
            return first;

        // horizontal
        first = b->fields[P(0, x)];
        if (first == b->fields[P(1, x)] && first == b->fields[P(2, x)] && first != 0)
            return first;
    }

    // diagonal
    int first = b->fields[P(0, 0)];
    if (first == b->fields[P(1, 1)] && first == b->fields[P(2, 2)] && first != 0)
        return first;

    first = b->fields[P(0, 2)];
    if (first == b->fields[P(1, 1)] && first == b->fields[P(2, 0)] && first != 0)
        return first;

    for (int i = 0; i < 9; i++) {
        if (b->fields[i] == EMPTY)
            return PLAYING;
    }
    return DRAW;
}
