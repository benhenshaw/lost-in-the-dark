# FLAGS="game.c -o game -O2 -Wall"
FLAGS="new_game_plus.c -o game -O2 -Wall"

clang $FLAGS -framework SDL2
# gcc $FLAGS -mwindow -lmingw32 -lSDL2main -lSDL2

if [[ $? -eq 0 ]]; then
    ./game
    rm game
fi
