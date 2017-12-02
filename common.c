/*
    common.c - Basic functions, types, io, error handling, maths
*/

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int uint;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef _Bool bool;
#define true 1
#define false 0

#define PI  3.1415926536f
#define PI2 6.2831853072f

#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define CLAMP(low, val, high) MAX(low, MIN(val, high))

#define RAW_CAST(type, var) (*(type *)&(var))

#define BIT(n) (1 << (n))

u64 xorshift128plus_random_seed[2] = { ~0, ~0 };

u64 xorshift128plus() {
    u64 x = xorshift128plus_random_seed[0];
    u64 const y = xorshift128plus_random_seed[1];
    xorshift128plus_random_seed[0] = y;
    x ^= x << 23;
    xorshift128plus_random_seed[1] = x ^ y ^ (x >> 17) ^ (y >> 26);
    return xorshift128plus_random_seed[1] + y;
}

void seed_rng(u64 a, u64 b) {
    xorshift128plus_random_seed[0] = a ^ a << 32;
    xorshift128plus_random_seed[1] = b ^ a << 32;
    for (int i = 0; i < 64; ++i) {
        xorshift128plus();
    }
}

int random_int() {
    return (int)xorshift128plus();
}

float random_float() {
    return ((float)xorshift128plus() / (float)UINT64_MAX);
}

float random_float_range(float low, float high) {
    float d = high - low;
    return random_float() * d + low;
}

int random_int_range(int low, int high) {
    float d = abs((high) - low) + 1;
    return random_float() * d + low;
}

bool chance(float likeliness) {
    return random_float() < likeliness;
}

void panic_exit(char * format, ...) {
    va_list args;
    va_start(args, format);
    char message[128];
    vsnprintf(message, 128, format, args);
    fprintf(stderr, "%.128s\n", message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", message, NULL);
    va_end(args);
    exit(1);
}

void issue_warning(char * format, ...) {
    va_list args;
    va_start(args, format);
    char message[128];
    vsnprintf(message, 128, format, args);
    fprintf(stderr, "%.128s\n", message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning!", message, NULL);
    va_end(args);
}

#define GFMT(x) _Generic((x),                 \
    bool:                     #x " = %d\n",   \
    char:                     #x " = %c\n",   \
    signed char:              #x " = %hhd\n", \
    unsigned char:            #x " = %hhu\n", \
    signed short:             #x " = %hd\n",  \
    unsigned short:           #x " = %hu\n",  \
    signed int:               #x " = %d\n",   \
    unsigned int:             #x " = %u\n",   \
    long int:                 #x " = %ld\n",  \
    unsigned long int:        #x " = %lu\n",  \
    long long int:            #x " = %lld\n", \
    unsigned long long int:   #x " = %llu\n", \
    float:                    #x " = %f\n",   \
    double:                   #x " = %f\n",   \
    long double:              #x " = %Lf\n",  \
    char *:                   #x " = %s\n",   \
    signed char *:            #x " = %p\n",   \
    unsigned char *:          #x " = %p\n",   \
    signed short *:           #x " = %p\n",   \
    unsigned short *:         #x " = %p\n",   \
    signed int *:             #x " = %p\n",   \
    unsigned int *:           #x " = %p\n",   \
    long int *:               #x " = %p\n",   \
    unsigned long int *:      #x " = %p\n",   \
    long long int *:          #x " = %p\n",   \
    unsigned long long int *: #x " = %p\n",   \
    float *:                  #x " = %p\n",   \
    double *:                 #x " = %p\n",   \
    long double *:            #x " = %p\n",   \
    void *:                   #x " = %p\n",   \
    default:                  "")
#define put(x) printf(GFMT(x),x)
