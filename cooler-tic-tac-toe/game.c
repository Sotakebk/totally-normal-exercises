#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "ftree.h"

// ugly disgusting
#ifdef _WIN32
#define clrscr() system("cls") // ewww
#else
#define clrscr() printf("\e[1;1H\e[2J")
#endif

ftree* cpu_first;
ftree* user_first;

char get_sym(int symbol, int x, int y, board* b) {
    int value = b->fields[P(x, y)];

    if (value == 1)
        return (symbol ? 'o' : 'x');
    else if (value == 2)
        return (symbol ? 'x' : 'o');

    return (char)(48 + y * 3 + x);
}

void draw_row(int sym, int y, board* b) {
    char c1, c2, c3;
    c1 = get_sym(sym, 0, y, b);
    c2 = get_sym(sym, 1, y, b);
    c3 = get_sym(sym, 2, y, b);

    printf("   |   |   \n");
    printf(" %c | %c | %c \n", c1, c2, c3);
    printf("   |   |   \n");
}

void draw_line(void) {
    printf("---+---+---\n");
}

void draw_board(int sym, board* b) {
    clrscr();
    draw_row(sym, 0, b);
    draw_line();
    draw_row(sym, 1, b);
    draw_line();
    draw_row(sym, 2, b);
    printf("=======I/O=======\n");
}

int input_yn(const char* text) {
    printf("%s\n", text);
    char c;
    if (scanf(" %c", &c) == 1)
        return (c == 'y');
    return 0;
}

int input_value(void) {
    while (1)
    {
        printf("Please enter a number\n");
        int value = 0;
        if (scanf(" %d", &value) != 1)
            return 0;

        if (value > 8 || value < 0)
            printf("Value out of bounds! must be in range <0, 8>, got: %d\n", value);
        else
            return value;
    }
}

void game(void) {
    board b = { .fields = {0,0,0,0,0,0,0,0,0} };

    draw_board(0, &b);

    int userO = input_yn("User plays as O? y/n");
    int cpumove = !input_yn("User makes first move? y/n");

    // prepare game vars here

    // to debug/see if the file format is read correctly, check helper.depth and helper.offset every move
    thelper helper = thelper_create((cpumove) ? cpu_first : user_first);
    int user_move = -1;

    while (1) {
        if (cpumove) {
            // CPU moves here
            int move = 0;
            if (helper.depth != 8) // in this case, the move is the only empty field anyway
                move = thelper_move(&helper, user_move);
            move = nth_to_id(move, &b);
            b.fields[move] = CPU;

            draw_board(userO, &b);
            cpumove = 0;
        }
        else {
            int point = input_value();
            if (b.fields[point]) {
                printf("Field already taken, please enter a different value\n");
            }
            else {
                // user moves here
                user_move = id_to_nth(point, &b);
                b.fields[point] = PLAYER;

                draw_board(userO, &b);
                cpumove = 1;
            }
        }

        status out = test(&b);
        if (out == HWIN) {
            printf("Player wins!\n");
            break;
        }
        else if (out == CPUWIN) {
            printf("Computer wins!\n");
            break;
        }
        else if (out == DRAW) {
            printf("Draw!\n");
            break;
        }
    }
}

int play(void) {
    int ret = 0;

    user_first = ftree_load(USER_FIRST_NAME);
    if (user_first == NULL)
        ret = -2;
    cpu_first = ftree_load(CPU_FIRST_NAME);
    if (cpu_first == NULL)
        ret = -2;

    if (ret) {
        if (!user_first)
            ftree_delete(user_first);
        if (!cpu_first)
            ftree_delete(cpu_first);
        return ret;
    }

    do {
        game();
    } while (input_yn("Play again? y/n"));

    ftree_delete(user_first);
    ftree_delete(cpu_first);

    return 0;
}
