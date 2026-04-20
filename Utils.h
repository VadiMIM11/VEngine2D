#pragma once
#include <string>
#include <cctype>
#include "Renderer.h"
#include "ActorDB.h"

struct Color
{
	int r;
	int g;
	int b;
	int a;

	constexpr Color() : r(0), g(0), b(0), a(255) {}
	constexpr Color(int red, int green, int blue, int alpha = 255)
		: r(red), g(green), b(blue), a(alpha)
	{
	}

	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Pink;
	static const Color Orange;
};

inline constexpr Color Color::Red{ 255, 0, 0 };
inline constexpr Color Color::Green{ 0, 255, 0 };
inline constexpr Color Color::Blue{ 0, 0, 255 };
inline constexpr Color Color::Pink{ 255, 192, 203 };
inline constexpr Color Color::Orange{ 255, 165, 0 };


class Utils
{
public:
	static std::string ObtainWordAfterPhrase(const std::string& input, const std::string& phrase)
	{
		// Find the position of the phrase in the string
		size_t pos = input.find(phrase);

		// If phrase is not found, return an empty string
		if (pos == std::string::npos) return "";

		// Find the starting position of the next word (skip spaces after the phrase)
		pos += phrase.length();
		while (pos < input.size() && std::isspace(input[pos]))
		{
			++pos;
		}

		// If we're at the end of the string, return an empty string
		if (pos == input.size()) return "";

		// Find the end position of the word (until a space or the end of the string)
		size_t endPos = pos;
		while (endPos < input.size() && !std::isspace(input[endPos]))
		{
			++endPos;
		}

		// Extract and return the word
		return input.substr(pos, endPos - pos);
	}

	static std::string ObtainFileNameFromPath(const std::string& filePath)
	{
		size_t lastSlashPos = filePath.find_last_of("/\\");
		if (lastSlashPos == std::string::npos)
			return filePath; // No slashes, return the whole path
		return filePath.substr(lastSlashPos + 1);
	}

	static std::string ObtainFileNameWithoutExtension(const std::string& filePath)
	{
		std::string fileName = ObtainFileNameFromPath(filePath);
		size_t lastDotPos = fileName.find_last_of('.');
		if (lastDotPos == std::string::npos)
			return fileName; // No dot, return the whole filename
		return fileName.substr(0, lastDotPos);
	}

	static bool CheckFileExistence(const std::string& filePath)
	{
		return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
	}

	static bool CheckFolderExistence(const std::string& folderPath)
	{
		return std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath);
	}

	static std::string ToLowerAscii(std::string value)
	{
		for (char& c : value)
		{
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		return value;
	}


	static void DrawCross(const Renderer& renderer, const glm::ivec2& center, int size = 6, const Color& color = Color::Red)
	{
		auto r = color.r;
		auto g = color.g;
		auto b = color.b;
		auto a = color.a;
		SDL_SetRenderDrawColor(renderer.GetRenderer(), r, g, b, a);
		int x = center.x;
		int y = center.y;
		SDL_RenderDrawLine(renderer.GetRenderer(), x - size, y, x + size, y);
		SDL_RenderDrawLine(renderer.GetRenderer(), x, y - size, x, y + size);
	}


	static void DrawRectangle(const Renderer& renderer, float x, float y, float width, float height, const Color& color = Color::Red)
	{
		auto r = color.r;
		auto g = color.g;
		auto b = color.b;
		auto a = color.a;
		SDL_SetRenderDrawColor(renderer.GetRenderer(), r, g, b, a);
		SDL_FRect dst_rect = { x, y, width, height };
		SDL_RenderDrawRectF(renderer.GetRenderer(), &dst_rect);
	}

	static bool AABBCollision(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh)
	{
		// Inputs are object centers (ax,ay) and full widths/heights (aw,ah), (bx,by) and (bw,bh).
		if (aw <= 0 || ah <= 0 || bw <= 0 || bh <= 0)
		{
			return false; // Invalid boxes cannot collide
		}

		// Convert center -> top-left for AABB test
		float a_min_x = ax - (aw * 0.5f);
		float a_min_y = ay - (ah * 0.5f);
		float b_min_x = bx - (bw * 0.5f);
		float b_min_y = by - (bh * 0.5f);

		return (a_min_x < b_min_x + bw) && (a_min_x + aw > b_min_x) &&
			(a_min_y < b_min_y + bh) && (a_min_y + ah > b_min_y);
	}

	static const std::unordered_map<std::string, SDL_Scancode>& GetKeycodeToScancodeMap()
	{
		static const std::unordered_map<std::string, SDL_Scancode> __keycode_to_scancode = {
			// Directional (arrow) Keys
			{"up", SDL_SCANCODE_UP},
			{"down", SDL_SCANCODE_DOWN},
			{"right", SDL_SCANCODE_RIGHT},
			{"left", SDL_SCANCODE_LEFT},

			// Misc Keys
			{"escape", SDL_SCANCODE_ESCAPE},

			// Modifier Keys
			{"lshift", SDL_SCANCODE_LSHIFT},
			{"rshift", SDL_SCANCODE_RSHIFT},
			{"lctrl", SDL_SCANCODE_LCTRL},
			{"rctrl", SDL_SCANCODE_RCTRL},
			{"lalt", SDL_SCANCODE_LALT},
			{"ralt", SDL_SCANCODE_RALT},

			// Editing Keys
			{"tab", SDL_SCANCODE_TAB},
			{"return", SDL_SCANCODE_RETURN},
			{"enter", SDL_SCANCODE_RETURN},
			{"backspace", SDL_SCANCODE_BACKSPACE},
			{"delete", SDL_SCANCODE_DELETE},
			{"insert", SDL_SCANCODE_INSERT},

			// Character Keys
			{"space", SDL_SCANCODE_SPACE},
			{"a", SDL_SCANCODE_A},
			{"b", SDL_SCANCODE_B},
			{"c", SDL_SCANCODE_C},
			{"d", SDL_SCANCODE_D},
			{"e", SDL_SCANCODE_E},
			{"f", SDL_SCANCODE_F},
			{"g", SDL_SCANCODE_G},
			{"h", SDL_SCANCODE_H},
			{"i", SDL_SCANCODE_I},
			{"j", SDL_SCANCODE_J},
			{"k", SDL_SCANCODE_K},
			{"l", SDL_SCANCODE_L},
			{"m", SDL_SCANCODE_M},
			{"n", SDL_SCANCODE_N},
			{"o", SDL_SCANCODE_O},
			{"p", SDL_SCANCODE_P},
			{"q", SDL_SCANCODE_Q},
			{"r", SDL_SCANCODE_R},
			{"s", SDL_SCANCODE_S},
			{"t", SDL_SCANCODE_T},
			{"u", SDL_SCANCODE_U},
			{"v", SDL_SCANCODE_V},
			{"w", SDL_SCANCODE_W},
			{"x", SDL_SCANCODE_X},
			{"y", SDL_SCANCODE_Y},
			{"z", SDL_SCANCODE_Z},
			{"0", SDL_SCANCODE_0},
			{"1", SDL_SCANCODE_1},
			{"2", SDL_SCANCODE_2},
			{"3", SDL_SCANCODE_3},
			{"4", SDL_SCANCODE_4},
			{"5", SDL_SCANCODE_5},
			{"6", SDL_SCANCODE_6},
			{"7", SDL_SCANCODE_7},
			{"8", SDL_SCANCODE_8},
			{"9", SDL_SCANCODE_9},
			{"/", SDL_SCANCODE_SLASH},
			{";", SDL_SCANCODE_SEMICOLON},
			{"=", SDL_SCANCODE_EQUALS},
			{"-", SDL_SCANCODE_MINUS},
			{".", SDL_SCANCODE_PERIOD},
			{",", SDL_SCANCODE_COMMA},
			{"[", SDL_SCANCODE_LEFTBRACKET},
			{"]", SDL_SCANCODE_RIGHTBRACKET},
			{"\\", SDL_SCANCODE_BACKSLASH},
			{"'", SDL_SCANCODE_APOSTROPHE}
		};

		return __keycode_to_scancode;
	}
};

