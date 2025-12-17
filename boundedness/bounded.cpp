#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>



// Setting window height and width
#define WIDTH 600
#define HEIGHT 600
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define GRAVITY 0.5
#define ELASTICITY 0.9


// Creating a structure for Circle that takes x and y coordinates and radius
struct Circle
{
    double x,y;

    double oldx, oldy;
    double radius;
};


void FillCircle(SDL_Surface* surface, Circle circle, Uint32 color) {
    double radiusSquared = circle.radius * circle.radius;
    for (double x=circle.x - circle.radius; x <= circle.x + circle.radius; x++) {
        for (double y=circle.y - circle.radius; y <= circle.y + circle.radius; y++) {
            double distance_squared = (x - circle.x) * (x - circle.x) + (y - circle.y) * (y - circle.y);
            if (distance_squared <= radiusSquared) {
                SDL_Rect pixel = (SDL_Rect){(int)x, (int)y, 1, 1};
                SDL_FillRect(surface, &pixel, color);
            }
        }
    }
}

void UpdateCircle(Circle& circle)
{
    double dt = 1.0;

    // Calculate velocity
    // velocity = current_position - old_position
    double vx = circle.x - circle.oldx;
    double vy = circle.y - circle.oldy;

    // Store current position
    circle.oldx = circle.x;
    circle.oldy = circle.y;

    // Apply Verlet integration
    // New position = current position + velocity + acceleration * dt^2
    // Here, acceleration is due to gravity
    circle.x += vx;
    circle.y += vy + GRAVITY * dt * dt;
}

void ApplyConstraints(Circle& circle)
{

    // Elasticity makes the circle bounce off the walls
    // velocity *= -e
    // Here, we adjust the old position based on the new position and the elasticity factor
    

    // Calculate velocity
    double vx = circle.x - circle.oldx;
    double vy = circle.y - circle.oldy;

    // Left wall
    if (circle.x < circle.radius) {
        circle.x = circle.radius;
        circle.oldx = circle.x + vx * ELASTICITY;
    }

    // Right wall
    if (circle.x > WIDTH - circle.radius) {
        circle.x = WIDTH - circle.radius;
        circle.oldx = circle.x + vx * ELASTICITY;
    }

    // Top wall
    if (circle.y < circle.radius) {
        circle.y = circle.radius;
        circle.oldy = circle.y + vy * ELASTICITY;
    }

    // Bottom wall
    if (circle.y > HEIGHT - circle.radius) {
        circle.y = HEIGHT - circle.radius;
        circle.oldy = circle.y + vy * ELASTICITY;
    }
}

// Main function
int main(int argc, char* argv[]) {
    // Initializing SDL
    SDL_Init(SDL_INIT_VIDEO);

    // Creating a window
    // SDL_WINDOW_SHOWN makes the window visible -> 0nly after this call
    SDL_Window* window = SDL_CreateWindow(
        "Gravity Simulation",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN
    );

    // Getting window surface
    SDL_Surface* surface = SDL_GetWindowSurface(window);

    
    struct Circle circle;
    circle.x = 300;
circle.y = 100;
circle.oldx = 300;
circle.oldy = 100;
circle.radius = 40;



    int simulation_running = 1;
    SDL_Event event;
    while (simulation_running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                simulation_running = 0;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    simulation_running = 0;
                }
            }

        }
        // Clear the surface
        SDL_FillRect(surface, NULL, COLOR_BLACK);
    UpdateCircle(circle);
    ApplyConstraints(circle);
    FillCircle(surface, circle, COLOR_WHITE);
        SDL_UpdateWindowSurface(window);


        SDL_Delay(16); // Delay to control frame rate - here ~60 FPS
    }


    // SDL_DestroyWindow(window);
    // SDL_Quit();
    return 0;
}



// g++ gravity.cpp -o gravity -I C:/MinGW/include -L C:/MinGW/lib -lmingw32 -lSDL2main -lSDL2 -lm