#include "Engine.h"
#include "Rigidbody.h"
#include "ParticleSystem.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

Engine::Engine()
{
	isRunning = false;
}

Engine::~Engine()
{
	on_start_call_queue.clear();
	pending_on_start_call_queue.clear();
	actors_pending_destruction.clear();
	scenes.clear();
	actor_template_cache.clear();
	ScriptBindings::Shutdown();
}

void Engine::Run()
{
	isRunning = true;
	renderer = std::make_shared<Renderer>();
	componentDB = std::make_shared<ComponentDB>();
	textDB = std::make_shared<TextDB>();
	audioDB = std::make_shared<AudioDB>();
	LoadGameConfig();
	renderer->Init(game_title);
	imageDB = std::make_shared<ImageDB>(renderer->GetRenderer());

	ParticleSystem::SetImageDB(imageDB.get());

	LoadComponents();
	ScriptBindings::RegisterCore(componentDB->GetLuaState().get());
	componentDB->RegisterNativeComponent(
		"Rigidbody",
		[](lua_State* L)
		{
			return std::make_shared<luabridge::LuaRef>(L, Rigidbody{});
		},
		true,
		true,
		true,
		true);
	componentDB->RegisterNativeComponent(
		"ParticleSystem",
		[](lua_State* L)
		{
			return std::make_shared<luabridge::LuaRef>(L, ParticleSystem{});
		},
		true,
		true,
		false,
		false);
	ScriptBindings::RegisterRendering(componentDB->GetLuaState().get(), renderer.get(), textDB.get(), imageDB.get());
	ScriptBindings::RegisterAudio(componentDB->GetLuaState().get(), audioDB.get());
	ScriptBindings::SetRuntimeActorOps(
		[this](const std::string& template_name) { return InstantiateActor(template_name); },
		[this](Actor* actor) { DestroyActor(actor); });
	ScriptBindings::SetSceneOps(
		[this](const std::string& scene_name) { QueueSceneLoad(scene_name); },
		[this]() { return GetCurrentSceneName(); },
		[this](Actor* actor) { MarkActorDontDestroy(actor); });

	LoadNewScene(initial_scene_name);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(&Engine::MainLoopThunk, this, 0, 1);
#else
	while (isRunning) { Tick(); }
#endif
}

void Engine::Tick()
{
	Input();
	Update();
	Rigidbody::StepPhysicsWorld();
	Render();

#ifdef __EMSCRIPTEN__
	if (!isRunning) emscripten_cancel_main_loop();
#endif
}

#ifdef __EMSCRIPTEN__
void Engine::MainLoopThunk(void* arg)
{
	static_cast<Engine*>(arg)->Tick();
}
#endif

void Engine::Input()
{
	SDL_Event e;
	while (Helper::SDL_PollEvent(&e))
	{
		if (e.type == SDL_QUIT)
		{
			isRunning = false;
		}
		else
		{
			InputManager::ProcessEvent(e);
		}
	}
}

void Engine::Update()
{
	if (!pending_scene_load_name.empty())
	{
		LoadNewScene(pending_scene_load_name);
		pending_scene_load_name.clear();
	}

	is_in_update = true;

	// 1) Drain OnStart queue first (for all newly created components)
	if (!on_start_call_queue.empty())
	{
		for (const auto& [actor, instance_ptr] : on_start_call_queue)
		{
			InvokeLifecycle(actor, *instance_ptr, "OnStart");
		}
		on_start_call_queue.clear();
	}

	if (scenes.empty())
	{
		is_in_update = false;
		return;
	}

	// Active scene
	Scene& scene = *scenes.back();
	const int current_frame = Helper::GetFrameNumber();

	// 2) Iterate actors by ID (explicitly sorted)
	std::vector<Actor*> actors_by_id;
	actors_by_id.reserve(scene.actors.GetActors()->size());
	for (Actor& actor : *scene.actors.GetActors())
	{
		if (actor.pending_destroy)
		{
			continue;
		}

		if (actor.spawned_frame == current_frame)
		{
			continue;
		}

		actors_by_id.push_back(&actor);
	}
	std::sort(actors_by_id.begin(), actors_by_id.end(),
		[](const Actor* a, const Actor* b) { return a->id < b->id; });

	// 3) OnUpdate for every component (actors by ID, components by key)
	for (Actor* actor : actors_by_id)
	{
		for (const auto& [component_key, instance_ptr] : actor->component_instances)
		{
			InvokeLifecycle(actor, *instance_ptr, "OnUpdate");
		}
	}

	// 4) OnLateUpdate for every component (same ordering)
	for (Actor* actor : actors_by_id)
	{
		for (const auto& [component_key, instance_ptr] : actor->component_instances)
		{
			InvokeLifecycle(actor, *instance_ptr, "OnLateUpdate");
		}
	}

	// 5) OnDestroy for components removed during this frame (order by key)
	for (Actor* actor : actors_by_id)
	{
		for (const std::string& key : actor->pending_component_removals)
		{
			auto it = actor->component_instances.find(key);
			if (it != actor->component_instances.end() && it->second)
			{
				InvokeDestroyLifecycle(actor, *(it->second));
			}
		}
	}

	for (Actor& actor : *scene.actors.GetActors())
	{
		if (!actor.pending_destroy)
		{
			actor.ProcessDeferredComponentOperations(pending_on_start_call_queue);
		}
	}

	if (!actors_pending_destruction.empty())
	{
		actors_pending_destruction.clear();
	}

	if (!pending_on_start_call_queue.empty())
	{
		on_start_call_queue.insert(on_start_call_queue.end(), pending_on_start_call_queue.begin(), pending_on_start_call_queue.end());
		pending_on_start_call_queue.clear();
	}

	ScriptBindings::ApplyPendingEventSubscriptions();
	InputManager::LateUpdate();
	is_in_update = false;
}

void Engine::Render()
{
	renderer->Clear();
	ScriptBindings::FlushRenderingQueues();
	renderer->	Present();
}

void Engine::LoadComponents()
{

	if (!Utils::CheckFolderExistence(components_folder))
	{
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(components_folder))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".lua")
		{
			// Load the Lua script and register it in the Component DB
			componentDB->LoadComponentFromFile(entry.path().string());
		}
	}
}

void Engine::LoadGameConfig()
{
	rapidjson::Document doc;
	std::vector<char> buffer;

	if (!Utils::CheckFolderExistence(resources_folder))
	{
		std::cout << "error: resources/ missing";
		exit(0);
	}


	// Load resources/game.config
	if (JsonParser::ReadJsonFileInsitu(game_config_path, doc, buffer))
	{
		if (doc.HasMember("game_title") && doc["game_title"].IsString())
			game_title = doc["game_title"].GetString();
		if (doc.HasMember("initial_scene") && doc["initial_scene"].IsString())
			initial_scene_name = doc["initial_scene"].GetString();


		// Note: x_scale_actor_flipping_on_movement belongs in resources/rendering.config and is read by Renderer
	}

}

void Engine::LoadNewScene(const std::string& scene_name)
{
	const std::string folder_path = "./resources/scenes";
	const std::string scene_path = folder_path + "/" + scene_name + ".scene";
	if (!Utils::CheckFileExistence(scene_path))
	{
		std::cout << "error: scene " << scene_name << " is missing";
		return;
	}

	std::vector<char> file_buffer;
	rapidjson::Document document;
	JsonParser::ReadJsonFileInsitu(scene_path, document, file_buffer);

	if (!scenes.empty())
	{
		on_start_call_queue.clear();
		pending_on_start_call_queue.clear();
		actors_pending_destruction.clear();
	}

	std::shared_ptr<Scene> new_scene = std::make_shared<Scene>(scene_name);
	if (!scenes.empty())
	{
		Scene& previous_scene = *scenes.back();
		for (Actor& actor : *previous_scene.actors.GetActors())
		{
			if (!actor.pending_destroy && actor.dont_destroy_on_load)
			{
				Actor transferred_actor = actor;
				Actor* stored_actor = new_scene->actors.AddActor(transferred_actor);
				stored_actor->InitializeRuntimeContext(componentDB->GetLuaState().get(), componentDB.get());

				for (auto& [key, instance] : stored_actor->component_instances)
				{
					stored_actor->InjectConvenienceReferences(instance);
				}
			}
		}

		for (Actor& actor : *previous_scene.actors.GetActors())
		{
			if (actor.pending_destroy || actor.dont_destroy_on_load)
			{
				continue;
			}

			for (const auto& [key, instance_ptr] : actor.component_instances)
			{
				if (instance_ptr)
				{
					InvokeDestroyLifecycle(&actor, *instance_ptr);
				}
			}
		}
	}

	LoadScene(document, *new_scene);

	scenes.clear();
	scenes.emplace_back(new_scene);
	ScriptBindings::SetActiveActorDB(&new_scene->actors);
}

void Engine::QueueSceneLoad(const std::string& scene_name)
{
	pending_scene_load_name = scene_name;
}

std::string Engine::GetCurrentSceneName() const
{
	if (scenes.empty())
	{
		return "";
	}

	return scenes.back()->name;
}

void Engine::MarkActorDontDestroy(Actor* actor)
{
	if (actor == nullptr)
	{
		return;
	}

	actor->dont_destroy_on_load = true;
}

void Engine::LoadActorFromTemplate(const std::string& template_name, Actor& out_actor)
{
	auto it = actor_template_cache.find(template_name);
	if (it != actor_template_cache.end())
	{
		out_actor = it->second;
		return;
	}

	const std::string file_path = "./resources/actor_templates/" + template_name + ".template";

	rapidjson::Document document;
	std::vector<char> file_buffer;

	// PERFORMANCE FIX: We try to open. If it fails, we print the specific error and exit.
	// This saves the cost of calling std::filesystem::exists() before opening.
	if (!JsonParser::ReadJsonFileInsitu(file_path, document, file_buffer))
	{
		std::cout << "error: template " << template_name << " is missing";
		exit(0);
	}

	LoadActorFromJson(document, out_actor);
	actor_template_cache[template_name] = out_actor;
}

void Engine::LoadScene(const rapidjson::Document& document, Scene& scene_out)
{
	if (!(document.HasMember("actors") && document["actors"].IsArray()))
	{
		assert(false);
		return;
	}

	auto actors_json = document["actors"].GetArray();

	for (const rapidjson::Value& actor_json : actors_json)
	{
		Actor actor{};
#define query "template"
		if (actor_json.HasMember(query) && actor_json[query].IsString())
		{
			LoadActorFromTemplate(actor_json[query].GetString(), actor);
		}

		LoadActorFromJson(actor_json, actor);

		// Store actor first so pointer is stable
		Actor* stored_actor = scene_out.actors.AddActor(actor);
		stored_actor->InitializeRuntimeContext(componentDB->GetLuaState().get(), componentDB.get());

		for (const auto& [comp_key, comp_type] : stored_actor->components)
		{
			std::shared_ptr<Component> comp_template = componentDB->GetComponent(comp_type);
			if (!comp_template)
			{
				std::cout << "error: failed to locate component " << comp_type;
				exit(0);
			}

			auto instance_table = componentDB->CreateComponentInstance(comp_type, comp_key);
			if (!instance_table)
			{
				std::cout << "error: failed to create component instance " << comp_type;
				exit(0);
			}

			auto props_it = stored_actor->component_properties.find(comp_key);
			if (props_it != stored_actor->component_properties.end())
			{
				for (const auto& prop_kv : props_it->second)
				{
					const std::string& prop_name = prop_kv.first;
					const auto& prop_value = prop_kv.second;
					std::visit([&](const auto& v)
					{
						(*instance_table)[prop_name] = v;
					}, prop_value);
				}
			}

			// Inject self.actor, self.actor_name, self.id
			stored_actor->InjectConvenienceReferences(instance_table);

			stored_actor->component_instances.emplace(comp_key, instance_table);

			if (comp_template->hasStart)
			{
				on_start_call_queue.emplace_back(stored_actor, instance_table);
			}
		}
	}
}

Actor* Engine::InstantiateActor(const std::string& template_name)
{
	if (scenes.empty())
	{
		return nullptr;
	}

	Actor actor{};
	LoadActorFromTemplate(template_name, actor);

	Scene& scene = *scenes.back();
	Actor* stored_actor = scene.actors.AddActor(actor);
	stored_actor->InitializeRuntimeContext(componentDB->GetLuaState().get(), componentDB.get());
	stored_actor->spawned_frame = Helper::GetFrameNumber();

	for (const auto& [comp_key, comp_type] : stored_actor->components)
	{
		std::shared_ptr<Component> comp_template = componentDB->GetComponent(comp_type);
		if (!comp_template)
		{
			continue;
		}

		auto instance_table = componentDB->CreateComponentInstance(comp_type, comp_key);
		if (!instance_table)
		{
			continue;
		}
		stored_actor->InjectConvenienceReferences(instance_table);

		auto props_it = stored_actor->component_properties.find(comp_key);
		if (props_it != stored_actor->component_properties.end())
		{
			for (const auto& prop_kv : props_it->second)
			{
				const std::string& prop_name = prop_kv.first;
				const auto& prop_value = prop_kv.second;
				std::visit([&](const auto& v)
				{
					(*instance_table)[prop_name] = v;
				}, prop_value);
			}
		}

		stored_actor->component_instances.emplace(comp_key, instance_table);
		if (comp_template->hasStart)
		{
			if (is_in_update)
			{
				pending_on_start_call_queue.emplace_back(stored_actor, instance_table);
			}
			else
			{
				on_start_call_queue.emplace_back(stored_actor, instance_table);
			}
		}
	}

	return stored_actor;
}

void Engine::DestroyActor(Actor* actor)
{
	if (actor == nullptr || actor->pending_destroy)
	{
		return;
	}

	for (const auto& [component_key, instance_ptr] : actor->component_instances)
	{
		if (instance_ptr)
		{
			InvokeDestroyLifecycle(actor, *instance_ptr);
		}
	}

	actor->MarkPendingDestroy();

	if (!scenes.empty())
	{
		scenes.back()->actors.UnregisterActor(actor);
	}

	actors_pending_destruction.push_back(actor);
}


void Engine::LoadActorFromJson(const rapidjson::Value& actor_json, Actor& out_actor)
{
	for (auto itr = actor_json.MemberBegin(); itr != actor_json.MemberEnd(); ++itr)
	{
		const char* key = itr->name.GetString();

		switch (key[0])
		{
		case 'n':
			if (strcmp(key, "name") == 0 && itr->value.IsString())
			{
				out_actor.actor_name = itr->value.GetString();
			}
			break;

		case 'c':
			if (strcmp(key, "components") == 0 && itr->value.IsObject())
			{
				for (auto comp_itr = itr->value.MemberBegin(); comp_itr != itr->value.MemberEnd(); ++comp_itr)
				{
					const std::string comp_key = comp_itr->name.GetString();
					const rapidjson::Value& comp_obj = comp_itr->value;

					if (!comp_obj.IsObject())
					{
						continue;
					}

					// Existing entry (from template) is reused; new entry is created if missing.
					std::string& component_type = out_actor.components[comp_key];
					auto& props = out_actor.component_properties[comp_key];

					for (auto field_itr = comp_obj.MemberBegin(); field_itr != comp_obj.MemberEnd(); ++field_itr)
					{
						const std::string field_name = field_itr->name.GetString();
						const rapidjson::Value& field_value = field_itr->value;

						if (field_name == "type")
						{
							if (field_value.IsString())
							{
								component_type = field_value.GetString();
							}
							continue;
						}

						if (field_value.IsString())
						{
							props[field_name] = std::string(field_value.GetString());
						}
						else if (field_value.IsBool())
						{
							props[field_name] = field_value.GetBool();
						}
						else if (field_value.IsNumber())
						{
							if(field_value.IsInt())
							{
								props[field_name] = field_value.GetInt();
							}
							else
							{
								props[field_name] = field_value.GetFloat(); // Lua number
							}
						}
					}

					// New component must have type; inherited override may omit it.
					if (component_type.empty())
					{
						std::cout << "error: component [" << comp_key << "] is missing type";
						exit(0);
					}
				}
			}
			break;
		}
	}
}

void Engine::ReportLuaError(const std::string& actor_name, const luabridge::LuaException& e)
{
	std::string error_message = e.what();

	// Normalize separators across OS
	std::replace(error_message.begin(), error_message.end(), '\\', '/');

	// Autograder expects no leading "./"
	while (error_message.rfind("./", 0) == 0)
	{
		error_message.erase(0, 2);
	}

	std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}

void Engine::InvokeLifecycle(Actor* actor, luabridge::LuaRef& instance, const char* fn_name)
{
	if (actor != nullptr && actor->pending_destroy)
	{
		return;
	}

	luabridge::LuaRef enabled = instance["enabled"];
	if (enabled.isBool() && enabled.cast<bool>() == false)
	{
		return;
	}

	luabridge::LuaRef fn = instance[fn_name];
	if (!fn.isFunction())
	{
		return;
	}

	try
	{
		fn(instance); // self
	}
	catch (const luabridge::LuaException& e)
	{
		const std::string actor_name = (actor != nullptr) ? actor->actor_name : "unknown_actor";
		ReportLuaError(actor_name, e);
	}
}

void Engine::InvokeDestroyLifecycle(Actor* actor, luabridge::LuaRef& instance)
{
	luabridge::LuaRef fn = instance["OnDestroy"];
	if (!fn.isFunction())
	{
		return;
	}

	try
	{
		fn(instance); // self
	}
	catch (const luabridge::LuaException& e)
	{
		const std::string actor_name = (actor != nullptr) ? actor->actor_name : "unknown_actor";
		ReportLuaError(actor_name, e);
	}
}