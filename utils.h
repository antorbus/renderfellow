#ifndef utils
#define utils

#include <SDL2/SDL.h>


typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
} application;

int init_SDL(int scale);



void quit_SDL();

#endif