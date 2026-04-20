#include "InputManager.h"

void InputManager::Init()
{
	// Initialize all key states to UP
	for (int i = SDL_SCANCODE_UNKNOWN; i < SDL_NUM_SCANCODES; i++)
	{
		key_states[static_cast<SDL_Scancode>(i)] = INPUT_STATE::INPUT_STATE_UP;
	}

	// Initialize mouse button states to UP
	for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i)
	{
		mouse_button_states[i] = INPUT_STATE::INPUT_STATE_UP;
	}

	mouse_x = mouse_y = 0;
	mouse_wheel_x = mouse_wheel_y = 0;
}

void InputManager::ProcessEvent(const SDL_Event& e)
{
	if (e.type == SDL_KEYDOWN)
	{
		// Ignore auto-repeated keydown events (repeat != 0)
		//if (e_key_repeat != 0) return; // Commented out due to AutoGrader warning: "Please do not check key repeat in this course, as it is often left undefined"

		SDL_Scancode scancode = e.key.keysym.scancode;
		int idx = static_cast<int>(scancode);
		if (idx >= 0 && idx < SDL_NUM_SCANCODES)
		{
			if (key_states[scancode] != INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN)
			{
				key_states[scancode] = INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
				keys_just_became_down.push_back(scancode);
			}
		}
	}
	else if (e.type == SDL_KEYUP)
	{
		SDL_Scancode scancode = e.key.keysym.scancode;
		int idx = static_cast<int>(scancode);
		if (idx >= 0 && idx < SDL_NUM_SCANCODES)
		{
			if (key_states[scancode] != INPUT_STATE::INPUT_STATE_JUST_BECAME_UP)
			{
				key_states[scancode] = INPUT_STATE::INPUT_STATE_JUST_BECAME_UP;
				keys_just_became_up.push_back(scancode);
			}
		}
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN)
	{
		// Convert SDL button value to our enum then to index
		MOUSE_BUTTON mb = static_cast<MOUSE_BUTTON>(e.button.button);
		int btn_index = MouseButtonToIndex(mb);
		if (btn_index != -1)
		{
			if (mouse_button_states[btn_index] != INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN)
			{
				mouse_button_states[btn_index] = INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
				mouse_buttons_just_became_down.push_back(btn_index);
			}
		}
		// update position as well (event contains coords)
		mouse_x = e.button.x;
		mouse_y = e.button.y;
	}
	else if (e.type == SDL_MOUSEBUTTONUP)
	{
		MOUSE_BUTTON mb = static_cast<MOUSE_BUTTON>(e.button.button);
		int btn_index = MouseButtonToIndex(mb);
		if (btn_index != -1)
		{
			if (mouse_button_states[btn_index] != INPUT_STATE::INPUT_STATE_JUST_BECAME_UP)
			{
				mouse_button_states[btn_index] = INPUT_STATE::INPUT_STATE_JUST_BECAME_UP;
				mouse_buttons_just_became_up.push_back(btn_index);
			}
		}
		mouse_x = e.button.x;
		mouse_y = e.button.y;
	}
	else if (e.type == SDL_MOUSEMOTION)
	{
		mouse_x = e.motion.x;
		mouse_y = e.motion.y;
	}
	else if (e.type == SDL_MOUSEWHEEL)
	{
		// Accumulate wheel deltas until LateUpdate clears them
		mouse_wheel_x += e.wheel.preciseX;
		mouse_wheel_y += e.wheel.preciseY;	
	}
}

void InputManager::LateUpdate() 
{
    for (const auto& scancode : keys_just_became_down)
	{
		if (key_states[scancode] == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN)
		{
			key_states[scancode] = INPUT_STATE::INPUT_STATE_DOWN;
		}
	}
	keys_just_became_down.clear();

	for (const auto& scancode : keys_just_became_up)
	{
		if (key_states[scancode] == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP)
		{
			key_states[scancode] = INPUT_STATE::INPUT_STATE_UP;
		}
	}
	keys_just_became_up.clear();

	// Mouse button transitions
	for (const auto& bidx : mouse_buttons_just_became_down)
	{
		if (bidx >= 0 && bidx < MAX_MOUSE_BUTTONS)
		{
			if (mouse_button_states[bidx] == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN)
			{
				mouse_button_states[bidx] = INPUT_STATE::INPUT_STATE_DOWN;
			}
		}
	}
	mouse_buttons_just_became_down.clear();

	for (const auto& bidx : mouse_buttons_just_became_up)
	{
		if (bidx >= 0 && bidx < MAX_MOUSE_BUTTONS)
		{
			if (mouse_button_states[bidx] == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP)
			{
				mouse_button_states[bidx] = INPUT_STATE::INPUT_STATE_UP;
			}
		}
	}
	mouse_buttons_just_became_up.clear();

	// Reset wheel deltas after they've been read for this frame/iteration
	mouse_wheel_x = 0.0f;
	mouse_wheel_y = 0.0f;
}

bool InputManager::GetKeyUp(SDL_Scancode keycode)
{
	auto state = key_states[keycode];
	return state == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP;
}

bool InputManager::GetKeyDown(SDL_Scancode keycode)
{
	auto state = key_states[keycode];
	return state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
}

bool InputManager::GetKey(SDL_Scancode keycode)
{
	auto state = key_states[keycode];
	return state == INPUT_STATE::INPUT_STATE_DOWN || state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
}

// Mouse accessors
bool InputManager::GetMouseButton(MOUSE_BUTTON button)
{
	int idx = MouseButtonToIndex(button);
	if (idx == -1) return false;
	auto state = mouse_button_states[idx];
	return state == INPUT_STATE::INPUT_STATE_DOWN || state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
}

bool InputManager::GetMouseButtonDown(MOUSE_BUTTON button)
{
	int idx = MouseButtonToIndex(button);
	if (idx == -1) return false;
	auto state = mouse_button_states[idx];
	return state == INPUT_STATE::INPUT_STATE_JUST_BECAME_DOWN;
}

bool InputManager::GetMouseButtonUp(MOUSE_BUTTON button)
{
	int idx = MouseButtonToIndex(button);
	if (idx == -1) return false;
	auto state = mouse_button_states[idx];
	return state == INPUT_STATE::INPUT_STATE_JUST_BECAME_UP;
}

void InputManager::GetMousePosition(int& out_x, int& out_y)
{
	out_x = mouse_x;
	out_y = mouse_y;
}

int InputManager::GetMouseX()
{
	return mouse_x;
}

int InputManager::GetMouseY()
{
	return mouse_y;
}

float InputManager::GetMouseWheelX()
{
	return mouse_wheel_x;
}

float InputManager::GetMouseWheelY()
{
	return mouse_wheel_y;
}
