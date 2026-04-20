#pragma once
#include <memory>
#include <string>
#include <functional>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"

class Component
{
public:
	explicit Component() = default;

	enum class Backend
	{
		Lua,
		NativeCpp
	};

	bool IsEnabled() const;

	std::shared_ptr<luabridge::LuaRef> componentRef;
	std::function<std::shared_ptr<luabridge::LuaRef>(lua_State*)> instanceFactory;
	std::string type;
	Backend backend = Backend::Lua;

	bool hasStart = false;
	bool hasUpdate = false;
	bool hasLateUpdate = false;
	bool hasDestroy = false;
};

