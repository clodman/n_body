#include "base.c"
#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <time.h>

enum {
    TARGET_FPS = 120,
    BALLS_NUMBER = 20,
};

static const float G = 6.674 * 1000.0;
static const float MAX_ACUMULATOR = 10;
static const float PHYS_DT = 0.1 / TARGET_FPS;
float GRAVITATIONAL_SOFTENING = 0.1;
static const float SIM_SPEED = 300.0;

static const float COLLISION_SLOP = 0.01;
static const float COLLISION_PERCENT = 0.8;

typedef struct {
    u32 height;
    u32 width;
    bool paused;
    float zoom;
    Vec2 shifting;
    RenderTexture2D trail_texture;
} Window;

typedef struct {
    Vec2 positions[BALLS_NUMBER];
    Vec2 velocities[BALLS_NUMBER];
    Vec2 accelerations[BALLS_NUMBER];
    float masses[BALLS_NUMBER];
    float radiuses[BALLS_NUMBER];
    float restitutions[BALLS_NUMBER];
} Balls;

typedef struct {
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    float mass;
    float radius;
    float restitution;
} Ball;

static Window window = {
    .width = 800,
    .height = 600,
    .paused = false,
    .zoom = 1.0,
    .shifting = (Vec2){0.0, 0.0},
    .trail_texture = {0},
};

static void balls_handle_input();
static Balls balls_init(void);
static void balls_accelerate(Balls *balls);
static void balls_colide(Balls *balls);
static void balls_resolve_colition(Balls *balls, u32 i, u32 j, Vec2 n12);
static void balls_separate_overlap(Balls *balls, u32 i, u32 j, Vec2 n12,
                                   float dist);
static void balls_move(Balls *balls);
static void balls_draw(const Balls *balls, Color color);
static Vec2 to_screen_position(Vec2 world_position);

int main(void) {
    assert(BALLS_NUMBER > 0);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow((int)window.width, (int)window.height, "N-body??");
    SetTargetFPS(TARGET_FPS);

    window.trail_texture =
        LoadRenderTexture((int)window.width, (int)window.height);

    Balls balls = balls_init();
    balls_accelerate(&balls);
    // setup leapfrog
    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        balls.velocities[i].x += 0.5 * balls.accelerations[i].x * PHYS_DT;
        balls.velocities[i].y += 0.5 * balls.accelerations[i].y * PHYS_DT;

        balls.positions[i].x += balls.velocities[i].x * PHYS_DT;
        balls.positions[i].y += balls.velocities[i].y * PHYS_DT;
    }

    float acumulator = 0.0;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            window.width = (u32)GetScreenWidth();
            window.height = (u32)GetScreenHeight();
            UnloadRenderTexture(window.trail_texture);
            window.trail_texture =
                LoadRenderTexture((int)window.width, (int)window.height);
        }

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

            // Draw to trail texture with fade effect
            BeginTextureMode(window.trail_texture);
            DrawRectangle(
                0, 0, (int)window.width, (int)window.height,
                (Color){0, 0, 0, 4}); 
            balls_draw(&balls, LIGHTGRAY);
            EndTextureMode();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // Draw the trail texture (flipped vertically because render textures
        // are upside down)
        DrawTextureRec(
            window.trail_texture.texture,
            (Rectangle){0, 0, (float)window.width, -(float)window.height},
            (Vector2){0, 0}, WHITE);

        if (window.paused) {
            DrawText("PAUSED", (int)(window.width / 2.0 - 11.0 * 3.0),
                     (int)(window.height / 2.0 - 20.0), 20, LIGHTGRAY);
        }

        DrawText("N-body??", 10, 10, 20, LIGHTGRAY);
        DrawFPS(10, 40);

        EndDrawing();
    }

    UnloadRenderTexture(window.trail_texture);
    CloseWindow();
    return 0;
}

static void balls_handle_input() {
    if (IsKeyPressed(KEY_SPACE)) {
        window.paused = !window.paused;
    }
    if (IsKeyPressed(KEY_W)) {
        window.zoom *= 1.5;
        ClearBackground(BLACK);
    }
    if (IsKeyPressed(KEY_S)) {
        window.zoom /= 1.5;
        ClearBackground(BLACK);
    }
    if (IsKeyDown(KEY_UP)) {
        window.shifting.y += 70.0 * sqrt(window.zoom);
    }
    if (IsKeyDown(KEY_DOWN)) {
        window.shifting.y -= 70.0 * sqrt(window.zoom);
    }
    if (IsKeyDown(KEY_RIGHT)) {
        window.shifting.x -= 70.0 * sqrt(window.zoom);
    }
    if (IsKeyDown(KEY_LEFT)) {
        window.shifting.x += 70.0 * sqrt(window.zoom);
    }
}

static Balls balls_init(void) {
    Balls result = {0};

    result.positions[0] = (Vec2){0, 0};
    result.velocities[0] = (Vec2){0, 0};
    result.masses[0] = 200000.0;
    result.radiuses[0] = 300.0;
    result.restitutions[0] = 0.7;

    for (u32 i = 1; i < BALLS_NUMBER; ++i) {
        float r = frand(5000.0, 7000.0);
        float a = frand(0.0, 2.0 * M_PI);

        Vec2 pos = (Vec2){r * cos(a), r * sin(a)};
        result.positions[i] = pos;

        Vec2 er = vec2_normalize(pos);
        Vec2 et = (Vec2){-er.y, er.x};

        result.masses[i] = frand(10.0, 80.0);
        result.radiuses[i] = result.masses[i] * 0.3;
        result.restitutions[i] = frand(0.7, 1.0);

        float M = result.masses[0];
        float v_circ = sqrt(G * M / r);

        float tangential_scale = frand(0.85, 0.98);
        float radial_inward = -0.0 * v_circ;

        result.velocities[i] =
            vec2_add(vec2_scale(et, v_circ * tangential_scale),
                     vec2_scale(er, radial_inward));
    }

    return result;
}

static void balls_accelerate(Balls *balls) {
    const float eps2 = GRAVITATIONAL_SOFTENING * (float)GRAVITATIONAL_SOFTENING;

    for (u32 i = 0; i < BALLS_NUMBER; ++i) {
        const Vec2 pi = balls->positions[i];

        float ax = 0.;
        float ay = 0.;

        for (u32 j = 0; j < BALLS_NUMBER; ++j) {
            if (j == i)
                continue;

            const Vec2 pj = balls->positions[j];
            const float dx = pj.x - (float)pi.x;
            const float dy = pj.y - (float)pi.y;

            const float r2 = dx * dx + dy * dy + eps2;

            const float inv_r = 1. / sqrt(r2);
            const float inv_r3 = inv_r * inv_r * inv_r;

            const float s = G * balls->masses[j] * inv_r3;

            ax += s * dx;
            ay += s * dy;
        }

        balls->accelerations[i] = (Vec2){ax, (float)ay};
    }
}

static void balls_colide(Balls *balls) {
    assert(balls != NULL);

    for (u32 i = 0; i < BALLS_NUMBER - 1; ++i) {
        for (u32 j = i + 1; j < BALLS_NUMBER; ++j) {
            Vec2 distance_vec =
                vec2_sub(balls->positions[j], balls->positions[i]);
            float distance_scalar2 = vec2_dot(distance_vec, distance_vec);

            float radius_sum = balls->radiuses[i] + balls->radiuses[j];
            if (distance_scalar2 <= radius_sum * radius_sum) {
                Vec2 delta = vec2_sub(balls->positions[j], balls->positions[i]);
                float dist = vec2_length(delta);
                Vec2 n12 = vec2_scale(delta, 1 / dist);
                balls_resolve_colition(balls, i, j, n12);
                balls_separate_overlap(balls, i, j, n12, dist);
            }
        }
    }
}

static void balls_resolve_colition(Balls *balls, u32 i, u32 j, Vec2 n12) {

    const float v1 = vec2_dot(balls->velocities[i], n12);
    const float v2 = vec2_dot(balls->velocities[j], n12);

    const float p1 = balls->masses[i] * v1;
    const float p2 = balls->masses[j] * v2;

    const float speed_normal_relative = v1 - v2;
    if (speed_normal_relative <= 0.0) {
        return;
    }

    const float e = MIN(balls->restitutions[i], balls->restitutions[j]);

    const float mass_sum = balls->masses[i] + balls->masses[j];
    const float vprime1 = (1.0 + e) * (p1 + p2) / mass_sum - e * v1;
    const float vprime2 = (1.0 + e) * (p1 + p2) / mass_sum - e * v2;

    Vec2 v1_tangential = vec2_sub(balls->velocities[i], vec2_scale(n12, v1));
    Vec2 v2_tangential = vec2_sub(balls->velocities[j], vec2_scale(n12, v2));

    balls->velocities[i] = vec2_add(v1_tangential, vec2_scale(n12, vprime1));
    balls->velocities[j] = vec2_add(v2_tangential, vec2_scale(n12, vprime2));
}

static void balls_separate_overlap(Balls *balls, u32 i, u32 j, Vec2 n12,
                                   float dist) {
    assert(dist >= 0);

    const float min_dist = balls->radiuses[i] + balls->radiuses[j];
    const float penetration = min_dist - dist;
    assert(penetration >= 0);

    const float inv_m1 = 1.0 / balls->masses[i];
    const float inv_m2 = 1.0 / balls->masses[j];
    const float inv_sum = inv_m1 + inv_m2;

    float corr_mag =
        COLLISION_PERCENT * MAX(penetration - COLLISION_SLOP, 0.0) / inv_sum;
    Vec2 correction = vec2_scale(n12, corr_mag);

    balls->positions[i] =
        vec2_sub(balls->positions[i], vec2_scale(correction, inv_m1));
    balls->positions[j] =
        vec2_add(balls->positions[j], vec2_scale(correction, inv_m2));
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
        float r = window.zoom * balls->radiuses[i];
        DrawCircle((int)screen_p.x, (int)screen_p.y, r, color);
        if (r < 1) {
            DrawPixel(screen_p.x, screen_p.y, color);
        }
    }
}

static Vec2 to_screen_position(Vec2 world_position) {
    world_position.x =
        world_position.x * window.zoom + window.width / 2.0 + window.shifting.x;
    world_position.y = window.height / 2.0 - world_position.y * window.zoom +
                       window.shifting.y;

    return world_position;
}
