#pragma once
#include "SDL2_image/SDL_image.h"
#include "Image.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iostream>

class ImageDB
{
public:
    ImageDB(SDL_Renderer* renderer);
    ~ImageDB() = default;

    std::shared_ptr<Image> LoadImageFromDisk(const std::string& folder_path, const std::string& image_name);
    void CreateDefaultParticleTextureWithName(const std::string& name);

private:
    SDL_Renderer* m_renderer = nullptr;

    std::vector<std::shared_ptr<Image>> images;
    std::unordered_map<int, std::shared_ptr<Image>> id_map;
    std::unordered_map<std::string, std::shared_ptr<Image>> path_map;

    // Cache for constructed path strings to avoid repeated allocations
    mutable std::string cached_path_buffer;
};

