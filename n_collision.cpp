#include <stdio.h>
#include <vector>
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
#define BALL_COUNT 200
#define SUBSTEP_COUNT 8
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
    Uint32 color;
};

Uint32 getRainbow(SDL_Surface* surface, float t)
{
    float r = sinf(t);
    float g = sinf(t + 0.33f * 2.0f * M_PI);
    float b = sinf(t + 0.66f * 2.0f * M_PI);

    Uint8 R = (Uint8)(255.0f * r * r);
    Uint8 G = (Uint8)(255.0f * g * g);
    Uint8 B = (Uint8)(255.0f * b * b);

    return SDL_MapRGB(surface->format, R, G, B);
}


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
//     Circle ball;
//     ball.x = 200;
//     ball.y = 100;
//     ball.oldx = 300;
//     ball.oldy = 100;
//     ball.radius = 40;


    std::vector<Circle> balls;
    // Vector - used to store multiple balls if needed
    // <Circle> - template for Circle structure
    // balls - name of the vector
float colorTime = 0.0f;
float colorStep = 0.15f; // smaller = smoother rainbow

for (int i = 0; i < BALL_COUNT; i++) {
    Circle b;

    b.x = 300 + i * 2;
    b.y = 100 + i * 2;
    b.oldx = b.x;
    b.oldy = b.y;
    b.radius = 6 + (rand() % 15); // Random radius between 6 and 20
    // b.radius = 5;

    b.color = getRainbow(surface, colorTime);
    colorTime += colorStep;

    balls.push_back(b);
}

    int running = 1;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = 0;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
               printf("Mouse Clicked at (%d, %d)\n", event.button.x, event.button.y);
            //    If ball is clicked, remove the ball
            int BALL_CLICKED = 0;
               for (int i = 0; i < balls.size(); i++) {
                     double dx = balls[i].x - event.button.x;
                     double dy = balls[i].y - event.button.y;
                     double dist2 = dx*dx + dy*dy;
                     if (dist2 < balls[i].radius * balls[i].radius) {
                         printf("Ball %d clicked!\n", i);
                         printf("Ball Color: 0x%X\n", balls[i].color);
                         balls[i].color = COLOR_WHITE; // Change color on click
                        //  Remove the ball on click
                         balls.erase(balls.begin() + i);
                            BALL_CLICKED = 1;
                            break;
                     }
               }
                if (BALL_CLICKED == 0) {
// Open, so add a new ball at mouse position
                Circle newBall;
                newBall.x = event.button.x;
                newBall.y = event.button.y;
                newBall.oldx = newBall.x;
                newBall.oldy = newBall.y;
                newBall.radius = 10 + (rand() % 10);
                // newBall.radius = 5;
                // newBall.color = getRainbow(surface, colorTime);
                newBall.color = 0xffffffff;
                colorTime += colorStep;
                balls.push_back(newBall);
                
                }
                

            }

            if (event.type == SDL_MOUSEMOTION && (event.motion.state != 0)) {
                // If dragging on ball, delete ball
                 printf("Mouse Dragged at (%d, %d)\n", event.motion.x, event.motion.y);
                 int BALL_DRAGGED = 0;
                    for (int i = 0; i < balls.size(); i++) {
                        double dx = balls[i].x - event.motion.x;
                        double dy = balls[i].y - event.motion.y;
                        double dist2 = dx*dx + dy*dy;
                        if (dist2 < balls[i].radius * balls[i].radius) {
                            printf("Ball %d dragged and removed!\n", i);
                            balls.erase(balls.begin() + i);
                            BALL_DRAGGED = 1;
                            break;
                        }
                    }
                if (BALL_DRAGGED == 0) {
               // Drag to add multiple balls
                Circle newBall;
                newBall.x = event.motion.x;
                newBall.y = event.motion.y;
                newBall.oldx = newBall.x;
                newBall.oldy = newBall.y;
                newBall.radius = 6 + (rand() % 10);
                // newBall.radius = 5;
                // newBall.color = getRainbow(surface, colorTime);
                newBall.color = 0xffffffff;
                colorTime += colorStep;
                balls.push_back(newBall);}
            }
        }

        SDL_FillRect(surface, NULL, COLOR_BLACK);

            for (int i = 0; i < balls.size(); i++) {
        UpdateCircle(balls[i]);
    } // Update all balls
        

        // Solve constraints & collisions multiple times
        // SUB-STEPPING FOR STABILITY
        for (int i = 0; i < SUBSTEP_COUNT; i++) {
            for (int i = 0; i < balls.size(); i++) {
                for (int j = i + 1; j < balls.size(); j++) {
                    ResolveBallCollision(balls[i], balls[j]);
                }
            ApplyCircularConstraint(balls[i], container);
            }
        }



        // Render
        for (int i = 0; i < balls.size(); i++) {
    FillCircle(surface, balls[i], balls[i].color);
}
        DrawCircleOutline(surface, container, 0XCCCCCC);

        SDL_UpdateWindowSurface(window);
        SDL_Delay(16); // ~60 FPS
    }

    SDL_Quit();
    return 0;
}

// g++ n_collision.cpp -o n_collision -I C:/MinGW/include -L C:/MinGW/lib -lmingw32 -lSDL2main -lSDL2 -lm