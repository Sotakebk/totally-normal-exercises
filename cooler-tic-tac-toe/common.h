#ifndef COMMONH
#define COMMONH

#define P(x,y) ((x)+((y)*3))

extern const char* CPU_FIRST_NAME;
extern const char* USER_FIRST_NAME;

// check if above exist
extern int files_exist(void);

typedef enum {
    EMPTY = 0,
    PLAYER = 1,
    CPU = 2
} field;

typedef struct board {
    field fields[9];
} board;

// field ID as nth empty field on a given board state
extern int id_to_nth(int id, board* b);

// nth empty field as ID on a given board state
extern int nth_to_id(int n, board* b);

/* examples:
 * 011102100
 * ID - 8
 * NTH - 2
 *
 * 000011110
 * ID - 3
 * NTH - 3
 *
 * 000000000
 * ID = NTH
 */

typedef enum {
    PLAYING = 0,
    HWIN = 1,
    CPUWIN = 2,
    DRAW = 3
} status;

// check board status
extern status test(board* b);

#endif
