#include "Renderer.h"
#include "JsonParser.h"
#include "rapidjson/document.h"
#include <iostream>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
}

void Renderer::LoadRenderingConfig()
{
    // Defaults are already set in the class definition (640x360, White)

    rapidjson::Document doc;
    std::vector<char> buffer;

    // Try to load resources/rendering.config
    if (JsonParser::ReadJsonFileInsitu("resources/rendering.config", doc, buffer))
    {

        if (doc.HasMember("x_resolution") && doc["x_resolution"].IsInt())
        {
            x_res = doc["x_resolution"].GetInt();
        }

        if (doc.HasMember("y_resolution") && doc["y_resolution"].IsInt())
        {
            y_res = doc["y_resolution"].GetInt();
        }

        if (doc.HasMember("clear_color_r") && doc["clear_color_r"].IsInt())
        {
            clear_r = doc["clear_color_r"].GetInt();
        }

        if (doc.HasMember("clear_color_g") && doc["clear_color_g"].IsInt())
        {
            clear_g = doc["clear_color_g"].GetInt();
        }

        if (doc.HasMember("clear_color_b") && doc["clear_color_b"].IsInt())
        {
            clear_b = doc["clear_color_b"].GetInt();
        }
        if (doc.HasMember("zoom_factor") && doc["zoom_factor"].IsNumber())
        {
            zoom_factor = doc["zoom_factor"].GetFloat();
        }
        if (doc.HasMember("cam_ease_factor") && doc["cam_ease_factor"].IsNumber())
        {
            cam_ease_factor = doc["cam_ease_factor"].GetFloat();
        }
        if (doc.HasMember("x_scale_actor_flipping_on_movement") && doc["x_scale_actor_flipping_on_movement"].IsBool())
        {
            x_scale_actor_flipping_on_movement = doc["x_scale_actor_flipping_on_movement"].GetBool();
        }

    }
    // If file doesn't exist, we just keep the defaults.
}

void Renderer::Init(const std::string& window_title)
{
    LoadRenderingConfig();

    // Create Window using Helper
    window = Helper::SDL_CreateWindow(
        window_title.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        x_res,
        y_res,
        SDL_WINDOW_SHOWN
    );

    // Create Renderer using Helper
    // Index -1, Flags: VSYNC | ACCELERATED
    renderer = Helper::SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );

    // Set the blending mode (standard SDL call)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Ensure renderer starts unscaled for UI; world scaling will be applied when requested.
    if (renderer)
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void Renderer::SetZoom(float z)
{
    if (z <= 0.0f) return;
    zoom_factor = z;
    // If renderer exists and world is currently expected to be scaled, apply immediately.
    // We intentionally do not force UI scaling here; BeginWorldRender/EndWorldRender control when scale is active.
    if (renderer)
    {
        // Do not change global renderer scale here; BeginWorldRender will apply zoom when needed.
        // However also safe to set the hardware scale to 1 here to avoid unexpected UI scaling.
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    }
}

void Renderer::BeginWorldRender()
{
    if (renderer)
    {
        // Apply the configured zoom for all subsequent draw calls (actors)
        SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);
    }
}

void Renderer::EndWorldRender()
{
    if (renderer)
    {
        // Reset scale back to neutral so UI/text/hud are drawn at normal size
        SDL_RenderSetScale(renderer, 1.0f, 1.0f);
    }
}

void Renderer::Clear()
{
    // Set draw color based on config
    SDL_SetRenderDrawColor(renderer, (Uint8)clear_r, (Uint8)clear_g, (Uint8)clear_b, 255);

    // Clear the screen
    SDL_RenderClear(renderer);
}

void Renderer::Present()
{
    // Must use Helper wrapper to support autograder/frame dumping
    Helper::SDL_RenderPresent(renderer);
}