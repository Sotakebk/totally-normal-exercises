#include <stdlib.h>
#include <stdio.h>
#include "ftree.h"

ftree* ftree_create(int data_length) {
    ftree* f = malloc(sizeof(ftree));
    if (!f)
        return NULL;

    // amount of bytes needed for specified amount of nibbles
    int real_length = (data_length / 2) + (data_length % 2);

    f->data = malloc(real_length * sizeof(ubyte));
    if (!f->data) {
        free(f);
        return NULL;
    }

    for (int x = 0; x < real_length; x++)
        f->data[x] = 0;

    f->data_length = 0;
    f->real_length = real_length;

    return f;
}

void ftree_delete(ftree* p) {
    free(p->data);
    free(p);
}

int ftree_save(ftree* p, const char* name) {
    FILE* file;
    file = fopen(name, "wb+");
    if (!file)
        return -1;

    int ret = 0;

    // count of nibbles
    ret = (fwrite(&(p->data_length), sizeof(int), 1, file) != 1);
    // minimal number of bytes needed to save the nibbles
    int real_length = (p->data_length / 2) + (p->data_length % 2);
    // write the data
    ret |= (fwrite(p->data, sizeof(ubyte), real_length, file) != real_length);
    ret |= fflush(file);
    ret |= fclose(file);
    return ret; // returns non-0 if anything above failed
}

ftree* ftree_load(const char* name) {
    ftree* p = malloc(sizeof(ftree));
    if (!p)
        return NULL;

    FILE* file;
    file = fopen(name, "rb");
    if (!file) {
        ftree_delete(p);
        return NULL;
    }

    // declared count of nibbles
    int fcode = (fread(&(p->data_length), sizeof(int), 1, file) != 1);

    // measure count of bytes between declaration and EOF
    fcode |= fseek(file, 0, SEEK_END);
    int real_l = ftell(file) - sizeof(int);
    rewind(file);
    fcode |= fseek(file, sizeof(int), SEEK_CUR);

    // this is count of bytes as declared by the file
    int decl_l = (p->data_length / 2) + (p->data_length % 2);

    // DEBUG
    //if (real_l != decl_l) printf("real_l != decl_l (%d != %d)", real_l, decl_l);

    p->real_length = decl_l;

    p->data = malloc(decl_l * sizeof(ubyte));
    if (!p->data) {
        ftree_delete(p);
        return NULL;
    }
    else {
        fcode |= (fread(p->data, sizeof(ubyte), decl_l, file) != decl_l);
    }

    fcode |= fclose(file);
    if (fcode) {
        ftree_delete(p);
        return NULL;
    }
    return p;
}

int ftree_write(ftree* tree, int endflag, int move) {
    if (tree->data_length / 2 > tree->real_length)
        return 1;

    int pos = tree->data_length;
    ubyte byte = tree->data[pos / 2];

    // xxxxEMMM
    // one bit as an end flag, and three bits for the next move
    ubyte nibble = ((endflag & 1) << 3) | (move & 7);
    // we only lose the value '8', which does not exist on the choice tree anyway,
    // as it is only possible on the first CPU move, but that does not matter

    // first of the pair is on the high bits
    if (pos % 2) {
        //EMMMemmm
        byte |= ((nibble & 15) << 4);
    }
    else { // B
        //emmmEMMM
        byte |= nibble;
    }
    tree->data[pos / 2] = byte;

    tree->data_length++;

    return 0;
}

int get_nth(ftree* tree, int position) {
    if (position >= tree->data_length || position < 0) return 0; // out of bounds
    ubyte b = tree->data[position / 2];

    if (position % 2)
        return (b >> 4) & 15; // binary AND not even needed here, but is comforting
    else {
        return b & 15;
    }
}

int ftree_get_move(ftree* tree, int position) {
    return get_nth(tree, position) & 7;
}

int ftree_get_endflag(ftree* tree, int position) {
    return get_nth(tree, position) >> 3;
}

thelper thelper_create(ftree* tree) {
    thelper it = {
        .depth = 0,
        .offset = 0,
        .tree_pointer = tree
    };

    return it;
}

int get_branch_offset(ftree* tree, int start_offset, int depth, int nth) {
    if (depth > 7)
        return start_offset;
    // if depth = 8 or 9, these branches do not exist, and as such their size is 0

    // we start at a point
    int offset = start_offset;

    // how many branches of lower depth are there?
    int scount = 0;
    for (int x = 0; x < nth; x++) {
        if (!ftree_get_endflag(tree, start_offset + x))
            scount++;
    }

    // move to the start of subbranches
    offset += (9 - depth);

    int ndepth = depth + 2;
    // move over the skipped branches
    for (int x = 0; x < scount; x++)
        offset = get_branch_offset(tree, offset, ndepth, (9 - ndepth));

    return offset;
}

int thelper_move(thelper* iterator, int move) {
    // edge case - start of the game
    if (move == -1) {
        iterator->offset = 1;
        iterator->depth = 1;
        return ftree_get_move(iterator->tree_pointer, 0);
    }

    // getting the value is straightforward
    int ret = ftree_get_move(iterator->tree_pointer, iterator->offset + move);

    // moving into the new position now
    iterator->offset = get_branch_offset(iterator->tree_pointer, iterator->offset, iterator->depth, move);
    iterator->depth += 2;

    return ret;
}
