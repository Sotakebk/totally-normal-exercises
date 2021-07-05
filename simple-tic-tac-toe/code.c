#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// ugly disgusting
#ifdef _WIN32
#define clrscr() system("cls") // ewww
#else
#define clrscr() printf("\e[1;1H\e[2J")
#endif

/*
 *   0 1 2 X
 * 0 0 1 2
 * 1 3 4 5
 * 2 6 7 8
 * Y
 */

typedef enum {
    EMPTY = 0,
    PLAYER = 1,
    CPU = 2
} field;

typedef enum
{
    PLAYING = 0,
    HWIN = 1,
    CPUWIN = 2,
    DRAW = 3
} status;

typedef uint16_t state;
enum { END = 65534, UNSET = 65535 };

field getp(state s, int x, int y)
{
    static const int pow[9] = { 1, 3, 9, 27, 81, 243, 729, 2187, 6561 };
    int i = (y * 3 + x);
    return (s / pow[i]) % 3;
}

state setp(state oldstate, int x, int y, int value)
{
    static const int pow[9] = { 1, 3, 9, 27, 81, 243, 729, 2187, 6561 };
    int p = (y * 3 + x);
    uint16_t newstate = 0;
    for (int i = 0; i < 9; i++) {
        int val = 0;
        if (i == p)
            val = (value & 3);
        else
            val = getp(oldstate, i, 0);
        newstate += val * pow[i];
    }
    return newstate;
}

status test(state s) {
    // vertical
    for (int x = 0; x < 3; x++) {
        int first = getp(s, x, 0);
        if (first == getp(s, x, 1) && first == getp(s, x, 2)) {
            if (first != 0)
                return first;
        }
    }

    // horizontal
    for (int y = 0; y < 3; y++) {
        int first = getp(s, 0, y);
        if (first == getp(s, 1, y) && first == getp(s, 2, y)) {
            if (first != 0)
                return first;
        }
    }

    // diagonal
    {
        int first = getp(s, 0, 0);
        if (first == getp(s, 1, 1) && first == getp(s, 2, 2)) {
            if (first != 0)
                return first;
        }
    }

    {
        int first = getp(s, 0, 2);
        if (first == getp(s, 1, 1) && first == getp(s, 2, 0)) {
            if (first != 0)
                return first;
        }
    }

    // still haven't returned anything, time to check if the board is full
    for (int i = 0; i < 9; i++) { // there must be a better way somehow
        if (getp(s, i, 0) == 0) // there still is at least one empty field
            return PLAYING;
    }
    return DRAW;
}

uint16_t* steps;
int evaluateStep(state s, int depth, int cpumove, int alpha, int beta) {
    status status = test(s);

    // check if this is the end and I don't know why, it doesn't even matter how hard you try
    if (status != PLAYING) {
        steps[s] = END;
        if (status == HWIN)
            return -10;
        else if (status == CPUWIN)
            return 10;
        else
            return 0;
    }

    // max
    int i = -1;
    int score = 0;
    if (cpumove) {
        score = INT8_MIN;
        for (int x = 0; x < 9; x++) {
            if (getp(s, x, 0) == 0) {
                state n = setp(s, x, 0, 2);
                int nscore = evaluateStep(n, depth + 1, 0, alpha, beta) - depth;
                if (nscore > score) {
                    score = nscore;
                    i = x;
                    if (score > beta) {
                        break;
                    }
                    if (score > alpha) alpha = score;
                }
            }
        }
    }
    // min
    else {
        score = INT8_MAX;
        for (int x = 0; x < 9; x++) {
            if (getp(s, x, 0) == 0) {
                state n = setp(s, x, 0, 1);
                int nscore = evaluateStep(n, depth + 1, 1, alpha, beta) + depth;
                if (nscore < score) {
                    score = nscore;
                    i = x;
                    if (score < alpha) {
                        //break;
                    }
                    if (score < beta) beta = score;
                }
            }
        }
    }
    state n = setp(s, i, 0, (cpumove ? 2 : 1));
    steps[s] = n;
    return score;
}

int prepare_steps(void) {
    steps = malloc(UINT16_MAX * sizeof(state));
    if (steps == NULL) return -1;

    for (int x = 0; x < UINT16_MAX; x++) {
        steps[x] = UNSET; // debug value
    }
    state n = 0;
    evaluateStep(n, 0, 0, INT16_MIN, INT16_MAX);
    evaluateStep(n, 0, 1, INT16_MIN, INT16_MAX);

    return 0;
}

char getsym(state s, int symbol, int x, int y) {
    int value = getp(s, x, y);

    if (value == 1)
        return (symbol ? 'o' : 'x');
    else if (value == 2)
        return (symbol ? 'x' : 'o');

    return (char)(48 + y * 3 + x);
}

void drawc(state s, int sym, int y) {
    printf("   |   |   \n");
    printf(" %c | %c | %c \n", getsym(s, sym, 0, y), getsym(s, sym, 1, y), getsym(s, sym, 2, y));
    printf("   |   |   \n");
}

void drawl(void) {
    printf("-----------\n");
}

void draw(state s, int sym)
{
    clrscr();
    drawc(s, sym, 0);
    drawl();
    drawc(s, sym, 1);
    drawl();
    drawc(s, sym, 2);
    printf("=======I/O=======\n");
}

int input_yn(const char* text)
{
    printf("%s\n", text);
    char c;
    if (scanf(" %c", &c) == 1)
        return (c == 'y');
    return 0;
}

int input_value(void)
{
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

int gameloop(void) {
    state s = 0;
    draw(s, 0);

    int userO = input_yn("User plays as O? y/n");

    int cpumove = !input_yn("User makes first move? y/n");

    while (1) {
        if (cpumove) {
            s = steps[(int)s];
            // debug
            if (s == 65534) return -1;
            if (s == 65535) return -2;
            draw(s, userO);
            cpumove = 0;
        }
        else {
            int point = input_value();
            if (getp(s, point, 0)) {
                printf("Field already taken, please enter a different value\n");
            }
            else {
                s = setp(s, point, 0, 1);
                draw(s, userO);
                cpumove = 1;
            }
        }

        status out = test(s);
        switch (out) {
        case(PLAYING):
            continue;
        case(HWIN):
            printf("Player wins!\n");
            break;
        case(CPUWIN):
            printf("Computer wins!\n");
            break;
        case(DRAW):
            printf("Draw!\n");
            break;
        }
    }
    return 0;
}

// returns:
// -1 - end of game state reached but test(s) did not catch it
// -2 - unreachable game state reached
// -3 - malloc failed
int main(void)
{
    if (prepare_steps() == -1) return -3;

    do {
        int code = gameloop();
        if (code != 0)
            return code;
    } while (input_yn("Play again? y/n"));

    return 0;
}
