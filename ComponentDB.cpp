#include "ComponentDB.h"


ComponentDB::ComponentDB()
{
	lua_state = std::shared_ptr<lua_State>(luaL_newstate(), lua_close);
	luaL_openlibs(lua_state.get());
}

void ComponentDB::EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table)
{
	luabridge::LuaRef new_metatable = luabridge::newTable(lua_state.get());
	new_metatable["__index"] = parent_table;

	instance_table.push(lua_state.get());
	new_metatable.push(lua_state.get());
	lua_setmetatable(lua_state.get(), -2);
	lua_pop(lua_state.get(), 1);
}

void ComponentDB::AddComponent(const std::string& key, std::shared_ptr<Component> component)
{
	components[key] = component;
}

void ComponentDB::RegisterNativeComponent(const std::string& key,
	const std::function<std::shared_ptr<luabridge::LuaRef>(lua_State*)>& factory,
	bool has_start,
	bool has_update,
	bool has_late_update,
	bool has_destroy)
{
	if (!factory)
	{
		return;
	}

	std::shared_ptr<Component> component = std::make_shared<Component>();
	component->type = key;
	component->backend = Component::Backend::NativeCpp;
	component->instanceFactory = factory;
	component->hasStart = has_start;
	component->hasUpdate = has_update;
	component->hasLateUpdate = has_late_update;
	component->hasDestroy = has_destroy;
	AddComponent(key, component);
}

std::shared_ptr<luabridge::LuaRef> ComponentDB::CreateComponentInstance(const std::string& type_name, const std::string& component_key)
{
	auto component_template = GetComponent(type_name);
	if (!component_template)
	{
		return nullptr;
	}

	std::shared_ptr<luabridge::LuaRef> instance;
	if (component_template->backend == Component::Backend::NativeCpp)
	{
		if (!component_template->instanceFactory)
		{
			return nullptr;
		}

		instance = component_template->instanceFactory(lua_state.get());
	}
	else
	{
		if (!component_template->componentRef)
		{
			return nullptr;
		}

		instance = std::make_shared<luabridge::LuaRef>(luabridge::newTable(lua_state.get()));
		EstablishInheritance(*instance, *component_template->componentRef);
	}

	if (!instance)
	{
		return nullptr;
	}

	(*instance)["key"] = component_key;
	(*instance)["type"] = type_name;
	(*instance)["enabled"] = true;
	return instance;
}

std::shared_ptr<Component> ComponentDB::GetComponent(const std::string& key)
{
	auto it = components.find(key);
	if (it != components.end())
	{
		return it->second;
	}
	return nullptr;
}

void ComponentDB::LoadComponentFromFile(const std::string& lua_file)
{
	std::filesystem::path path(lua_file);
	std::string component_name = path.stem().string();

	// Execute script via Lua-side dofile (no raw stack ops in this method)
	try
	{
		luabridge::LuaRef dofile_fn = luabridge::getGlobal(lua_state.get(), "dofile");
		if (!dofile_fn.isFunction())
		{
			std::cout << "Error: Lua global 'dofile' is not available.\n";
			return;
		}

		dofile_fn(lua_file);
	}
	catch (const luabridge::LuaException& e)
	{
#ifdef _DEBUG
		std::cout << "Error loading component script " << lua_file << ": " << e.what() << std::endl;
#endif // _DEBUG
		std::string file_name = Utils::ObtainFileNameFromPath(lua_file);
		std::string file_name_without_ext = Utils::ObtainFileNameWithoutExtension(file_name);
		std::cout << "problem with lua file " << file_name_without_ext;
		exit(0);
	}

	// Script must define a global table named after the file stem
	luabridge::LuaRef component_table = luabridge::getGlobal(lua_state.get(), component_name.c_str());
	if (component_table.isNil() || !component_table.isTable())
	{
		std::cout << "Error: script " << lua_file
			<< " must define a global table named " << component_name << std::endl;
		return;
	}

	std::shared_ptr<Component> new_component = std::make_shared<Component>();
	new_component->type = component_name;
	new_component->backend = Component::Backend::Lua;
	new_component->componentRef = std::make_shared<luabridge::LuaRef>(component_table);
	new_component->hasStart = component_table["OnStart"].isFunction();
	new_component->hasUpdate = component_table["OnUpdate"].isFunction();
	new_component->hasLateUpdate = component_table["OnLateUpdate"].isFunction();
	new_component->hasDestroy = component_table["OnDestroy"].isFunction();

	AddComponent(component_name, new_component);
}
