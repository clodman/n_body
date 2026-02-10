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
constexpr double RESTITUTION = 0.8;

typedef struct {
    double masses[BALLS_NUMBER];
    double radiuses[BALLS_NUMBER];
    Vec2 positions[BALLS_NUMBER];
    Vec2 velocities[BALLS_NUMBER];
    Vec2 accelerations[BALLS_NUMBER];
} Balls;

Vec2 screen_coordinates(Vec2 central_coordinates);
Balls balls_init();
void balls_collide_inelastic(Balls *balls, u32 i, u32 j);
void balls_collide_elastic(Balls *balls, u32 i, u32 j);
void balls_interact(Balls *balls);
void balls_move(Balls *balls);
void balls_draw(Balls *balls);

struct Window window = {.width = 800, .height = 600};
int main(void) {
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
        .masses = {10, 30, 50},
        .radiuses = {10, 30, 50},
        .positions = {(Vec2){0, 0}, (Vec2){0, 100}, (Vec2){-200, -100}},
        .velocities = {(Vec2){1, 0}, (Vec2){-0.5, 0}, (Vec2){1, 1}}};
}

void balls_collide_inelastic(Balls *balls, u32 i, u32 j) {
    Vec2 v1_factor = SCALAR_MULT_COPY(balls->velocities[i], balls->masses[i]);
    Vec2 v2_factor = SCALAR_MULT_COPY(balls->velocities[j], balls->masses[j]);
    Vec2 v3 = VECTOR_ADD_COPY(v1_factor, v2_factor);
    // how tf do i prevent them from drifting together after 1 billion shocs
    SCALAR_MULT(v3, RESTITUTION / (balls->masses[i] + balls->masses[j]))
    balls->velocities[i] = v3;
    balls->velocities[j] = v3;
}

void balls_collide_elastic(Balls *balls, u32 i, u32 j) {
    // parentheses or crash???
    Vec2 v1_factor = SCALAR_MULT_COPY(balls->velocities[i],
                                   (balls->masses[i] - balls->masses[j]));
    Vec2 v2_factor = SCALAR_MULT_COPY(balls->velocities[j], 2 * balls->masses[j]);
    Vec2 v1 = VECTOR_ADD_COPY(v1_factor, v2_factor);
    SCALAR_MULT(v1, RESTITUTION / (balls->masses[i] + balls->masses[j]))

    v2_factor = SCALAR_MULT_COPY(balls->velocities[j],
                                   (balls->masses[j] - balls->masses[i]));
    v1_factor = SCALAR_MULT_COPY(balls->velocities[i], 2 * balls->masses[i]);
    Vec2 v2 = VECTOR_ADD_COPY(v2_factor, v1_factor);
    SCALAR_MULT(v2, RESTITUTION / (balls->masses[i] + balls->masses[j]))
    balls->velocities[i] = v1;
    balls->velocities[j] = v2;
}

void balls_interact(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Vec2 vec_IJ = {.x = balls->positions[j].x - balls->positions[i].x,
                           .y = balls->positions[j].y - balls->positions[i].y};
            double r_size = sqrt(vec_IJ.x * vec_IJ.x + vec_IJ.y * vec_IJ.y);

            if (r_size <= balls->radiuses[i] + balls->radiuses[j]) {
                Vec2 rel_velocity = {.x = balls->velocities[j].x - balls->velocities[i].x,
                           .y = balls->velocities[j].y - balls->velocities[i].y};
                // why does it have to be on a separate line??
                double vsize = VECTOR_NORM(rel_velocity);
                // is this check even physically sensible??
                vsize >= 1  ? balls_collide_elastic(balls, i, j) : balls_collide_inelastic(balls,i, j);
                balls->positions[j] = VECTOR_ADD_COPY(balls->positions[i], SCALAR_MULT_COPY(vec_IJ, (balls->radiuses[i]+balls->radiuses[j]+0.1) / r_size));
            }

            balls->accelerations[i] = (Vec2){
                G * balls->masses[j] / (r_size * r_size * r_size) * vec_IJ.x,
                G * balls->masses[j] / (r_size * r_size * r_size) * vec_IJ.y};

            balls->accelerations[j] = (Vec2){
                -G * balls->masses[i] / (r_size * r_size * r_size) * vec_IJ.x,
                -G * balls->masses[i] / (r_size * r_size * r_size) * vec_IJ.y};
        }
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
