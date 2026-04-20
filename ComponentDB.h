#pragma once
#include <map>
#include <memory>
#include <string>
#include <filesystem>
#include <functional>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "Component.h"
#include "Utils.h"
class ComponentDB
{
public:
	ComponentDB();
	~ComponentDB() = default;

	void EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table);

	void AddComponent(const std::string& key, std::shared_ptr<Component> component);
	void RegisterNativeComponent(const std::string& key,
		const std::function<std::shared_ptr<luabridge::LuaRef>(lua_State*)>& factory,
		bool has_start,
		bool has_update,
		bool has_late_update,
		bool has_destroy = false);

	std::shared_ptr<luabridge::LuaRef> CreateComponentInstance(const std::string& type_name, const std::string& component_key);
	
	void LoadComponentFromFile(const std::string& lua_file);

	/* Getters */
	std::shared_ptr<Component> GetComponent(const std::string& key);
	std::shared_ptr<lua_State> GetLuaState() const { return lua_state; }

private:
	std::shared_ptr<lua_State> lua_state;
	std::map<std::string, std::shared_ptr<Component>> components; // key (component name aka type), Component
};

