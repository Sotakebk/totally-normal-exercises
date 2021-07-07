#ifndef FTREEH
#define FTREEH

/* What is this format, anyway?
 *
 * Basically, we abstract a byte to contain two cells of data first.
 *
 * Data is arranged in such a way, that:
 * - if this is the first move, and the CPU moves first, the first cell contains the initial move;
 * - either after this cell or at the start of the array, there are (9 - depth) CPU responses to each user move,
 *      where user move is considered as an n-th empty field counted from 0;
 * - if the CPU move ends the game, the isend bit is set to 1; if the users move that would serve as an index
 *      to a CPU move ends the game, the isend bit is set to 1;
 * - after each group of responses, there is another group of responses of size (9 - depth - K) where
 *      K equals number of responses that have the isend bit set to 1; depth is increased by 2 after each group
 *
 * Visualized layout in memory, where number is the depth (capped at 2 for simplification):
 * 0 - has 4 branches max
 * 1 - has 3 branches max
 * 2 - has 2 branches max
 * 3 - as there is only one branch possible, there is no need to keep these in memory
 * 0 1 2 3 3 2 3 2 3 1 2...
 * R 0 0 0 1 1 0 2 0 1 0... <- branches numbered as seen by their roots
*/

typedef unsigned char ubyte;

typedef struct ftree {
    // how many bytes there are allocated for data
    int real_length;
    // how many 'datalets' there are in the array
    int data_length;
    ubyte* data;
} ftree;

// allocate memory
extern ftree* ftree_create(int data_length);

// free memory
extern void ftree_delete(ftree* p);

// save ftree to file
extern int ftree_save(ftree* p, const char* name);

// load ftree from file
extern ftree* ftree_load(const char* name);

// writing to the ftree
extern int ftree_write(ftree* tree, int endflag, int move);

// reading from ftree
extern int ftree_get_move(ftree* tree, int position);
extern int ftree_get_endflag(ftree* tree, int position);

// struct used for easier walking through the structure
typedef struct thelper {
    int offset;
    int depth;
    ftree* tree_pointer;
} thelper;

extern thelper thelper_create(ftree* tree);

// called on user move, returns CPU response and moves to according branch down
// call with -1 if CPU moves first, and this is the CPU tree
extern int thelper_move(thelper* iterator, int move);

#endif
