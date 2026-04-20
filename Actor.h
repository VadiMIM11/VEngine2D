#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <variant>
#include <vector>
#include <set>
#include <utility>
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "Component.h"

class ComponentDB;

class Actor
{
public:
	using ComponentProperty = std::variant<std::string, int, float, bool>;
	inline static int runtime_component_add_counter = 0;

	int id = -1;
	std::string actor_name;

	// Needed for returning nil / creating Lua tables
	lua_State* lua_state = nullptr;
	ComponentDB* component_db = nullptr;
	int spawned_frame = -1;
	bool pending_destroy = false;
	bool dont_destroy_on_load = false;

	// Component key -> type
	std::map<std::string, std::string> components;

	// Component key -> (property name -> property value)
	std::map<std::string, std::map<std::string, ComponentProperty>> component_properties;

	// Runtime instances
	std::map<std::string, std::shared_ptr<luabridge::LuaRef>> component_instances;
	std::set<std::string> pending_component_removals;

	struct PendingComponentAdd
	{
		std::string key;
		std::string type;
		std::shared_ptr<luabridge::LuaRef> instance;
		bool hasStart = false;
	};

	std::vector<PendingComponentAdd> pending_component_additions;

	void InitializeRuntimeContext(lua_State* state, ComponentDB* db);

	std::string GetName() const;

	int GetID() const;

	luabridge::LuaRef GetComponentByKey(const std::string& key);

	luabridge::LuaRef GetComponent(const std::string& type_name);

	luabridge::LuaRef GetComponents(const std::string& type_name);

	luabridge::LuaRef AddComponent(const std::string& type_name);

	void RemoveComponent(luabridge::LuaRef component_ref);

	void ProcessDeferredComponentOperations(std::vector<std::pair<Actor*, std::shared_ptr<luabridge::LuaRef>>>& on_start_queue);

	void DisableAllComponents();

	void MarkPendingDestroy();

	void InjectConvenienceReferences(std::shared_ptr<luabridge::LuaRef> instance_table);
};

