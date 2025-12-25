#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>

// Window size
#define WIDTH 600
#define HEIGHT 600

// Colors
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000

// Physics constants
#define GRAVITY 0.5
#define ELASTICITY 0.9   // 1.0 = perfectly elastic

// ----------------------------
// Circle structure
// ----------------------------
// x, y       -> current position
// oldx, oldy -> previous position (used to infer velocity)
// radius     -> circle size
struct Circle
{
    double x, y;
    double oldx, oldy;
    double radius;
};

// ----------------------------
// Filled circle rendering
// ----------------------------
void FillCircle(SDL_Surface* surface, Circle circle, Uint32 color)
{
    double r2 = circle.radius * circle.radius;

    for (int x = circle.x - circle.radius; x <= circle.x + circle.radius; x++) {
        for (int y = circle.y - circle.radius; y <= circle.y + circle.radius; y++) {
            double dx = x - circle.x;
            double dy = y - circle.y;
            if (dx*dx + dy*dy <= r2) {
                SDL_Rect pixel = { x, y, 1, 1 };
                SDL_FillRect(surface, &pixel, color);
            }
        }
    }
}

// ----------------------------
// Draw outline of container circle
// ----------------------------
void DrawCircleOutline(SDL_Surface* surface, Circle circle, Uint32 color)
{
    double rOuter = circle.radius * circle.radius;
    double rInner = (circle.radius - 1) * (circle.radius - 1);

    for (int x = circle.x - circle.radius; x <= circle.x + circle.radius; x++) {
        for (int y = circle.y - circle.radius; y <= circle.y + circle.radius; y++) {
            double dx = x - circle.x;
            double dy = y - circle.y;
            double d = dx*dx + dy*dy;
            if (d <= rOuter && d >= rInner) {
                SDL_Rect pixel = { x, y, 1, 1 };
                SDL_FillRect(surface, &pixel, color);
            }
        }
    }
}

// ----------------------------
// Verlet integration step
// ----------------------------
void UpdateCircle(Circle& c)
{
    double vx = c.x - c.oldx;
    double vy = c.y - c.oldy;

    // Store current position
    c.oldx = c.x;
    c.oldy = c.y;

    // Integrate position (gravity acts downward)
    c.x += vx;
    c.y += vy + GRAVITY;
}

// ----------------------------
// Circular container constraint
// ----------------------------
void ApplyCircularConstraint(Circle& particle, const Circle& container)
{
    // Capture velocity FIRST (before modifying position)
    double vx = particle.x - particle.oldx;
    double vy = particle.y - particle.oldy;

    // Vector from container center to particle
    double dx = particle.x - container.x;
    double dy = particle.y - container.y;

    double dist = sqrt(dx*dx + dy*dy);
    double maxDist = container.radius - particle.radius;

    // If particle escapes container
    if (dist > maxDist) {
        // Normal vector
        double nx = dx / dist;
        double ny = dy / dist;

        // Snap particle back onto boundary
        particle.x = container.x + nx * maxDist;
        particle.y = container.y + ny * maxDist;

        // Reflect velocity across normal
        double dot = vx * nx + vy * ny;
        vx -= 2.0 * dot * nx;
        vy -= 2.0 * dot * ny;

        // Apply elasticity
        vx *= ELASTICITY;
        vy *= ELASTICITY;

        // Reconstruct previous position
        particle.oldx = particle.x - vx;
        particle.oldy = particle.y - vy;
    }
}


// Applying Collisions
void ResolveBallCollision(Circle& a, Circle& b)
{
    // Vector between centers
    double dx = b.x - a.x;
    double dy = b.y - a.y;

    double dist = sqrt(dx*dx + dy*dy);
    double minDist = a.radius + b.radius;

    // No collision
    if (dist >= minDist || dist == 0.0)
        return;

    // Normalized collision normal
    double nx = dx / dist;
    double ny = dy / dist;

    // -------- POSITION CORRECTION --------
    double overlap = minDist - dist;
    double correction = overlap * 0.5;

    a.x -= nx * correction;
    a.y -= ny * correction;
    b.x += nx * correction;
    b.y += ny * correction;

    // -------- VELOCITY (VERLET STYLE) --------
    double avx = a.x - a.oldx;
    double avy = a.y - a.oldy;
    double bvx = b.x - b.oldx;
    double bvy = b.y - b.oldy;

    // Relative velocity
    double rvx = bvx - avx;
    double rvy = bvy - avy;

    // Velocity along normal
    double velAlongNormal = rvx * nx + rvy * ny;

    // If balls are separating, don't resolve
    if (velAlongNormal > 0)
        return;

    // Elastic response
    double impulse = -(1.0 + ELASTICITY) * velAlongNormal;
    impulse *= 0.5; // equal mass

    double ix = impulse * nx;
    double iy = impulse * ny;

    avx -= ix;
    avy -= iy;
    bvx += ix;
    bvy += iy;

    // Reconstruct old positions
    a.oldx = a.x - avx;
    a.oldy = a.y - avy;
    b.oldx = b.x - bvx;
    b.oldy = b.y - bvy;
}


// ----------------------------
// Main
// ----------------------------
int main(int argc, char* argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        "Verlet Circle Constraint",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN
    );

    SDL_Surface* surface = SDL_GetWindowSurface(window);

    // Container circle
    Circle container;
    container.x = WIDTH / 2;
    container.y = HEIGHT / 2;
    container.radius = 250;

    // Particle
    Circle ball;
    ball.x = 200;
    ball.y = 100;
    ball.oldx = 300;
    ball.oldy = 100;
    ball.radius = 40;

//     Circle ball2;
// ball2.x = 350;
// ball2.y = 100;
// ball2.oldx = 350;
// ball2.oldy = 100;
// ball2.radius = 40;


    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
        }

        SDL_FillRect(surface, NULL, COLOR_BLACK);

        UpdateCircle(ball);
        // UpdateCircle(ball2);

        // Solve constraints & collisions multiple times
        for (int i = 0; i < 4; i++) {
            ApplyCircularConstraint(ball, container);
            // ApplyCircularConstraint(ball2, container);
            // ResolveBallCollision(ball, ball2);
        }

        // Render
        FillCircle(surface, ball, COLOR_WHITE);
        // FillCircle(surface, ball2, COLOR_WHITE);
        // DrawCircleOutline(surface, container, COLOR_WHITE);

        SDL_UpdateWindowSurface(window);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_Quit();
    return 0;
}
