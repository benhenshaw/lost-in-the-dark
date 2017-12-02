#include <SDL2/SDL.h>
#include "common.c"

SDL_Window * window;
SDL_Renderer * renderer;
SDL_Texture * sprite_texture;
SDL_Texture * font_texture;

void draw_number(int number, int x, int y) {
    const int font_width  = 8;
    const int font_height = 8;
    char string[64];
    snprintf(string, 64, "%d", number);
    for (char * c = string; *c; ++c) {
        int sx = (*c - '0') * font_width;
        SDL_RenderCopy(renderer, font_texture,
            &(SDL_Rect){ sx, 0, font_width, font_height },
            &(SDL_Rect){  x, y, font_width, font_height });
        x += font_width;
    }
}

const int level_width = 16;
const int level_height = 16;
const int tile_size = 32;

const int window_width  = (level_width+2)  * tile_size;
const int window_height = (level_height+2) * tile_size;

typedef struct {
    int score;
    int levels_cleared;
    int enemies_defeated;
    int health;
    bool key_found;
} Session;

enum {
    UP = 1,
    DOWN,
    LEFT,
    RIGHT,
};

enum {
    FLOOR = 1,
    SPIDER,
    SPIKES,
    PLAYER,
    WALL,
    EXIT,
    LOCK,
    KEY,
    GOLD_SMALL,
    GOLD_LARGE,
};

struct {
    u16 x, y;
    u8 r, g, b;
    u8 flags;
} sprite_table[] = {
    [0] = {},
    [FLOOR]       = { 10,  7, 124, 175, 194, 0 },
    [SPIDER]      = {  3,  3, 186, 139, 175, 0 },
    [SPIKES]      = { 11,  6, 124, 175, 194, 0 },
    [PLAYER]      = {  3,  0, 161, 181, 108, 0 },
    [WALL]        = {  2,  8, 216, 216, 216, 0 },
    [EXIT]        = {  5,  7, 216, 216, 216, 0 },
    [LOCK]        = {  4,  7, 216, 216, 216, 0 },
    [KEY]         = {  2, 11, 247, 202, 136, 0 },
    [GOLD_SMALL]  = {  0,  9, 247, 202, 136, 0 },
    [GOLD_LARGE]  = {  0, 10, 247, 202, 136, 0 },
};

void draw_sprite(int sprite_index, int x, int y) {
    if (sprite_index) {
        int sx = sprite_table[sprite_index].x * tile_size;
        int sy = sprite_table[sprite_index].y * tile_size;
        int r = sprite_table[sprite_index].r;
        int g = sprite_table[sprite_index].g;
        int b = sprite_table[sprite_index].b;
        SDL_SetTextureColorMod(sprite_texture, r, g, b);
        SDL_RenderCopyEx(renderer, sprite_texture,
            &(SDL_Rect){ sx, sy, tile_size, tile_size },
            &(SDL_Rect){  x,  y, tile_size, tile_size },
            0, 0, 0);
        SDL_SetTextureColorMod(sprite_texture, 255, 255, 255);
    }
}

typedef struct {
    u8 type;
    u8 entity;
    u16 flags;
} Tile;

void generate_level(Tile * tiles, int width, int height) {
    for (int x = 0; x < width; ++x) {
        tiles[x + 0          * width] = (Tile){ .type = WALL };
        tiles[x + (height-1) * width] = (Tile){ .type = WALL };
    }
    for (int y = 0; y < height; ++y) {
        tiles[0         + y * width] = (Tile){ .type = WALL };
        tiles[(width-1) + y * width] = (Tile){ .type = WALL };
    }

    for (int y = 1; y < height-1; ++y) {
        for (int x = 1; x < width-1; ++x) {
            Tile tile = {};

            tile.type = chance(0.2f) ? WALL : FLOOR;

            if (tile.type == FLOOR) {
                if      (chance(0.05f)) tile.entity = GOLD_SMALL;
                else if (chance(0.01f)) tile.entity = GOLD_LARGE;
                else if (chance(0.03f)) tile.entity = SPIDER;
                else if (chance(0.02f)) tile.type = SPIKES;
            }

            tiles[x + y * width] = tile;
        }
    }

    {
        Tile t = { .type = EXIT, .entity = LOCK };
        int rx = random_int_range(1, width - 2);
        int ry = random_int_range(1, height - 2);
        tiles[rx + ry * width] = t;
    }

    {
        Tile t = { .type = FLOOR, .entity = KEY };
        int rx = random_int_range(1, width - 2);
        int ry = random_int_range(1, height - 2);
        tiles[rx + ry * width] = t;
    }

    {
        Tile t = { .type = FLOOR, .entity = PLAYER };
        int rx = random_int_range(1, width - 2);
        int ry = random_int_range(1, height - 2);
        tiles[rx + ry * width] = t;
    }

    putchar('s');
}

bool update_level(Tile * tiles, int width, int height, int player_respection, Session * session) {
    Tile new_tiles[width * height];
    memcpy(new_tiles, tiles, width * height * sizeof(*tiles));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Tile * old  = &tiles[x + y * width];
            Tile * tile = &new_tiles[x + y * width];

            if (old->type == EXIT) {
                tile->entity = session->key_found ? 0 : LOCK;
            }

            if (old->entity == PLAYER) {
                int new_x = x;
                int new_y = y;
                if (player_respection == UP)    --new_y;
                if (player_respection == DOWN)  ++new_y;
                if (player_respection == LEFT)  --new_x;
                if (player_respection == RIGHT) ++new_x;

                Tile * new_pos = &new_tiles[new_x + new_y * width];
                if (new_pos->type == WALL || new_pos->entity == LOCK) {
                    continue;
                }

                if (new_pos->type == SPIKES) {
                    session->health -= 1;
                } else if (new_pos->type == EXIT) {
                    return true;
                }

                if (new_pos->entity == GOLD_SMALL) {
                    session->score += 3;
                } else if (new_pos->entity == GOLD_LARGE) {
                    session->score += 20;
                } else if (new_pos->entity == KEY) {
                    session->key_found = true;
                } else if (new_pos->entity == SPIDER) {
                    session->enemies_defeated += 1;
                }

                new_pos->entity = PLAYER;
                tile->entity = 0;
            } else if (old->entity == SPIDER) {
                int new_x = x;
                int new_y = y;
                int respection = random_int_range(UP, RIGHT);
                if (respection == UP)    --new_y;
                if (respection == DOWN)  ++new_y;
                if (respection == LEFT)  --new_x;
                if (respection == RIGHT) ++new_x;

                Tile * new_pos = &new_tiles[new_x + new_y * width];
                if (new_pos->type == WALL ||
                    new_pos->entity == LOCK ||
                    new_pos->entity == PLAYER ||
                    new_pos->entity == KEY) {
                    continue;
                }

                new_pos->entity = SPIDER;
                tile->entity = 0;
            }
        }
    }
    memcpy(tiles, new_tiles, width * height * sizeof(*tiles));
    return false;
}


int io_thread(void * data) {
    while (true) {
        char * c = data;
        *c = getchar();
        SDL_Delay(10);
    }
}

int main(int argc, char ** argv) {
    setvbuf(stdout, 0, 0, _IONBF);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        panic_exit("Could not initialise SDL2.\n(%s)", SDL_GetError());
    }

    window = SDL_CreateWindow("",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        window_width, window_height, 0);
    if (window == NULL) {
        panic_exit("Could not create window.\n(%s)", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        panic_exit("Could not create renderer.\n(%s)", SDL_GetError());
    }
    SDL_RenderSetLogicalSize(renderer, window_width, window_height);
    SDL_RenderSetIntegerScale(renderer, true);

    seed_rng(~SDL_GetPerformanceCounter(), SDL_GetTicks());

    char response = '\0';
    SDL_CreateThread(io_thread, "io", &response);

    {
        SDL_Surface * surface = SDL_LoadBMP("sheet.bmp");
        if (surface == NULL) {
            panic_exit("Could not load sprite sheet.\n(%s)", SDL_GetError());
        }
        sprite_texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (sprite_texture == NULL) {
            panic_exit("Could not create sprite sheet texture.\n(%s)", SDL_GetError());
        }
        SDL_FreeSurface(surface);
    }

    {
        SDL_Surface * surface = SDL_LoadBMP("digits.bmp");
        if (surface == NULL) {
            panic_exit("Could not load sprite sheet.\n(%s)", SDL_GetError());
        }
        font_texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (sprite_texture == NULL) {
            panic_exit("Could not create sprite sheet texture.\n(%s)", SDL_GetError());
        }
        SDL_FreeSurface(surface);
    }

    Tile tiles[level_width * level_height];
    generate_level(tiles, level_width, level_height);
    Session current_session = {
        .health = 10
    };

    int end_time;
    bool otherPlayerFinished = false;
    bool game_over = false;
    
    while (true) {
        SDL_Event event;
        //end_time = SDL_GetTicks() + 30 * 1000;
        

        if(response == 's')
        {
            end_time = SDL_GetTicks() + 30 * 1000;
            response = '\0';
        } 
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);

            if (event.type == SDL_KEYDOWN) {
                SDL_Scancode sc = event.key.keysym.scancode;
                if (sc == SDL_SCANCODE_UP || sc == SDL_SCANCODE_W) {
                    putchar('u');
                }else if (sc == SDL_SCANCODE_DOWN || sc == SDL_SCANCODE_S) {
                    putchar('d');
                } else if (sc == SDL_SCANCODE_LEFT || sc == SDL_SCANCODE_A) {
                    putchar('l');
                } else if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) {
                    putchar('r');
                } else if (sc == SDL_SCANCODE_RETURN && !end_time) {
                    putchar('s');
                    end_time = SDL_GetTicks() + 30 * 1000;
                } else if (sc == SDL_SCANCODE_R) {
                    current_session = (Session){.health = 10};
                    generate_level(tiles, level_width, level_height);
                }
                fflush(stdout);
            }
            
        }
        

        {
            int player_direction = 0;
            if (response == 'f') otherPlayerFinished = true;
            if (response == 'u') player_direction = UP;   else
            if (response == 'd') player_direction = DOWN; else
            if (response == 'l') player_direction = LEFT; else
            if (response == 'r') player_direction = RIGHT;
            if (response == 'e') {
                game_over = true;
                //generate_level(tiles, level_width, level_height); //TODO: Game Over

            }
            
            if (player_direction) {
                bool finished = update_level(tiles, level_width, level_height,
                    player_direction, &current_session);
                if(finished) putchar('f');
                if(finished && otherPlayerFinished)
                {
                    generate_level(tiles, level_width, level_height);
                    current_session.levels_cleared += 1;
                    current_session.key_found = false;
                }
            }
            response = '\0';

            if (SDL_GetTicks() > end_time && end_time) putchar('e');
        }


        SDL_SetRenderDrawColor(renderer, 29, 32, 33, 255);
        SDL_RenderClear(renderer);
        if (!game_over) {
            for (int y = 0; y < level_height; ++y) {
                for (int x = 0; x < level_width; ++x) {
                    Tile tile = tiles[x + y * level_width];
                    draw_sprite(tile.type, (x+1) * tile_size, (y+1) * tile_size);
                    draw_sprite(tile.entity, (x+1) * tile_size, (y+1) * tile_size);
                }
            }

            draw_sprite(PLAYER, 32, 0);
            draw_number(current_session.health, 72, 12);
            draw_sprite(GOLD_SMALL, 160, 0);
            draw_number(current_session.score, 200, 12);
            draw_sprite(EXIT, 288, 0);
            draw_number(current_session.levels_cleared, 328, 12);
            draw_sprite(SPIDER, 416, 0);
            draw_number(current_session.enemies_defeated, 456, 12);
            if (current_session.key_found) draw_sprite(KEY, 512, 0);
        }
        SDL_RenderPresent(renderer);

    }
}
