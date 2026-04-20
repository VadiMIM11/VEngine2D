#pragma once
#include <memory>
#include <functional>
#include <string>
#include "lua/lua.hpp"

class Renderer;
class TextDB;
class ImageDB;
class AudioDB;
class ActorDB;
class Actor;

class ScriptBindings
{
public:
	static void RegisterCore(lua_State* L);
	static void RegisterRendering(lua_State* L, Renderer* renderer, TextDB* textDB, ImageDB* imageDB);
	static void RegisterAudio(lua_State* L, AudioDB* audioDB);
	static void FlushRenderingQueues();
	static void ApplyPendingEventSubscriptions();
	static void Shutdown();
	static void QueueImageDrawEx(const std::string& image_name, float x, float y, float rotation_degrees,
		float scale_x, float scale_y, float pivot_x, float pivot_y,
		float r, float g, float b, float a, float sorting_order);

	static void SetActiveActorDB(ActorDB* actor_db);
	static void SetRuntimeActorOps(const std::function<Actor*(const std::string&)>& instantiate_actor,
		const std::function<void(Actor*)>& destroy_actor);
	static void SetSceneOps(const std::function<void(const std::string&)>& load_scene,
		const std::function<std::string()>& get_current_scene,
		const std::function<void(Actor*)>& dont_destroy_actor);
};