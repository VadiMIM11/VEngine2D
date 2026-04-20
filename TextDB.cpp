#include "TextDB.h"
#include "Helper.h" 
#include <cstdlib>

TextDB::TextDB()
{
    if (TTF_Init() == -1)
    {
        std::cout << "TTF_Init: " << TTF_GetError();
        exit(0);
    }
}

TextDB::~TextDB()
{
	for (auto& path_pair : font_map)
	{
		for (auto& size_pair : path_pair.second)
		{
			TTF_CloseFont(size_pair.second);
		}
	}
    font_map.clear();
    TTF_Quit();
}

TTF_Font* TextDB::LoadFont(const std::string& path, int size)
{
	if (size <= 0)
	{
		return nullptr;
	}

	auto path_it = font_map.find(path);
	if (path_it != font_map.end())
    {
		auto size_it = path_it->second.find(size);
		if (size_it != path_it->second.end())
		{
			return size_it->second;
		}
    }

    TTF_Font* newFont = TTF_OpenFont(path.c_str(), size);

    if (newFont == nullptr)
    {
        return nullptr;
    }

	font_map[path][size] = newFont;
    return newFont;
}

void TextDB::DrawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color)
{
    if (!font || text.empty()) return;

    // 1. Render Text to Surface
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) return;

    // 2. Create Texture
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    // 3. Define Rects
    // We need the dimensions.
    int w = surface->w;
    int h = surface->h;

    // Cleanup surface immediately (data is now in texture)
    SDL_FreeSurface(surface);

    if (texture)
    {
        // 4. Render using Helper
        SDL_FRect dst_frect = {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(w),
            static_cast<float>(h)
        };

        Helper::SDL_RenderCopy(renderer, texture, NULL, &dst_frect);

        // 5. Cleanup Texture
        SDL_DestroyTexture(texture);
    }
}