# FLAGS="djinn.c -o djinn -O2"
# FLAGS="second_prototype.c -o djinn -O2 -Wall"
FLAGS="djinnet.c -o djinn -O2 -Wall"

clang $FLAGS -framework SDL2
# gcc $FLAGS -mwindow -lmingw32 -lSDL2main -lSDL2

# if [[ $? -eq 0 ]]; then
#     ./djinn
#     rm djinn
# fi
