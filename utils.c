#include "utils.h"

const int width;
const int height;
application app;


int init_SDL(int scale){
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_Window* window = SDL_CreateWindow("Render Fellow", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, scale*width, scale*height, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    app.window = window;
    app.renderer = renderer;
        
    if (!app.window){
        printf("Failed to open %d x %d window: %s\n", width, height, SDL_GetError());
        exit(1);
    }
    if (!app.renderer){
        printf("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_RenderSetLogicalSize(renderer, width, height);

    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "nearest", SDL_HINT_OVERRIDE);

    //SDL_ShowCursor(SDL_DISABLE);

    return 0;
}




void quit_SDL(){
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
}
