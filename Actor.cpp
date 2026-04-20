#include "Actor.h"
#include "ComponentDB.h"
#include <algorithm>

void Actor::InitializeRuntimeContext(lua_State* state, ComponentDB* db)
{
	lua_state = state;
	component_db = db;
}

std::string Actor::GetName() const
{
	return actor_name;
}

int Actor::GetID() const
{
	return id;
}

luabridge::LuaRef Actor::GetComponentByKey(const std::string& key)
{
	if (pending_component_removals.find(key) != pending_component_removals.end())
	{
		return luabridge::LuaRef(lua_state);
	}

	auto it = component_instances.find(key);
	if (it != component_instances.end() && it->second)
	{
		return *(it->second);
	}
	return luabridge::LuaRef(lua_state);
}

luabridge::LuaRef Actor::GetComponent(const std::string& type_name)
{
	for (const auto& [key, type] : components)
	{
		if (pending_component_removals.find(key) != pending_component_removals.end())
		{
			continue;
		}

		if (type == type_name)
		{
			auto it = component_instances.find(key);
			if (it != component_instances.end() && it->second)
			{
				return *(it->second);
			}
		}
	}
	return luabridge::LuaRef(lua_state);
}

luabridge::LuaRef Actor::GetComponents(const std::string& type_name)
{
	luabridge::LuaRef result = luabridge::newTable(lua_state);
	int lua_index = 1;

	for (const auto& [key, type] : components)
	{
		if (pending_component_removals.find(key) != pending_component_removals.end())
		{
			continue;
		}

		if (type == type_name)
		{
			auto it = component_instances.find(key);
			if (it != component_instances.end() && it->second)
			{
				result[lua_index++] = *(it->second);
			}
		}
	}

	return result;
}

luabridge::LuaRef Actor::AddComponent(const std::string& type_name)
{
	if (lua_state == nullptr || component_db == nullptr)
	{
		return luabridge::LuaRef(lua_state);
	}

	std::shared_ptr<Component> component_template = component_db->GetComponent(type_name);
	if (!component_template)
	{
		return luabridge::LuaRef(lua_state);
	}

	const std::string key = "r" + std::to_string(runtime_component_add_counter++);
	auto instance = component_db->CreateComponentInstance(type_name, key);
	if (!instance)
	{
		return luabridge::LuaRef(lua_state);
	}

	InjectConvenienceReferences(instance);

	pending_component_additions.push_back(PendingComponentAdd{ key, type_name, instance, component_template->hasStart });
	return *instance;
}

void Actor::RemoveComponent(luabridge::LuaRef component_ref)
{
	if (component_ref.isNil() || (!component_ref.isTable() && !component_ref.isUserdata()))
	{
		return;
	}

	component_ref["enabled"] = false;

	luabridge::LuaRef key_ref = component_ref["key"];
	if (key_ref.isString())
	{
		pending_component_removals.insert(key_ref.cast<std::string>());
	}
}

void Actor::ProcessDeferredComponentOperations(std::vector<std::pair<Actor*, std::shared_ptr<luabridge::LuaRef>>>& on_start_queue)
{
	if (!pending_component_removals.empty())
	{
		for (const std::string& key : pending_component_removals)
		{
			components.erase(key);
			component_instances.erase(key);
			component_properties.erase(key);
		}

		pending_component_additions.erase(
			std::remove_if(pending_component_additions.begin(), pending_component_additions.end(),
				[this](const PendingComponentAdd& add) {
					return pending_component_removals.find(add.key) != pending_component_removals.end();
				}),
			pending_component_additions.end());

		pending_component_removals.clear();
	}

	for (const PendingComponentAdd& add : pending_component_additions)
	{
		components[add.key] = add.type;
		component_instances[add.key] = add.instance;
		if (add.hasStart)
		{
			on_start_queue.emplace_back(this, add.instance);
		}
	}

	pending_component_additions.clear();
}

void Actor::DisableAllComponents()
{
	for (const auto& [key, instance] : component_instances)
	{
		if (instance)
		{
			(*instance)["enabled"] = false;
		}
	}

	for (const PendingComponentAdd& add : pending_component_additions)
	{
		if (add.instance)
		{
			(*add.instance)["enabled"] = false;
		}
	}
}

void Actor::MarkPendingDestroy()
{
	pending_destroy = true;
	DisableAllComponents();
}

void Actor::InjectConvenienceReferences(std::shared_ptr<luabridge::LuaRef> instance_table)
{
	(*instance_table)["actor"] = this;

	luabridge::LuaRef actorName = (*instance_table)["actor_name"];
	if (actorName.isNil())
	{
		(*instance_table)["actor_name"] = actor_name;
	}

	luabridge::LuaRef actorId = (*instance_table)["id"];
	if (actorId.isNil())
	{
		(*instance_table)["id"] = id;
	}
}
