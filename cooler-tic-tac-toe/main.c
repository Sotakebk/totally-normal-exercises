#include <stdio.h>

#include "common.h"
#include "pregen.h"
#include "game.h"

int main(void) {
    if (!files_exist()) {
        printf("Regenerating choice trees...\n");
        int ret = pregenerate();
        if (ret) {
            printf("Failed to pregenerate choice trees.");
            return ret;
        }
    }
    return play();
}
