#include "base.c"
#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <time.h>

// TODO: fusion

typedef unsigned int u32;

enum {
    TARGET_FPS = 120,
    BALLS_NUMBER = 400,
};

static const double G = 6.674 * 10.0;
static const double MAX_ACUMULATOR = 0.25;
static const double PHYS_DT = 0.1 / (double)TARGET_FPS;
static const double SIM_SPEED = 2.0;

static const double COLLISION_SLOP = 0.01;
static const double COLLISION_PERCENT = 0.8;

typedef struct {
    u32 height;
    u32 width;
    bool paused;
    double zoom;
    Vec2 shifting;
} Window;

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

static Window window = {
    .width = 800,
    .height = 600,
    .paused = false,
    .zoom = 1.0,
    .shifting = (Vec2){0.0, 0.0},
};

static void balls_handle_input();
static Balls balls_init(void);
static Ball balls_ball_set(const Balls *balls, u32 i);
static void balls_ball_get(Ball ball, Balls *balls, u32 i);
static void balls_accelerate(Balls *balls);
static void balls_colide(Balls *balls);
static void balls_resolve_colition(Ball *b1, Ball *b2);
static void balls_separate_overlap(Ball *b1, Ball *b2);
static void balls_move(Balls *balls);
static void balls_draw(const Balls *balls, Color color);
static Vec2 to_screen_position(Vec2 world_position);

int main(void) {
    assert(BALLS_NUMBER > 0);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow((int)window.width, (int)window.height, "N-body??");
    SetTargetFPS(TARGET_FPS);

    Balls balls = balls_init();
    double acumulator = 0.0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        balls_handle_input();

        if (!window.paused) {
            acumulator += GetFrameTime() * SIM_SPEED;
            acumulator = MIN(acumulator, MAX_ACUMULATOR);

            while (acumulator >= PHYS_DT) {
                balls_accelerate(&balls);
                balls_colide(&balls);
                balls_move(&balls);
                acumulator -= PHYS_DT;
            }

            balls_draw(&balls, LIGHTGRAY);
        } else {
            balls_draw(&balls, (Color){65, 67, 67, 255});
            DrawText("PAUSED", (int)(window.width / 2.0 - 11.0 * 3.0),
                     (int)(window.height / 2.0 - 20.0), 20, LIGHTGRAY);
        }

        DrawText("N-body??", 10, 10, 20, LIGHTGRAY);
        DrawFPS(10, 40);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

static void balls_handle_input() {
    if (IsKeyPressed(KEY_SPACE)) {
        window.paused = !window.paused;
    }
    if (IsKeyPressed(KEY_W)) {
        window.zoom *= 2.0;
    }
    if (IsKeyPressed(KEY_S)) {
        window.zoom /= 2.0;
    }
    if (IsKeyDown(KEY_UP)) {
        window.shifting.y += 7.0 / window.zoom;
    }
    if (IsKeyDown(KEY_DOWN)) {
        window.shifting.y -= 7.0 / window.zoom;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        window.shifting.x -= 7.0 / window.zoom;
    }
    if (IsKeyDown(KEY_LEFT)) {
        window.shifting.x += 7.0 / window.zoom;
    }
}

static Balls balls_init(void) {
    Balls result = {0};

    result.positions[0] = (Vec2){0, 0};
    result.velocities[0] = (Vec2){0, 0};
    result.masses[0] = 20000.0;
    result.radiuses[0] = 30.0;
    result.restitutions[0] = 1.0;

    for (u32 i = 1; i < BALLS_NUMBER; ++i) {
        double r = frand(150.0, 700.0);
        double a = frand(0.0, 2.0 * M_PI);

        Vec2 pos = (Vec2){r * cos(a), r * sin(a)};
        result.positions[i] = pos;

        Vec2 er = vec2_normalize(pos);
        Vec2 et = (Vec2){-er.y, er.x};

        result.masses[i] = frand(10.0, 80.0);
        result.radiuses[i] = result.masses[i] * 0.3;
        result.restitutions[i] = frand(0.7, 1.0);

        double M = result.masses[0];
        double v_circ = sqrt(G * M / r) * 0.7;

        double tangential_scale = frand(0.85, 0.98);
        double radial_inward = -1.0 * v_circ;

        result.velocities[i] =
            vec2_add(vec2_scale(et, v_circ * tangential_scale),
                     vec2_scale(er, radial_inward));
    }

    return result;
}

static Ball balls_ball_set(const Balls *balls, u32 i) {
    assert(balls != NULL);
    assert(i < BALLS_NUMBER);

    return (Ball){
        .position = balls->positions[i],
        .velocity = balls->velocities[i],
        .acceleration = balls->accelerations[i],
        .mass = balls->masses[i],
        .radius = balls->radiuses[i],
        .restitution = balls->restitutions[i],
    };
}

static void balls_ball_get(Ball ball, Balls *balls, u32 i) {
    assert(balls != NULL);
    assert(i < BALLS_NUMBER);

    balls->positions[i] = ball.position;
    balls->velocities[i] = ball.velocity;
    balls->accelerations[i] = ball.acceleration;
    balls->masses[i] = ball.mass;
    balls->radiuses[i] = ball.radius;
    balls->restitutions[i] = ball.restitution;
}

static void balls_accelerate(Balls *balls) {
    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        balls->accelerations[i] = (Vec2){0, 0};
    }
    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        Ball b1 = balls_ball_set(balls, i);

        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Ball b2 = balls_ball_set(balls, j);

            Vec2 distance_vec = vec2_sub(b2.position, b1.position);
            double distance_scalar = vec2_length(distance_vec);

            double inv_r3 =
                1.0 / (distance_scalar * distance_scalar * distance_scalar);

            b1.acceleration.x += G * b2.mass * inv_r3 * distance_vec.x;
            b1.acceleration.y += G * b2.mass * inv_r3 * distance_vec.y;

            b2.acceleration.x += -G * b1.mass * inv_r3 * distance_vec.x;
            b2.acceleration.y += -G * b1.mass * inv_r3 * distance_vec.y;
        balls_ball_get(b2, balls, j);
        }
    balls_ball_get(b1, balls, i);
    }
}

static void balls_colide(Balls *balls) {
    assert(balls != NULL);

    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        Ball b1 = balls_ball_set(balls, i);

        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Ball b2 = balls_ball_set(balls, j);

            if (b1.mass * b2.mass == 0.0) {
                continue;
            }

            Vec2 distance_vec = vec2_sub(b2.position, b1.position);
            double distance_scalar = vec2_length(distance_vec);

            if (distance_scalar <= b1.radius + b2.radius) {
                balls_resolve_colition(&b1, &b2);
                balls_separate_overlap(&b1, &b2);
            }

            balls_ball_get(b2, balls, j);
        }

        balls_ball_get(b1, balls, i);
    }
}

static void balls_resolve_colition(Ball *b1, Ball *b2) {
    Vec2 normal12 = vec2_normalize(vec2_sub(b2->position, b1->position));

    double v1 = vec2_dot(b1->velocity, normal12);
    double v2 = vec2_dot(b2->velocity, normal12);

    double p1 = b1->mass * v1;
    double p2 = b2->mass * v2;

    double speed_normal_relative = v1 - v2;
    if (speed_normal_relative <= 0.0) {
        return;
    }

    double e = MIN(b1->restitution, b2->restitution);

    double vprime1 = (1.0 + e) * (p1 + p2) / (b1->mass + b2->mass) - e * v1;
    double vprime2 = (1.0 + e) * (p1 + p2) / (b1->mass + b2->mass) - e * v2;

    Vec2 v1_tangential = vec2_sub(b1->velocity, vec2_scale(normal12, v1));
    Vec2 v2_tangential = vec2_sub(b2->velocity, vec2_scale(normal12, v2));

    b1->velocity = vec2_add(v1_tangential, vec2_scale(normal12, vprime1));
    b2->velocity = vec2_add(v2_tangential, vec2_scale(normal12, vprime2));
}

static void balls_separate_overlap(Ball *b1, Ball *b2) {
    Vec2 delta = vec2_sub(b2->position, b1->position);
    double dist = vec2_length(delta);

    if (!(dist > 0.0)) {
        return;
    }

    double min_dist = b1->radius + b2->radius;
    double penetration = min_dist - dist;

    if (penetration <= 0.0) {
        return;
    }

    Vec2 n = vec2_scale(delta, 1.0 / dist);

    double inv_m1 = 1.0 / b1->mass;
    double inv_m2 = 1.0 / b2->mass;
    double inv_sum = inv_m1 + inv_m2;

    double corr_mag =
        COLLISION_PERCENT * MAX(penetration - COLLISION_SLOP, 0.0) / inv_sum;
    Vec2 correction = vec2_scale(n, corr_mag);

    b1->position = vec2_sub(b1->position, vec2_scale(correction, inv_m1));
    b2->position = vec2_add(b2->position, vec2_scale(correction, inv_m2));
}

static void balls_move(Balls *balls) {
    assert(balls != NULL);

    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        balls->velocities[i].x += balls->accelerations[i].x * PHYS_DT;
        balls->velocities[i].y += balls->accelerations[i].y * PHYS_DT;

        balls->positions[i].x += balls->velocities[i].x * PHYS_DT;
        balls->positions[i].y += balls->velocities[i].y * PHYS_DT;
    }
}

static void balls_draw(const Balls *balls, Color color) {
    assert(balls != NULL);

    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        Vec2 screen_p = to_screen_position(balls->positions[i]);
        double r = window.zoom * balls->radiuses[i];

        DrawCircle((int)screen_p.x, (int)screen_p.y, (float)r, color);
    }
}

static Vec2 to_screen_position(Vec2 world_position) {
    if (IsWindowResized()) {
        window.width = (u32)GetScreenWidth();
        window.height = (u32)GetScreenHeight();
    }

    world_position.x =
        world_position.x * window.zoom + window.width / 2.0 + window.shifting.x;
    world_position.y = window.height / 2.0 - world_position.y * window.zoom +
                       window.shifting.y;

    return world_position;
}
