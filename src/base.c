#include <stdint.h>
#include <math.h>

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

typedef double Matrix4[16];
typedef double Matrix3[9];
typedef double Matrix2[4];

typedef struct {
    double x;
    double y;
    double z;
} Vec4;

typedef struct {
    double x;
    double y;
    double z;
} Vec3;

typedef struct {
    double x;
    double y;
} Vec2;

static inline Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

static inline void vec2_add_inplace(Vec2 *v, Vec2 other) {
    v->x += other.x;
    v->y += other.y;
}

static inline Vec2 vec2_scale(Vec2 v, double scalar) {
    return (Vec2){v.x * scalar, v.y * scalar};
}

static inline void vec2_scale_inplace(Vec2 *v, double scalar) {
    v->x *= scalar;
    v->y *= scalar;
}

static inline double vec2_length(Vec2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

// Dot product of two vectors
static inline double vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

// Subtract two vectors
static inline Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

// Normalize a vector (return unit vector in same direction)
static inline Vec2 vec2_normalize(Vec2 v) {
    double len = vec2_length(v);
    if (len < 1e-10) return (Vec2){0, 0};  // Avoid division by zero
    return vec2_scale(v, 1.0 / len);
}
