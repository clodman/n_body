#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "base.c"
// fusions
// scale
// trajectories
// autogen balls
// RK 4
// assert
// drag and drop
constexpr u32 TARGET_FPS = 120;

struct Window {
    u32 height;
    u32 width;
} window;

constexpr u32 BALLS_NUMBER = 3;
constexpr double G = 6.674 * 10;
constexpr double MAX_ACUMULATOR = 0.25;
constexpr double PHYS_DT = 1. / 120;
constexpr double SIM_SPEED = 2000.0;

typedef struct {
    Vec2 positions[BALLS_NUMBER];
    Vec2 velocities[BALLS_NUMBER];
    Vec2 accelerations[BALLS_NUMBER];
    double masses[BALLS_NUMBER];
    double radiuses[BALLS_NUMBER];
    double restitutions[BALLS_NUMBER];
} Balls;

typedef struct {
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    double mass;
    double radius;
    double restitution;
} Ball;

Vec2 screen_coordinates(Vec2 central_coordinates);
Balls balls_init();
Ball ball_init_from_index(const Balls balls[static 1], u32 i);
void store_ball_into_index(Ball ball, Balls balls[static 1], u32 i);
void balls_collide_elastic(Ball *b1, Ball *b2);
void balls_interact(Balls *balls);
void balls_move(Balls *balls);
void balls_draw(Balls *balls);

struct Window window = {.width = 800, .height = 600};
int main(void) {
    assert(BALLS_NUMBER <= 0xFFFFFFFF &&
           "indexes are u32, so any more will overflow");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window.width, window.height, "N-body??");
    SetTargetFPS(TARGET_FPS);

    Balls balls = balls_init();
    double acumulator = 0.;

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);

        acumulator += GetFrameTime() * SIM_SPEED;
        acumulator = MIN(acumulator, MAX_ACUMULATOR);
        while (acumulator >= PHYS_DT) {
            balls_interact(&balls);
            balls_move(&balls);
            acumulator -= PHYS_DT;
        }
        balls_draw(&balls);

        DrawText("N-body??", 10, 10, 20, LIGHTGRAY);
        DrawFPS(10, 40);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

inline Vec2 screen_coordinates(Vec2 central_coordinates) {
    if (IsWindowResized()) {
        window.width = GetScreenWidth();
        window.height = GetScreenHeight();
    }

    central_coordinates.x += window.width / 2.;
    central_coordinates.y =
        (window.height - central_coordinates.y) - window.height / 2.;
    return central_coordinates;
}

Balls balls_init() {
    return (Balls){
        .positions = {(Vec2){0, 0}, (Vec2){0, 100}, (Vec2){-200, -100}},
        .velocities = {(Vec2){2, 0}, (Vec2){-0.5, 1}, (Vec2){1, 1}},
        .masses = {10, 30, 50},
        .radiuses = {10, 30, 50},
        .restitutions = {1, 1, 1},
    };
}

inline Ball ball_init_from_index(const Balls balls[static 1], u32 i) {
    assert(balls != NULL && "ball_init_from_index needs ball to be not null");
    return (Ball){
        .position = balls->positions[i],
        .velocity = balls->velocities[i],
        .acceleration = balls->accelerations[i],
        .mass = balls->masses[i],
        .radius = balls->radiuses[i],
        .restitution = balls->restitutions[i],
    };
}

inline void store_ball_into_index(Ball ball, Balls balls[static 1], u32 i) {
    assert(balls != NULL && "store_ball_into_index needs ball to be not null");
    balls->positions[i] = ball.position;
    balls->velocities[i] = ball.velocity;
    balls->accelerations[i] = ball.acceleration;
    balls->masses[i] = ball.mass;
    balls->radiuses[i] = ball.radius;
    balls->restitutions[i] = ball.restitution;
}

// TODO: Implement or remove this stub function
// Fusion should: combine masses, compute new velocity from momentum
// conservation, remove one ball, adjust BALLS_NUMBER (requires dynamic
// allocation)
void balls_fuse(Ball *b1, Ball *b2) {
    // bruuuh
}

void balls_collide_elastic(Ball *b1, Ball *b2) {
    Vec2 normal12 = vec2_normalize(vec2_sub(b2->position, b1->position));

    double v1 = vec2_dot(b1->velocity, normal12);
    double v2 = vec2_dot(b2->velocity, normal12);
    double p1 = b1->mass * v1;
    double p2 = b2->mass * v2;

    double speed_normal_relative = v1 - v2;
    if (speed_normal_relative <= 0)
        return;

    double vprime1 = (1 + MIN(b1->restitution, b2->restitution)) * (p1 + p2) / (b1->mass + b2->mass) -
                     MIN(b1->restitution, b2->restitution) * v1;
    double vprime2 = (1 + MIN(b1->restitution, b2->restitution)) * (p1 + p2) / (b1->mass + b2->mass) -
                     MIN(b1->restitution, b2->restitution) * v2;

    Vec2 v1_tangential = vec2_sub(b1->velocity, vec2_scale(normal12, v1));
    Vec2 v2_tangential = vec2_sub(b2->velocity, vec2_scale(normal12, v2));
    b1->velocity = vec2_add(v1_tangential, vec2_scale(normal12, vprime1));
    b2->velocity = vec2_add(v2_tangential, vec2_scale(normal12, vprime2));
}

void balls_interact(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        balls->accelerations[i] = (Vec2){0, 0};
    }

    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        Ball b1 = ball_init_from_index(balls, i);
        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Ball b2 = ball_init_from_index(balls, j);
            Vec2 distance_vec = vec2_sub(b2.position, b1.position);
            double distance_scalar = vec2_length(distance_vec);

            if (distance_scalar <= b1.radius + b2.radius) {
                balls_collide_elastic(&b1, &b2);
                // change for biggest mass
                b2.position =
                    vec2_add(b1.position,
                             vec2_scale(distance_vec, (b1.radius + b2.radius) /
                                                          distance_scalar));
            }

            b1.acceleration.x +=
                G * b2.mass /
                (distance_scalar * distance_scalar * distance_scalar) *
                distance_vec.x,
                b1.acceleration.y +=
                G * b2.mass /
                (distance_scalar * distance_scalar * distance_scalar) *
                distance_vec.y;

            b2.acceleration.x +=
                -G * b1.mass /
                (distance_scalar * distance_scalar * distance_scalar) *
                distance_vec.x,
                b2.acceleration.y +=
                -G * b1.mass /
                (distance_scalar * distance_scalar * distance_scalar) *
                distance_vec.y;
            store_ball_into_index(b2, balls, j);
        }
        store_ball_into_index(b1, balls, i);
    }
}

void balls_move(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        balls->velocities[i].x += balls->accelerations[i].x * PHYS_DT;
        balls->velocities[i].y += balls->accelerations[i].y * PHYS_DT;

        balls->positions[i].x += balls->velocities[i].x * PHYS_DT;
        balls->positions[i].y += balls->velocities[i].y * PHYS_DT;
    }
}

void balls_draw(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        Vec2 screen_p = screen_coordinates(balls->positions[i]);
        double r = balls->radiuses[i];
        DrawCircle(screen_p.x, screen_p.y, r, LIGHTGRAY);
    }
}
