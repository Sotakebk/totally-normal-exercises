# Cooler tic-tac-toe
Why calculate something many times when you can just precalculate everything and fit it into a tiny binary file?

This is a bit like the simple version, but with a twist: this time, we save the choice tree as a flat array of bytes. The two files for state trees have 36 and 388 bytes for AI-first and player-first games, respectively.

Each state is a group of moves, indexed by the user-picked field considered as n-th empty field, so the first branch has 9 (or 8, if AI moved first) nibble-moves, then 9 branches of 7 moves, and 7 branches of 5 moves and so on.

More comments and explanations inside.
