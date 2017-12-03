#include <SDL2/SDL.h>
#include "common.c"

SDL_Window * window;
SDL_Renderer * renderer;
SDL_Texture * sprite_texture;
SDL_Texture * font_texture;
const int tile_size = 32;

typedef u8 Tile;

typedef struct {
    int type;
    int x;
    int y;
} Enemy;

typedef struct {
    int x;
    int y;
    int health;
    bool has_key;
} Player;

typedef struct {
    int x;
    int y;
} Exit;

typedef struct {
    Tile * tiles;
    int width;
    int height;
    Enemy * enemies;
    int enemy_count;
    Player player;
    Exit exit;
} Level;

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

void destroy_level(Level * level) {
    free(level->tiles);
    free(level->enemies);
    free(level);
}

Level * generate_level(int width, int height) {
    Level * level = calloc(1, sizeof(Level));
    if (!level) panic_exit("Could not allocate level.");

    int tile_count = width * height;
    level->tiles = calloc(tile_count, sizeof(Tile));
    if (!level->tiles) panic_exit("Could not allocate tiles.");
    level->width = width;
    level->height = height;

    for (int x = 0; x < level->width; ++x) {
        level->tiles[x + 0          * width] = WALL;
        level->tiles[x + (height-1) * width] = WALL;
    }
    for (int y = 0; y < level->height; ++y) {
        level->tiles[0         + y * width] = WALL;
        level->tiles[(width-1) + y * width] = WALL;
    }

    level->enemy_count = random_int_range(2, 5);
    level->enemies = calloc(level->enemy_count, sizeof(Enemy));
    if (!level->enemies) panic_exit("Could not allocate enemies.");
    for (int i = 0; i < level->enemy_count; ++i) {
        int x = random_int_range(1, width - 2);
        int y = random_int_range(1, height - 2);
        if (level->player.x == x && level->player.y == y) {
            ++i;
            continue;
        }
        level->enemies[i] = (Enemy) {
            .type = SPIDER,
            .x = x,
            .y = y
        };
        level->tiles[x + y * width] = FLOOR;
    }

    level->player.x = random_int_range(1, width - 2);
    level->player.y = random_int_range(1, height - 2);
    level->tiles[level->player.x + level->player.y * width] = FLOOR;

    for (int y = 1; y < level->height - 1; ++y) {
        for (int x = 1; x < level->width - 1; ++x) {
            bool enemy_here = false;
            for (int e = 0; e < level->enemy_count; ++e) {
                if (level->enemies[e].x == x && level->enemies[e].y == y) {
                    enemy_here = true;
                    break;
                }
            }
            if (enemy_here) continue;
            if (level->player.x == x && level->player.y == y) continue;
            level->tiles[x + y * width] = chance(0.2f) ? WALL : FLOOR;
        }
    }

    return level;
}

void update_level(Level * level, int direction) {}

void draw_level(Level * level) {
    for (int y = 0; y < level->height; ++y) {
        for (int x = 0; x < level->width; ++x) {
            draw_sprite(level->tiles[x + y * level->width],
                x * tile_size,
                y * tile_size);
        }
    }
    for (int i = 0; i < level->enemy_count; ++i) {
        draw_sprite(level->enemies[i].type,
            level->enemies[i].x * tile_size,
            level->enemies[i].y * tile_size);
    }
    draw_sprite(PLAYER,
        level->player.x * tile_size,
        level->player.y * tile_size);
}

int main(int argc, char ** argv) {
    setvbuf(stdout, 0, 0, _IONBF);
    setvbuf(stdin,  0, 0, _IONBF);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        panic_exit("Could not initialise SDL2.\n(%s)", SDL_GetError());
    }

    Level * level = generate_level(16, 16);

    int window_width = 16 * tile_size;
    int window_height = 16 * tile_size;

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
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    seed_rng(~SDL_GetPerformanceCounter(), SDL_GetTicks());

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

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }
        destroy_level(level);
        level = generate_level(16, 16);
        SDL_RenderClear(renderer);
        draw_level(level);
        SDL_RenderPresent(renderer);
        SDL_Delay(300);
    }
}
