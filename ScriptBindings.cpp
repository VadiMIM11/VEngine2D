#include "ScriptBindings.h"
#include "LuaBridge/LuaBridge.h"
#include <iostream>
#include <thread>
#include <queue>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include "Actor.h"
#include "ActorDB.h"
#include "Helper.h"
#include "InputManager.h"
#include "Utils.h"
#include "glm/glm.hpp"
#include "box2d/box2d.h"
#include "Renderer.h"
#include "TextDB.h"
#include "ImageDB.h"
#include "AudioDB.h"
#include "Rigidbody.h"
#include "ParticleSystem.h"

#include <algorithm>
#include <unordered_map>

namespace
{
	lua_State* g_lua_state = nullptr;
	ActorDB* g_actor_db = nullptr;
	std::function<Actor*(const std::string&)> g_instantiate_actor;
	std::function<void(Actor*)> g_destroy_actor;
	std::function<void(const std::string&)> g_load_scene;
	std::function<std::string()> g_get_current_scene;
	std::function<void(Actor*)> g_dont_destroy_actor;
	Renderer* g_renderer = nullptr;
	TextDB* g_text_db = nullptr;
	ImageDB* g_image_db = nullptr;
	AudioDB* g_audio_db = nullptr;
	float g_camera_x = 0.0f;
	float g_camera_y = 0.0f;
	float g_camera_zoom = 1.0f;

	struct TextDrawRequest
	{
		std::string content;
		int x = 0;
		int y = 0;
		std::string font_name;
		int font_size = 16;
		Uint8 r = 255;
		Uint8 g = 255;
		Uint8 b = 255;
		Uint8 a = 255;
	};

	std::queue<TextDrawRequest> g_text_draw_queue;

	struct ImageDrawRequest
	{
		std::string image_name;
		float x = 0.0f;
		float y = 0.0f;
		int rotation_degrees = 0;
		float scale_x = 1.0f;
		float scale_y = 1.0f;
		float pivot_x = 0.5f;
		float pivot_y = 0.5f;
		int r = 255;
		int g = 255;
		int b = 255;
		int a = 255;
		int sorting_order = 0;
		std::size_t call_index = 0;
	};

	struct PixelDrawRequest
	{
		int x = 0;
		int y = 0;
		Uint8 r = 255;
		Uint8 g = 255;
		Uint8 b = 255;
		Uint8 a = 255;
	};

	std::vector<ImageDrawRequest> g_scene_image_draw_queue;
	std::vector<ImageDrawRequest> g_ui_image_draw_queue;
	std::vector<PixelDrawRequest> g_pixel_draw_queue;
	std::size_t g_image_draw_call_counter = 0;
	constexpr uint16 PHANTOM_CATEGORY = 0x0004;

	int ToIntNoRounding(float value)
	{
		return static_cast<int>(value);
	}

	struct RaycastHit
	{
		Actor* actor = nullptr;
		b2Vec2 point = b2Vec2_zero;
		b2Vec2 normal = b2Vec2_zero;
		bool is_trigger = false;
		float fraction = 0.0f;
	};

	class RaycastCollectCallback final : public b2RayCastCallback
	{
	public:
		std::vector<RaycastHit> hits;

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
		{
			if (fixture == nullptr)
			{
				return 1.0f;
			}

			if ((fixture->GetFilterData().categoryBits & PHANTOM_CATEGORY) != 0)
			{
				return -1.0f;
			}

			Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
			if (actor == nullptr)
			{
				return -1.0f;
			}

			RaycastHit hit;
			hit.actor = actor;
			hit.point = point;
			hit.normal = normal;
			hit.is_trigger = fixture->IsSensor();
			hit.fraction = fraction;
			hits.push_back(hit);
			return 1.0f;
		}
	};

	luabridge::LuaRef MakeHitResult(lua_State* L, const RaycastHit& hit)
	{
		luabridge::LuaRef result = luabridge::newTable(L);
		result["actor"] = hit.actor;
		result["point"] = hit.point;
		result["normal"] = hit.normal;
		result["is_trigger"] = hit.is_trigger;
		return result;
	}

	luabridge::LuaRef PhysicsRaycast(const b2Vec2& pos, b2Vec2 dir, float dist)
	{
		if (dist <= 0.0f || dir.LengthSquared() <= 0.0f)
		{
			return luabridge::LuaRef(g_lua_state);
		}

		b2World* world = Rigidbody::GetPhysicsWorld();
		if (world == nullptr)
		{
			return luabridge::LuaRef(g_lua_state);
		}

		dir.Normalize();
		const b2Vec2 end = pos + dist * dir;

		RaycastCollectCallback callback;
		world->RayCast(&callback, pos, end);
		if (callback.hits.empty())
		{
			return luabridge::LuaRef(g_lua_state);
		}

		auto closest = std::min_element(callback.hits.begin(), callback.hits.end(),
			[](const RaycastHit& lhs, const RaycastHit& rhs)
			{
				return lhs.fraction < rhs.fraction;
			});

		return MakeHitResult(g_lua_state, *closest);
	}

	luabridge::LuaRef PhysicsRaycastAll(const b2Vec2& pos, b2Vec2 dir, float dist)
	{
		luabridge::LuaRef result = luabridge::newTable(g_lua_state);
		if (dist <= 0.0f || dir.LengthSquared() <= 0.0f)
		{
			return result;
		}

		b2World* world = Rigidbody::GetPhysicsWorld();
		if (world == nullptr)
		{
			return result;
		}

		dir.Normalize();
		const b2Vec2 end = pos + dist * dir;

		RaycastCollectCallback callback;
		world->RayCast(&callback, pos, end);

		std::sort(callback.hits.begin(), callback.hits.end(),
			[](const RaycastHit& lhs, const RaycastHit& rhs)
			{
				return lhs.fraction < rhs.fraction;
			});

		int index = 1;
		for (const RaycastHit& hit : callback.hits)
		{
			result[index++] = MakeHitResult(g_lua_state, hit);
		}

		return result;
	}

	struct EventSubscription
	{
		std::shared_ptr<luabridge::LuaRef> component;
		std::shared_ptr<luabridge::LuaRef> fn;
	};

	struct PendingEventOp
	{
		std::string event_type;
		std::shared_ptr<luabridge::LuaRef> component;
		std::shared_ptr<luabridge::LuaRef> fn;
	};

	std::unordered_map<std::string, std::vector<EventSubscription>> g_event_subscriptions;
	std::vector<PendingEventOp> g_pending_subscribes;
	std::vector<PendingEventOp> g_pending_unsubscribes;

	bool LuaRefsEqual(const luabridge::LuaRef& a, const luabridge::LuaRef& b)
	{
		return a.rawequal(b);
	}

	void EventPublish(const std::string& event_type, luabridge::LuaRef event_object)
	{
		auto it = g_event_subscriptions.find(event_type);
		if (it == g_event_subscriptions.end())
		{
			return;
		}

		for (const EventSubscription& sub : it->second)
		{
			if (!sub.component || !sub.fn)
			{
				continue;
			}

			try
			{
				(*(sub.fn))(*(sub.component), event_object);
			}
			catch (const luabridge::LuaException&)
			{
			}
		}
	}

	void EventSubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef fn)
	{
		if (!fn.isFunction())
		{
			return;
		}

		g_pending_subscribes.push_back(PendingEventOp{
			event_type,
			std::make_shared<luabridge::LuaRef>(component),
			std::make_shared<luabridge::LuaRef>(fn)
			});
	}

	void EventUnsubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef fn)
	{
		if (!fn.isFunction())
		{
			return;
		}

		g_pending_unsubscribes.push_back(PendingEventOp{
			event_type,
			std::make_shared<luabridge::LuaRef>(component),
			std::make_shared<luabridge::LuaRef>(fn)
			});
	}

	void ApplyPendingEventSubscriptionsInternal()
	{
		for (const PendingEventOp& op : g_pending_subscribes)
		{
			g_event_subscriptions[op.event_type].push_back(EventSubscription{ op.component, op.fn });
		}
		g_pending_subscribes.clear();

		for (const PendingEventOp& op : g_pending_unsubscribes)
		{
			auto map_it = g_event_subscriptions.find(op.event_type);
			if (map_it == g_event_subscriptions.end())
			{
				continue;
			}

			auto& subs = map_it->second;
			subs.erase(
				std::remove_if(subs.begin(), subs.end(),
					[&](const EventSubscription& sub)
					{
						if (!sub.component || !sub.fn || !op.component || !op.fn)
						{
							return false;
						}
						return LuaRefsEqual(*sub.component, *op.component) && LuaRefsEqual(*sub.fn, *op.fn);
					}),
				subs.end());
		}
		g_pending_unsubscribes.clear();
	}

	void DebugLog(const std::string& message)
	{
		std::cout << message << std::endl;
	}

	Actor* ActorFind(const std::string& name)
	{
		if (!g_actor_db) return nullptr;
		return g_actor_db->GetActorByName(name);
	}

	luabridge::LuaRef ActorFindAll(const std::string& name)
	{
		luabridge::LuaRef result = luabridge::newTable(g_lua_state);
		if (!g_actor_db) return result;

		const std::vector<Actor*>* found = g_actor_db->GetActorsByName(name);
		if (!found) return result;

		int i = 1;
		for (Actor* actor : *found)
		{
			result[i++] = actor;
		}
		return result;
	}

	Actor* ActorInstantiate(const std::string& actor_template_name)
	{
		if (!g_instantiate_actor)
		{
			return nullptr;
		}
		return g_instantiate_actor(actor_template_name);
	}

	void ActorDestroy(Actor* actor)
	{
		if (!g_destroy_actor)
		{
			return;
		}
		g_destroy_actor(actor);
	}

	void SceneLoad(const std::string& scene_name)
	{
		if (g_load_scene)
		{
			g_load_scene(scene_name);
		}
	}

	std::string SceneGetCurrent()
	{
		if (g_get_current_scene)
		{
			return g_get_current_scene();
		}
		return "";
	}

	void SceneDontDestroy(Actor* actor)
	{
		if (g_dont_destroy_actor)
		{
			g_dont_destroy_actor(actor);
		}
	}

	void ApplicationQuit()
	{
		exit(0);
	}

	void ApplicationSleep(int milliseconds)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}

	int ApplicationGetFrame()
	{
		return Helper::GetFrameNumber();
	}

	void ApplicationOpenURL(const std::string& url)
	{
		std::string command;
#if defined(_WIN32)
		command = "start \"\" \"" + url + "\"";
#elif defined(__APPLE__)
		command = "open \"" + url + "\"";
#else
		command = "xdg-open \"" + url + "\"";
#endif
		std::system(command.c_str());
	}

	void CameraSetPosition(float x, float y)
	{
		g_camera_x = x;
		g_camera_y = y;
	}

	float CameraGetPositionX()
	{
		return g_camera_x;
	}

	float CameraGetPositionY()
	{
		return g_camera_y;
	}

	void CameraSetZoom(float zoom_factor)
	{
		if (zoom_factor <= 0.0f)
		{
			return;
		}
		g_camera_zoom = zoom_factor;
		if (g_renderer != nullptr)
		{
			g_renderer->SetZoom(zoom_factor);
		}
	}

	float CameraGetZoom()
	{
		return g_camera_zoom;
	}

	SDL_Scancode ResolveScancodeFromKeycode(const std::string& keycode)
	{
		const auto& map = Utils::GetKeycodeToScancodeMap();
		auto it = map.find(Utils::ToLowerAscii(keycode));
		if (it == map.end())
		{
			return SDL_SCANCODE_UNKNOWN;
		}
		return it->second;
	}

	bool InputGetKey(const std::string& keycode)
	{
		SDL_Scancode scancode = ResolveScancodeFromKeycode(keycode);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			return false;
		}
		return InputManager::GetKey(scancode);
	}

	bool InputGetKeyDown(const std::string& keycode)
	{
		SDL_Scancode scancode = ResolveScancodeFromKeycode(keycode);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			return false;
		}
		return InputManager::GetKeyDown(scancode);
	}

	bool InputGetKeyUp(const std::string& keycode)
	{
		SDL_Scancode scancode = ResolveScancodeFromKeycode(keycode);
		if (scancode == SDL_SCANCODE_UNKNOWN)
		{
			return false;
		}
		return InputManager::GetKeyUp(scancode);
	}

	glm::vec2 InputGetMousePosition()
	{
		return glm::vec2(static_cast<float>(InputManager::GetMouseX()), static_cast<float>(InputManager::GetMouseY()));
	}

	MOUSE_BUTTON ResolveMouseButton(int button_num)
	{
		switch (button_num)
		{
		case 1: return MOUSE_BUTTON::LEFT;
		case 2: return MOUSE_BUTTON::MIDDLE;
		case 3: return MOUSE_BUTTON::RIGHT;
		default: return static_cast<MOUSE_BUTTON>(0);
		}
	}

	bool InputGetMouseButton(int button_num)
	{
		MOUSE_BUTTON button = ResolveMouseButton(button_num);
		if (button_num < 1 || button_num > 3)
		{
			return false;
		}
		return InputManager::GetMouseButton(button);
	}

	bool InputGetMouseButtonDown(int button_num)
	{
		MOUSE_BUTTON button = ResolveMouseButton(button_num);
		if (button_num < 1 || button_num > 3)
		{
			return false;
		}
		return InputManager::GetMouseButtonDown(button);
	}

	bool InputGetMouseButtonUp(int button_num)
	{
		MOUSE_BUTTON button = ResolveMouseButton(button_num);
		if (button_num < 1 || button_num > 3)
		{
			return false;
		}
		return InputManager::GetMouseButtonUp(button);
	}

	float InputGetMouseScrollDelta()
	{
		return InputManager::GetMouseWheelY();
	}

	void InputHideCursor()
	{
		SDL_ShowCursor(SDL_DISABLE);
	}

	void InputShowCursor()
	{
		SDL_ShowCursor(SDL_ENABLE);
	}

	void TextDraw(const std::string& str_content, float x, float y, const std::string& font_name,
		float font_size, float r, float g, float b, float a)
	{
		TextDrawRequest request;
		request.content = str_content;
		request.x = static_cast<int>(x);
		request.y = static_cast<int>(y);
		request.font_name = font_name;
		request.font_size = static_cast<int>(font_size);
		request.r = static_cast<Uint8>(static_cast<int>(r));
		request.g = static_cast<Uint8>(static_cast<int>(g));
		request.b = static_cast<Uint8>(static_cast<int>(b));
		request.a = static_cast<Uint8>(static_cast<int>(a));
		g_text_draw_queue.push(request);
	}

	void ImageDrawUI(const std::string& image_name, float x, float y)
	{
		ImageDrawRequest request;
		request.image_name = image_name;
		request.x = static_cast<float>(ToIntNoRounding(x));
		request.y = static_cast<float>(ToIntNoRounding(y));
		request.call_index = g_image_draw_call_counter++;
		g_ui_image_draw_queue.push_back(request);
	}

	void ImageDrawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a, float sorting_order)
	{
		ImageDrawRequest request;
		request.image_name = image_name;
		request.x = static_cast<float>(ToIntNoRounding(x));
		request.y = static_cast<float>(ToIntNoRounding(y));
		request.r = ToIntNoRounding(r);
		request.g = ToIntNoRounding(g);
		request.b = ToIntNoRounding(b);
		request.a = ToIntNoRounding(a);
		request.sorting_order = ToIntNoRounding(sorting_order);
		request.call_index = g_image_draw_call_counter++;
		g_ui_image_draw_queue.push_back(request);
	}

	void ImageDraw(const std::string& image_name, float x, float y)
	{
		ImageDrawRequest request;
		request.image_name = image_name;
		request.x = x;
		request.y = y;
		request.call_index = g_image_draw_call_counter++;
		g_scene_image_draw_queue.push_back(request);
	}

	void ImageDrawEx(const std::string& image_name, float x, float y, float rotation_degrees,
		float scale_x, float scale_y, float pivot_x, float pivot_y,
		float r, float g, float b, float a, float sorting_order)
	{
		ImageDrawRequest request;
		request.image_name = image_name;
		request.x = x;
		request.y = y;
		request.rotation_degrees = ToIntNoRounding(rotation_degrees);
		request.scale_x = scale_x;
		request.scale_y = scale_y;
		request.pivot_x = pivot_x;
		request.pivot_y = pivot_y;
		request.r = ToIntNoRounding(r);
		request.g = ToIntNoRounding(g);
		request.b = ToIntNoRounding(b);
		request.a = ToIntNoRounding(a);
		request.sorting_order = ToIntNoRounding(sorting_order);
		request.call_index = g_image_draw_call_counter++;
		g_scene_image_draw_queue.push_back(request);
	}

	void ImageDrawPixel(float x, float y, float r, float g, float b, float a)
	{
		PixelDrawRequest request;
		request.x = ToIntNoRounding(x);
		request.y = ToIntNoRounding(y);
		request.r = static_cast<Uint8>(ToIntNoRounding(r));
		request.g = static_cast<Uint8>(ToIntNoRounding(g));
		request.b = static_cast<Uint8>(ToIntNoRounding(b));
		request.a = static_cast<Uint8>(ToIntNoRounding(a));
		g_pixel_draw_queue.push_back(request);
	}

	std::string ResolveClipPath(const std::string& clip_name)
	{
		if (clip_name.find('.') != std::string::npos)
		{
			return "./resources/audio/" + clip_name;
		}

		const std::string wav_path = "./resources/audio/" + clip_name + ".wav";
		if (Utils::CheckFileExistence(wav_path))
		{
			return wav_path;
		}

		const std::string ogg_path = "./resources/audio/" + clip_name + ".ogg";
		if (Utils::CheckFileExistence(ogg_path))
		{
			return ogg_path;
		}

		return wav_path;
	}

	void AudioPlay(int channel, const std::string& clip_name, bool does_loop)
	{
		if (g_audio_db == nullptr)
		{
			return;
		}

		Mix_Chunk* chunk = g_audio_db->LoadChunk(ResolveClipPath(clip_name));
		g_audio_db->Play(channel, chunk, does_loop ? -1 : 0);
	}

	void AudioHalt(int channel)
	{
		if (g_audio_db == nullptr)
		{
			return;
		}

		g_audio_db->Halt(channel);
	}

	void AudioSetVolume(int channel, float volume)
	{
		if (g_audio_db == nullptr)
		{
			return;
		}

		int volume_int = static_cast<int>(volume);
		if (volume_int < 0)
		{
			volume_int = 0;
		}
		if (volume_int > 128)
		{
			volume_int = 128;
		}
		g_audio_db->SetVolume(channel, volume_int);
	}

}

void ScriptBindings::SetActiveActorDB(ActorDB* actor_db)
{
	g_actor_db = actor_db;
}

void ScriptBindings::SetRuntimeActorOps(const std::function<Actor*(const std::string&)>& instantiate_actor,
	const std::function<void(Actor*)>& destroy_actor)
{
	g_instantiate_actor = instantiate_actor;
	g_destroy_actor = destroy_actor;
}

void ScriptBindings::SetSceneOps(const std::function<void(const std::string&)>& load_scene,
	const std::function<std::string()>& get_current_scene,
	const std::function<void(Actor*)>& dont_destroy_actor)
{
	g_load_scene = load_scene;
	g_get_current_scene = get_current_scene;
	g_dont_destroy_actor = dont_destroy_actor;
}

void ScriptBindings::RegisterCore(lua_State* L)
{
	g_lua_state = L;

	luabridge::getGlobalNamespace(L)
		.beginClass<glm::vec2>("vec2")
			.addConstructor<void(*)()>()
			.addProperty("x", &glm::vec2::x)
			.addProperty("y", &glm::vec2::y)
		.endClass()
		.beginClass<b2Vec2>("Vector2")
			.addConstructor<void(*)(float, float)>()
			.addData("x", &b2Vec2::x)
			.addData("y", &b2Vec2::y)
			.addFunction("Normalize", &b2Vec2::Normalize)
			.addFunction("Length", &b2Vec2::Length)
			.addFunction("__add", &b2Vec2::operator_add)
			.addFunction("__sub", &b2Vec2::operator_sub)
			.addFunction("__mul", &b2Vec2::operator_mul)
			.addStaticFunction("Distance", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Distance))
			.addStaticFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
		.endClass()
		.beginClass<Rigidbody>("Rigidbody")
			.addConstructor<void(*)()>()
			.addData("key", &Rigidbody::key)
			.addData("type", &Rigidbody::type)
			.addData("enabled", &Rigidbody::enabled)
			.addData("actor", &Rigidbody::actor)
			.addData("actor_name", &Rigidbody::actor_name)
			.addData("id", &Rigidbody::id)
			.addData("x", &Rigidbody::x)
			.addData("y", &Rigidbody::y)
			.addData("width", &Rigidbody::width)
			.addData("height", &Rigidbody::height)
			.addData("radius", &Rigidbody::radius)
			.addData("body_type", &Rigidbody::body_type)
			.addData("precise", &Rigidbody::precise)
			.addData("gravity_scale", &Rigidbody::gravity_scale)
			.addData("density", &Rigidbody::density)
			.addData("angular_friction", &Rigidbody::angular_friction)
			.addData("rotation", &Rigidbody::rotation)
			.addData("has_collider", &Rigidbody::has_collider)
			.addData("collider_type", &Rigidbody::collider_type)
			.addData("friction", &Rigidbody::friction)
			.addData("bounciness", &Rigidbody::bounciness)
			.addData("has_trigger", &Rigidbody::has_trigger)
			.addData("trigger_type", &Rigidbody::trigger_type)
			.addData("trigger_width", &Rigidbody::trigger_width)
			.addData("trigger_height", &Rigidbody::trigger_height)
			.addData("trigger_radius", &Rigidbody::trigger_radius)
			.addFunction("OnStart", &Rigidbody::OnStart)
			.addFunction("OnUpdate", &Rigidbody::OnUpdate)
			.addFunction("OnLateUpdate", &Rigidbody::OnLateUpdate)
			.addFunction("OnDestroy", &Rigidbody::OnDestroy)
			.addFunction("AddForce", &Rigidbody::AddForce)
			.addFunction("SetVelocity", &Rigidbody::SetVelocity)
			.addFunction("SetPosition", &Rigidbody::SetPosition)
			.addFunction("SetRotation", &Rigidbody::SetRotation)
			.addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
			.addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
			.addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
			.addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
			.addFunction("GetPosition", &Rigidbody::GetPosition)
			.addFunction("GetRotation", &Rigidbody::GetRotation)
			.addFunction("GetVelocity", &Rigidbody::GetVelocity)
			.addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
			.addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
			.addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
			.addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
		.endClass()
		.beginClass<ParticleSystem>("ParticleSystem")
			.addConstructor<void(*)()>()
			.addData("key", &ParticleSystem::key)
			.addData("type", &ParticleSystem::type)
			.addData("enabled", &ParticleSystem::enabled)
			.addData("actor", &ParticleSystem::actor)
			.addData("actor_name", &ParticleSystem::actor_name)
			.addData("id", &ParticleSystem::id)
			.addData("x", &ParticleSystem::x)
			.addData("y", &ParticleSystem::y)
			.addData("frames_between_bursts", &ParticleSystem::frames_between_bursts)
			.addData("burst_quantity", &ParticleSystem::burst_quantity)
			.addData("duration_frames", &ParticleSystem::duration_frames)
			.addData("start_scale_min", &ParticleSystem::start_scale_min)
			.addData("start_scale_max", &ParticleSystem::start_scale_max)
			.addData("rotation_min", &ParticleSystem::rotation_min)
			.addData("rotation_max", &ParticleSystem::rotation_max)
			.addData("end_scale", &ParticleSystem::end_scale)
			.addData("start_color_r", &ParticleSystem::start_color_r)
			.addData("start_color_g", &ParticleSystem::start_color_g)
			.addData("start_color_b", &ParticleSystem::start_color_b)
			.addData("start_color_a", &ParticleSystem::start_color_a)
			.addData("end_color_r", &ParticleSystem::end_color_r)
			.addData("end_color_g", &ParticleSystem::end_color_g)
			.addData("end_color_b", &ParticleSystem::end_color_b)
			.addData("end_color_a", &ParticleSystem::end_color_a)
			.addData("emit_angle_min", &ParticleSystem::emit_angle_min)
			.addData("emit_angle_max", &ParticleSystem::emit_angle_max)
			.addData("emit_radius_min", &ParticleSystem::emit_radius_min)
			.addData("emit_radius_max", &ParticleSystem::emit_radius_max)
			.addData("start_speed_min", &ParticleSystem::start_speed_min)
			.addData("start_speed_max", &ParticleSystem::start_speed_max)
			.addData("rotation_speed_min", &ParticleSystem::rotation_speed_min)
			.addData("rotation_speed_max", &ParticleSystem::rotation_speed_max)
			.addData("gravity_scale_x", &ParticleSystem::gravity_scale_x)
			.addData("gravity_scale_y", &ParticleSystem::gravity_scale_y)
			.addData("drag_factor", &ParticleSystem::drag_factor)
			.addData("angular_drag_factor", &ParticleSystem::angular_drag_factor)
			.addData("image", &ParticleSystem::image)
			.addData("sorting_order", &ParticleSystem::sorting_order)
			.addFunction("OnStart", &ParticleSystem::OnStart)
			.addFunction("OnUpdate", &ParticleSystem::OnUpdate)
			.addFunction("Stop", &ParticleSystem::Stop)
			.addFunction("Play", &ParticleSystem::Play)
			.addFunction("Burst", &ParticleSystem::Burst)
		.endClass()
		.beginNamespace("Debug")
			.addFunction("Log", &DebugLog)
		.endNamespace()
		.beginNamespace("Input")
			.addFunction("GetKey", &InputGetKey)
			.addFunction("GetKeyDown", &InputGetKeyDown)
			.addFunction("GetKeyUp", &InputGetKeyUp)
			.addFunction("GetMousePosition", &InputGetMousePosition)
			.addFunction("GetMouseButton", &InputGetMouseButton)
			.addFunction("GetMouseButtonDown", &InputGetMouseButtonDown)
			.addFunction("GetMouseButtonUp", &InputGetMouseButtonUp)
			.addFunction("GetMouseScrollDelta", &InputGetMouseScrollDelta)
			.addFunction("HideCursor", &InputHideCursor)
			.addFunction("ShowCursor", &InputShowCursor)
		.endNamespace()
		.beginNamespace("Application")
			.addFunction("Quit", &ApplicationQuit)
			.addFunction("Sleep", &ApplicationSleep)
			.addFunction("GetFrame", &ApplicationGetFrame)
			.addFunction("OpenURL", &ApplicationOpenURL)
		.endNamespace()
		.beginNamespace("Camera")
			.addFunction("SetPosition", &CameraSetPosition)
			.addFunction("GetPositionX", &CameraGetPositionX)
			.addFunction("GetPositionY", &CameraGetPositionY)
			.addFunction("SetZoom", &CameraSetZoom)
			.addFunction("GetZoom", &CameraGetZoom)
		.endNamespace()
		.beginNamespace("Physics")
			.addFunction("Raycast", &PhysicsRaycast)
			.addFunction("RaycastAll", &PhysicsRaycastAll)
		.endNamespace()
		.beginNamespace("Event")
			.addFunction("Publish", &EventPublish)
			.addFunction("Subscribe", &EventSubscribe)
			.addFunction("Unsubscribe", &EventUnsubscribe)
		.endNamespace()
		.beginNamespace("Scene")
			.addFunction("Load", &SceneLoad)
			.addFunction("GetCurrent", &SceneGetCurrent)
			.addFunction("DontDestroy", &SceneDontDestroy)
		.endNamespace()
		.beginNamespace("Text")
			.addFunction("Draw", &TextDraw)
		.endNamespace()
		.beginClass<Actor>("Actor")
			.addFunction("GetName", &Actor::GetName)
			.addFunction("GetID", &Actor::GetID)
			.addFunction("GetComponentByKey", &Actor::GetComponentByKey)
			.addFunction("GetComponent", &Actor::GetComponent)
			.addFunction("GetComponents", &Actor::GetComponents)
			.addFunction("AddComponent", &Actor::AddComponent)
			.addFunction("RemoveComponent", &Actor::RemoveComponent)
		.endClass()
		.beginNamespace("Actor")
			.addFunction("Find", &ActorFind)
			.addFunction("FindAll", &ActorFindAll)
			.addFunction("Instantiate", &ActorInstantiate)
			.addFunction("Destroy", &ActorDestroy)
		.endNamespace();
}

void ScriptBindings::RegisterRendering(lua_State* L, Renderer* renderer, TextDB* textDB, ImageDB* imageDB)
{
	g_renderer = renderer;
	g_text_db = textDB;
	g_image_db = imageDB;
	if (g_renderer != nullptr)
	{
		g_renderer->SetZoom(g_camera_zoom);
	}

	luabridge::getGlobalNamespace(L)
		.beginNamespace("Image")
			.addFunction("DrawUI", &ImageDrawUI)
			.addFunction("DrawUIEx", &ImageDrawUIEx)
			.addFunction("Draw", &ImageDraw)
			.addFunction("DrawEx", &ImageDrawEx)
			.addFunction("DrawPixel", &ImageDrawPixel)
		.endNamespace();
}

void ScriptBindings::RegisterAudio(lua_State* L, AudioDB* audioDB)
{
	g_audio_db = audioDB;

	luabridge::getGlobalNamespace(L)
		.beginNamespace("Audio")
			.addFunction("Play", &AudioPlay)
			.addFunction("Halt", &AudioHalt)
			.addFunction("SetVolume", &AudioSetVolume)
		.endNamespace();
}

void ScriptBindings::ApplyPendingEventSubscriptions()
{
	ApplyPendingEventSubscriptionsInternal();
}

void ScriptBindings::Shutdown()
{
	while (!g_text_draw_queue.empty())
	{
		g_text_draw_queue.pop();
	}
	g_scene_image_draw_queue.clear();
	g_ui_image_draw_queue.clear();
	g_pixel_draw_queue.clear();
	g_image_draw_call_counter = 0;

	g_pending_subscribes.clear();
	g_pending_unsubscribes.clear();
	g_event_subscriptions.clear();

	g_actor_db = nullptr;
	g_instantiate_actor = nullptr;
	g_destroy_actor = nullptr;
	g_load_scene = nullptr;
	g_get_current_scene = nullptr;
	g_dont_destroy_actor = nullptr;
	g_renderer = nullptr;
	g_text_db = nullptr;
	g_image_db = nullptr;
	g_audio_db = nullptr;
	g_lua_state = nullptr;
}

void ScriptBindings::QueueImageDrawEx(const std::string& image_name, float x, float y, float rotation_degrees,
	float scale_x, float scale_y, float pivot_x, float pivot_y,
	float r, float g, float b, float a, float sorting_order)
{
	ImageDrawEx(image_name, x, y, rotation_degrees, scale_x, scale_y, pivot_x, pivot_y,
		r, g, b, a, sorting_order);
}

void ScriptBindings::FlushRenderingQueues()
{
	if (g_renderer == nullptr || g_text_db == nullptr || g_image_db == nullptr)
	{
		g_scene_image_draw_queue.clear();
		g_ui_image_draw_queue.clear();
		g_pixel_draw_queue.clear();
		while (!g_text_draw_queue.empty())
		{
			g_text_draw_queue.pop();
		}
		return;
	}

	SDL_Renderer* sdl_renderer = g_renderer->GetRenderer();
	std::stable_sort(g_scene_image_draw_queue.begin(), g_scene_image_draw_queue.end(),
		[](const ImageDrawRequest& lhs, const ImageDrawRequest& rhs)
		{
			return lhs.sorting_order < rhs.sorting_order;
		});

	std::stable_sort(g_ui_image_draw_queue.begin(), g_ui_image_draw_queue.end(),
		[](const ImageDrawRequest& lhs, const ImageDrawRequest& rhs)
		{
			return lhs.sorting_order < rhs.sorting_order;
		});

	g_renderer->BeginWorldRender();
	SDL_RenderSetScale(sdl_renderer, g_camera_zoom, g_camera_zoom);

	for (const ImageDrawRequest& request : g_scene_image_draw_queue)
	{
		std::shared_ptr<Image> image = g_image_db->LoadImageFromDisk("./resources/images/", request.image_name);
		if (!image || image->texture == nullptr)
		{
			continue;
		}

		const int pixels_per_meter = 100;
		glm::vec2 final_rendering_position = glm::vec2(request.x, request.y) - glm::vec2(g_camera_x, g_camera_y);

		SDL_FRect tex_rect{};
		Helper::SDL_QueryTexture(image->texture, &tex_rect.w, &tex_rect.h);

		int flip_mode = SDL_FLIP_NONE;
		if (request.scale_x < 0.0f) flip_mode |= SDL_FLIP_HORIZONTAL;
		if (request.scale_y < 0.0f) flip_mode |= SDL_FLIP_VERTICAL;

		float x_scale = glm::abs(request.scale_x);
		float y_scale = glm::abs(request.scale_y);

		tex_rect.w = tex_rect.w * x_scale;
		tex_rect.h = tex_rect.h * y_scale;

		SDL_FPoint pivot_point = {
			static_cast<float>(request.pivot_x * static_cast<float>(tex_rect.w)),
			static_cast<float>(request.pivot_y * static_cast<float>(tex_rect.h))
		};

		glm::ivec2 cam_dimensions = glm::ivec2(g_renderer->GetWidth(), g_renderer->GetHeight());

		tex_rect.x =
			final_rendering_position.x * static_cast<float>(pixels_per_meter) +
			static_cast<float>(cam_dimensions.x) * 0.5f * (1.0f / g_camera_zoom) -
			static_cast<float>(pivot_point.x);

		tex_rect.y =
			final_rendering_position.y * static_cast<float>(pixels_per_meter) +
			static_cast<float>(cam_dimensions.y) * 0.5f * (1.0f / g_camera_zoom) -
			static_cast<float>(pivot_point.y);

		SDL_SetTextureColorMod(image->texture, static_cast<Uint8>(request.r), static_cast<Uint8>(request.g), static_cast<Uint8>(request.b));
		SDL_SetTextureAlphaMod(image->texture, static_cast<Uint8>(request.a));

		SDL_FRect dst_frect = {
			static_cast<float>(tex_rect.x),
			static_cast<float>(tex_rect.y),
			static_cast<float>(tex_rect.w),
			static_cast<float>(tex_rect.h)
		};

		SDL_FPoint pivot_fpoint = {
			static_cast<float>(pivot_point.x),
			static_cast<float>(pivot_point.y)
		};

		Helper::SDL_RenderCopyEx(
			-1, "",
			sdl_renderer,
			image->texture,
			nullptr,
			&dst_frect,
			static_cast<float>(request.rotation_degrees),
			&pivot_fpoint,
			static_cast<SDL_RendererFlip>(flip_mode));
	}

	SDL_RenderSetScale(sdl_renderer, 1.0f, 1.0f);
	g_scene_image_draw_queue.clear();

	for (const ImageDrawRequest& request : g_ui_image_draw_queue)
	{
		std::shared_ptr<Image> image = g_image_db->LoadImageFromDisk("./resources/images/", request.image_name);
		if (!image || image->texture == nullptr)
		{
			continue;
		}

		float tex_width_f = 0.0f;
		float tex_height_f = 0.0f;
		Helper::SDL_QueryTexture(image->texture, &tex_width_f, &tex_height_f);

		SDL_FRect dst_rect{
			request.x,
			request.y,
			tex_width_f,
			tex_height_f
		};

		SDL_SetTextureColorMod(image->texture, request.r, request.g, request.b);
		SDL_SetTextureAlphaMod(image->texture, request.a);
		Helper::SDL_RenderCopy(sdl_renderer, image->texture, nullptr, &dst_rect);
		SDL_SetTextureColorMod(image->texture, 255, 255, 255);
		SDL_SetTextureAlphaMod(image->texture, 255);
	}
	g_ui_image_draw_queue.clear();

	while (!g_text_draw_queue.empty())
	{
		TextDrawRequest request = g_text_draw_queue.front();
		g_text_draw_queue.pop();

		const std::string font_path = "./resources/fonts/" + request.font_name + ".ttf";
		TTF_Font* font = g_text_db->LoadFont(font_path, request.font_size);
		if (font == nullptr)
		{
			continue;
		}

		SDL_Color color{ request.r, request.g, request.b, request.a };
		g_text_db->DrawText(sdl_renderer, font, request.content, request.x, request.y, color);
	}

	if (!g_pixel_draw_queue.empty())
	{
		SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
		for (const PixelDrawRequest& request : g_pixel_draw_queue)
		{
			SDL_SetRenderDrawColor(sdl_renderer, request.r, request.g, request.b, request.a);
			SDL_RenderDrawPoint(sdl_renderer, request.x, request.y);
		}
		SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_NONE);
		g_pixel_draw_queue.clear();
	}
}