#include "ImageDB.h"

ImageDB::ImageDB(SDL_Renderer* renderer)
    : m_renderer(renderer)
{

}

std::shared_ptr<Image> ImageDB::LoadImageFromDisk(const std::string& folder_path, const std::string& image_name)
{
    auto by_name_it = path_map.find(image_name);
    if (by_name_it != path_map.end())
    {
        return by_name_it->second;
    }

    // Reuse cached buffer to avoid repeated allocations
    cached_path_buffer.clear();
    cached_path_buffer.reserve(folder_path.size() + image_name.size() + 4);
    cached_path_buffer.append(folder_path);
    cached_path_buffer.append(image_name);
    cached_path_buffer.append(".png");

    // 1. Check if image is already cached
    auto it = path_map.find(cached_path_buffer);
    if (it != path_map.end())
    {
        return it->second;
    }

    // 2. Load Texture from Disk
    SDL_Texture* newTexture = IMG_LoadTexture(m_renderer, cached_path_buffer.c_str());

    // 3. Error Handling
    if (newTexture == nullptr)
    {
        std::cout << "error: missing image " << image_name;
        exit(0);
    }

    // 4. Create Shared Pointer
    std::shared_ptr<Image> newImage = std::make_shared<Image>();

    newImage->path = cached_path_buffer;
    newImage->texture = newTexture;
    newImage->id = static_cast<int>(images.size());

    // 5. Store in containers
    images.push_back(newImage);

    id_map.emplace(newImage->id, newImage);
    path_map.emplace(newImage->path, newImage);

    return newImage;
}

void ImageDB::CreateDefaultParticleTextureWithName(const std::string& name)
{
    if (path_map.find(name) != path_map.end())
    {
        return;
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);
    if (surface == nullptr)
    {
        return;
    }

    Uint32 white_color = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
    SDL_FillRect(surface, NULL, white_color);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_FreeSurface(surface);

    if (texture == nullptr)
    {
        return;
    }

    std::shared_ptr<Image> new_image = std::make_shared<Image>();
    new_image->path = name;
    new_image->texture = texture;
    new_image->id = static_cast<int>(images.size());

    images.push_back(new_image);
    id_map.emplace(new_image->id, new_image);
    path_map.emplace(name, new_image);
}