#pragma once
#include <string>
#include "SDL2_image/SDL_image.h"
#include "SDL2/SDL.h"

class Image
{
public:
    int id = -1;
    std::string path = "";
    SDL_Texture* texture = nullptr;

    // specific destructor to clean up memory
    ~Image()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
};

