#pragma once

#include "SDL2/SDL.h"
#include <unordered_map>
#include <array>
#include <deque>
#include <vector>

enum class INPUT_STATE
{
	INPUT_STATE_UP,
	INPUT_STATE_JUST_BECAME_DOWN,
	INPUT_STATE_DOWN,
	INPUT_STATE_JUST_BECAME_UP
};

enum class MOUSE_BUTTON
{
	LEFT = 1,
	MIDDLE = 2,
	RIGHT = 3,
	X1 = 4,
	X2 = 5
};

class InputManager
{
public:
	static void Init(); // Call before main loop begins
	static void ProcessEvent(const SDL_Event& e); // Call for each SDL event in the main loop
	static void LateUpdate(); // Call at very end of frame. Update "just" keys to their natural states


	static bool GetKey(SDL_Scancode keycode);
	static bool GetKeyDown(SDL_Scancode keycode);
	static bool GetKeyUp(SDL_Scancode keycode);

	// Mouse API
	// button should be one of MOUSE_BUTTON
	// Left = 1, Middle = 2, Right = 3, X1 = 4, X2 = 5
	static bool GetMouseButton(MOUSE_BUTTON button);
	static bool GetMouseButtonDown(MOUSE_BUTTON button);
	static bool GetMouseButtonUp(MOUSE_BUTTON button);

	static void GetMousePosition(int& out_x, int& out_y);
	static int GetMouseX();
	static int GetMouseY();

	// Wheel delta since last LateUpdate (signed)
	static float GetMouseWheelX();
	static float GetMouseWheelY();

private:
	static inline std::array<INPUT_STATE, SDL_NUM_SCANCODES> key_states = {};
	static inline std::vector<SDL_Scancode> keys_just_became_down = {};
	static inline std::vector<SDL_Scancode> keys_just_became_up = {};

	// Mouse internals
	// SDL mouse buttons are 1..5; map to indices 0..4 via (button - 1)
	static constexpr int MAX_MOUSE_BUTTONS = 5;
	static inline std::array<INPUT_STATE, MAX_MOUSE_BUTTONS> mouse_button_states = {};
	static inline std::vector<int> mouse_buttons_just_became_down = {};
	static inline std::vector<int> mouse_buttons_just_became_up = {};

	static inline int mouse_x = 0;
	static inline int mouse_y = 0;
	// Wheel (accumulated between ProcessEvent and LateUpdate)
	static inline float mouse_wheel_x = 0.0f;
	static inline float mouse_wheel_y = 0.0f;

	// Helper to convert SDL button (via MOUSE_BUTTON) to index; returns -1 if out of range
	static inline int MouseButtonToIndex(MOUSE_BUTTON button) { int v = static_cast<int>(button); return (v >= 1 && v <= MAX_MOUSE_BUTTONS) ? (v - 1) : -1; }
};

