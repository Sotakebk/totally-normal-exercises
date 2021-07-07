# Cooler tic-tac-toe
Why calculate something many times when you can just precalculate it and fit it all into a tiny binary format?

This is a bit like the simple tic-tac-toe, but with a twist: this time, we save the choice tree as a flat array of bytes. The two flattened trees have 36 and 388 bytes for AI-first and player-first games, respectively.

Each move is stored in a 4-byte nibble, stored in series such that each branch is stored as a bunch of nibbles, count of which is equal to free spaces on the board. for a given state.

More comments and explanations inside.
