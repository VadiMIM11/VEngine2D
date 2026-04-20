#pragma once
#include "SDL.h"
#include "Helper.h"
#include <string>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Init now takes the window title (loaded by Engine from game.config)
    void Init(const std::string& window_title);

    void Clear();
    void Present();
    SDL_Renderer* GetRenderer() const { return renderer; }
    int GetWidth() const { return x_res; }
    int GetHeight() const { return y_res; }

    void SetZoom(float z);
    float GetZoom() const { return zoom_factor; }

    // Use to enable scaled rendering for world/actors (applies zoom_factor)
    // Call before drawing actors.
    void BeginWorldRender();

    // Use to restore scale to 1.0f so UI elements are not affected.
    // Call before drawing HUD/text/UI.
    void EndWorldRender();

    // Getter for easing factor (default 1.0f = instant)
    float GetCamEaseFactor() const { return cam_ease_factor; }

    // Getter for rendering.config option controlling x-scale flipping
    bool GetXScaleActorFlippingOnMovement() const { return x_scale_actor_flipping_on_movement; }

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Configurable settings with defaults
    int x_res = 640;
    int y_res = 360;
    float zoom_factor = 1.0f;

    // Camera easing factor (1.0f = instant)
    float cam_ease_factor = 1.0f;

    // Default clear color is White (255, 255, 255) per requirements
    int clear_r = 255;
    int clear_g = 255;
    int clear_b = 255;

    // Configurable: whether actors' x-scale flips based on movement intent
    bool x_scale_actor_flipping_on_movement = false;

    void LoadRenderingConfig();
};