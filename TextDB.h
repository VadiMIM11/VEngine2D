#pragma once
#include "SDL2_ttf/SDL_ttf.h"
#include <string>
#include <iostream>
#include <unordered_map>
#include "SDL.h"

// Simple wrapper to manage Font resources
class TextDB
{
public:
    TextDB();
    ~TextDB();

    // Loads a font. If logic fails, prints error and exits as per requirements.
    TTF_Font* LoadFont(const std::string& path, int size);

    // Draws text at specific x, y using the provided renderer
    void DrawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);

private:
    std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> font_map;       
};