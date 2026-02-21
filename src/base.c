#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) > (b) ? (a) : (b)))
#define CLAMP(a, b, c) (((b) > (c) ? (c) : (((b) < (a)) ? (a) : (b))))

typedef float Matrix4[16];
typedef float Matrix3[9];
typedef float Matrix2[4];

typedef struct {
    float x;
    float y;
    float z;
} Vec4;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    float x;
    float y;
} Vec2;

static inline Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

static inline void vec2_add_inplace(Vec2 *v, Vec2 other) {
    v->x += other.x;
    v->y += other.y;
}

static inline Vec2 vec2_scale(Vec2 v, float scalar) {
    return (Vec2){v.x * scalar, v.y * scalar};
}

static inline void vec2_scale_inplace(Vec2 *v, float scalar) {
    v->x *= scalar;
    v->y *= scalar;
}

static inline float vec2_length(Vec2 v) { return sqrt(v.x * v.x + v.y * v.y); }

static inline float vec2_dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

static inline Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

static inline Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    if (len < 1e-10)
        return (Vec2){0, 0};
    return vec2_scale(v, 1.0 / len);
}

static float frand01(void) { return (float)rand() / (float)RAND_MAX; }

float frand(float a, float b) { return a + (b - a) * frand01(); }
