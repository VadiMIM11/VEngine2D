#pragma once
#include <iostream>
#include <algorithm>
#include <utility>
#include "glm/glm.hpp"
#include "rapidjson/document.h"
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "InputManager.h"
#include "ComponentDB.h"
#include "ScriptBindings.h"
#include "TextDB.h"
#include "ImageDB.h"
#include "ActorDB.h"
#include "Renderer.h"
#include "AudioDB.h"
#include "Scene.h"
#include "Utils.h"
#include "JsonParser.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

class Engine
{
public:
	Engine();
	~Engine();
	void Run();

private:
	bool isRunning;

	std::shared_ptr<Renderer> renderer;
	std::shared_ptr<ComponentDB> componentDB;
	std::shared_ptr<TextDB> textDB;
	std::shared_ptr<ImageDB> imageDB;
	std::shared_ptr<AudioDB> audioDB;
	std::vector<std::shared_ptr<Scene>> scenes;


	std::string game_title;
	std::string initial_scene_name;
	std::unordered_map<std::string, Actor, std::hash<std::string>> actor_template_cache;
	std::vector<std::pair<Actor*, std::shared_ptr<luabridge::LuaRef>>> on_start_call_queue;
	std::vector<std::pair<Actor*, std::shared_ptr<luabridge::LuaRef>>> pending_on_start_call_queue;
	std::vector<Actor*> actors_pending_destruction;
	bool is_in_update = false;
	std::string pending_scene_load_name;

	/* Constants */
	std::string resources_folder = "./resources/";
	std::string scenes_folder = resources_folder + "scenes/";
	std::string images_folder = resources_folder + "images/";
	std::string fonts_folder = resources_folder + "fonts/";
	std::string audio_folder = resources_folder + "audio/";
	std::string game_config_path = resources_folder + "game.config";
	std::string components_folder = resources_folder + "component_types/";


	void Input();
	void Update();
	void Render();

	void Tick();
#ifdef __EMSCRIPTEN__
	static void MainLoopThunk(void* arg);
#endif


	void LoadComponents();
	void LoadGameConfig();
	void LoadNewScene(const std::string& scene_name);
	void QueueSceneLoad(const std::string& scene_name);
	std::string GetCurrentSceneName() const;
	void MarkActorDontDestroy(Actor* actor);
	void LoadActorFromTemplate(const std::string& template_name, Actor& out_actor);
	void LoadScene(const rapidjson::Document& document, Scene& scene_out);
	void LoadActorFromJson(const rapidjson::Value& actor_json, Actor& actor_out);
	Actor* InstantiateActor(const std::string& template_name);
	void DestroyActor(Actor* actor);
	void ReportLuaError(const std::string& actor_name, const luabridge::LuaException& e);
	void InvokeLifecycle(Actor* actor, luabridge::LuaRef& instance, const char* fn_name);
	void InvokeDestroyLifecycle(Actor* actor, luabridge::LuaRef& instance);
};

