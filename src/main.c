#include "raylib.h"
#include <assert.h>
#include <math.h>

#include "base.c"
// Coordinates normalized
// Shoks
// fusions
// scale
// trajectories
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
    double masses[BALLS_NUMBER];
    double radiuses[BALLS_NUMBER];
    Vec2 positions[BALLS_NUMBER];
    Vec2 velocities[BALLS_NUMBER];
    Vec2 accelerations[BALLS_NUMBER];
} Balls;

Vec2 screen_coordinates(Vec2 central_coordinates);
Balls balls_init();
void balls_interact(Balls *balls);
Vec2 ball_accelerate(Vec2 vec_IJ, double mass_of_j);
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
        .velocities = {(Vec2){1, 0}, (Vec2){-0.5, 0}, (Vec2){0, 1}}};
}

void balls_interact(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Vec2 vec_IJ = {.x = balls->positions[j].x - balls->positions[i].x,
                           .y = balls->positions[j].y - balls->positions[i].y};
            balls->accelerations[i] = ball_accelerate(vec_IJ, balls->masses[j]);
            balls->accelerations[j] = SCALAR_MULT(balls->accelerations[i], -balls->masses[i]/balls->masses[j]);
        }
    }
}

Vec2 ball_accelerate(Vec2 vec_IJ, double mass_of_j) {
    double r_size = sqrt(vec_IJ.x * vec_IJ.x + vec_IJ.y * vec_IJ.y);
    Vec2 acceleration_i =
        (Vec2){G * mass_of_j / (r_size * r_size * r_size) * vec_IJ.x,
               G * mass_of_j / (r_size * r_size * r_size) * vec_IJ.y};
    return acceleration_i;
}

void balls_collide(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Vec2 r = {.x = balls->positions[j].x - balls->positions[i].x,
                      .y = balls->positions[j].y - balls->positions[i].y};
            double r_size = sqrt(r.x * r.x + r.y * r.y);
            if (r_size <= balls->radiuses[i] + balls->radiuses[j]) {
                // reflect??
                // reuse??
            }
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
