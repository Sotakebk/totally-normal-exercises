#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "ftree.h"

// to use this, just pipe stdout into a text file
//#define PRINT_MAP

// this code does not generate most optimal choice tree
// there is still room for improvement, but this works and is not the focus of this project anyway

typedef struct node node;

// this way the struct is (likely) properly padded on both x86 and x64
// figuring out data padding was the reason why free(node*) caused a heap corruption
// was painful, but also funny
struct node {
    int end;
    int next;
    node* subnodes[];
};

node* player_first;
node* cpu_first;

void delete_node(node* n, int sub_count) {
    if (!n) return;

    for (int x = 0; x < sub_count; x++) {
        delete_node(n->subnodes[x], sub_count - 1);
    }
    free(n);
}

node* new_node(int sub_count) {
    node* b = malloc(2 * sizeof(int) + sub_count * sizeof(node*));
    if (!b)
        return NULL;

    b->end = 0;
    b->next = 255; // debug value, should never be reached
    // make sure all pointers are initialized, so we don't call free(*p) on something
    for (int x = 0; x < sub_count; x++) {
        b->subnodes[x] = NULL;
    }

    for (int x = 0; x < sub_count; x++) {
        b->subnodes[x] = new_node(sub_count - 1);

        // if allocation of branch failed, delete this, return NULL
        if (!b->subnodes[x]) {
            delete_node(b, sub_count);
            return NULL;
        }
    }

    return b;
}

int prepare_graphs(void) {
    player_first = new_node(9);
    cpu_first = new_node(9);

    if (!player_first || !cpu_first) {
        delete_node(player_first, 9);
        delete_node(cpu_first, 9);
        return -1;
    }

    return 0;
}

void delete_graphs(void) {
    delete_node(player_first, 9);
    player_first = NULL;
    delete_node(cpu_first, 9);
    cpu_first = NULL;
}

// minimax
int minimax(int depth, int cpumove, node* n, board* board) {
    status status = test(board);
    if (status != PLAYING) {
        n->end = 1;
        if (status == HWIN)
            return -10;
        else if (status == CPUWIN)
            return 10;
        else
            return 0;
    }

    int i = -1;
    int score = 0;
    // max
    if (cpumove) {
        score = -128;
        for (int x = 0; x < (9 - depth); x++) {
            // virtually iterating over empty fields
            // x = nth empty field; id = corresponding field ID
            int id = nth_to_id(x, board);
            board->fields[id] = CPU;
            int nscore = minimax(depth + 1, 0, n->subnodes[x], board) - depth;
            if (nscore > score) {
                score = nscore;
                i = x;
            }
            board->fields[id] = EMPTY;
        }
    }

    // min
    else {
        score = 127;
        for (int x = 0; x < (9 - depth); x++) {
            // virtually iterating over empty fields
            int id = nth_to_id(x, board);
            board->fields[id] = PLAYER;
            int nscore = minimax(depth + 1, 1, n->subnodes[x], board) + depth;
            if (nscore < score) {
                score = nscore;
                i = x;
            }
            board->fields[id] = EMPTY;
        }
    }
    n->next = i;

    // check if this node really needs any subnodes
    {
        int id = nth_to_id(i, board);
        board->fields[id] = (cpumove ? CPU : PLAYER);
        n->end = (test(board) != PLAYING || depth == 8);
        board->fields[id] = EMPTY;
    }

    return score;
}

int evaluate(void) {
    board* b = malloc(sizeof(board));
    if (!b) return -1;
    for (int x = 0; x < 9; x++) {
        b->fields[x] = EMPTY;
    }

    minimax(0, 0, player_first, b);
    minimax(0, 1, cpu_first, b);
    free(b);
    return 0;
}

/* How to save this?
 * for cpu_first:
 * save the first nibble as first cpu move;
 * move into sub[move]
 * we are now in player move space, we need to print out every move:
 * for each branch below, save a nibble as the move the CPU will make
 * move into sub[x]
 * we are in cpu move space again, we assume that move leading here was done, move into sub[move]...
 *
 * for player_first:
 * we start in a player move space, print out every move from branch below
 * move into sub[x]
 * we are in cpu space...
 */

void save_node(ftree* tree, node* b, int is_cpu, int depth) {
    if (depth > 8 || b->end)
        return;

    if (is_cpu) {
        save_node(tree, b->subnodes[b->next], 0, depth + 1);
    }
    else {
#ifdef PRINT_MAP
        printf("%d\tbranch is at offset start:\t%d\n\tsubs[]:\n", depth, tree->data_length);
#endif
        for (int x = 0; x < (9 - depth); x++) {
            int end = (b->subnodes[x]->end || depth > 5);
#ifdef PRINT_MAP
            printf("val:\t%d\tend:\t%d\n", b->subnodes[x]->next, end);
#endif
            ftree_write(tree, end, b->subnodes[x]->next);
        }
        for (int x = 0; x < (9 - depth); x++) {
            save_node(tree, b->subnodes[x], 1, depth + 1);
        }
}
}

int save_graph(node* b, int is_cpu, const char* name) {
    ftree* tree = ftree_create(59049); //3^10, just to be sure :^)
    if (!tree)
        return -1;

#ifdef PRINT_MAP
    printf("Map layout for: %s (is_cpu: %d)\n", name, is_cpu);
#endif

    if (is_cpu) {
        ftree_write(tree, 0, b->next);
        save_node(tree, b->subnodes[b->next], 1, 1);
    }
    else
        save_node(tree, b, 0, 0);

    int ret = ftree_save(tree, name);
    return ret;
}

int save(void) {
    int ret = save_graph(cpu_first, 1, CPU_FIRST_NAME);
    if (ret != 0) return ret;
    ret = save_graph(player_first, 0, USER_FIRST_NAME);
    return ret;
}

int pregenerate() {
    printf("Preparing graphs...\n");
    if (prepare_graphs()) return -1;
    printf("Evaluating...\n");
    int ret = evaluate();
    printf("Saving...\n");
    ret |= save();
    printf("Cleaning up...\n");
    delete_graphs();
    return ret;
}
