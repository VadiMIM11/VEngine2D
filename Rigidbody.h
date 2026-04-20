#pragma once
#include <string>
#include "box2d/box2d.h"

class Actor;

class Rigidbody
{
public:
	std::string key;
	std::string type;
	bool enabled = true;

	Actor* actor = nullptr;
	std::string actor_name;
	int id = -1;

	float x = 0.0f;
	float y = 0.0f;
	float width = 1.0f;
	float height = 1.0f;
	float radius = 0.5f;
	std::string body_type = "dynamic";
	bool precise = true;
	float gravity_scale = 1.0f;
	float density = 1.0f;
	float angular_friction = 0.3f;
	float rotation = 0.0f;
	bool has_collider = true;
	std::string collider_type = "box";
	float friction = 0.3f;
	float bounciness = 0.3f;
	bool has_trigger = true;
	std::string trigger_type = "box";
	float trigger_width = 1.0f;
	float trigger_height = 1.0f;
	float trigger_radius = 0.5f;

	void OnStart();
	void OnUpdate();
	void OnLateUpdate();
	void OnDestroy();

	void AddForce(const b2Vec2& force);
	void SetVelocity(const b2Vec2& velocity);
	void SetPosition(const b2Vec2& position);
	void SetRotation(float degrees_clockwise);
	void SetAngularVelocity(float degrees_clockwise);
	void SetGravityScale(float value);
	void SetUpDirection(b2Vec2 direction);
	void SetRightDirection(b2Vec2 direction);

	b2Vec2 GetPosition() const;
	float GetRotation() const;
	b2Vec2 GetVelocity() const;
	float GetAngularVelocity() const;
	float GetGravityScale() const;
	b2Vec2 GetUpDirection() const;
	b2Vec2 GetRightDirection() const;

	static void StepPhysicsWorld();
	static b2World* GetPhysicsWorld();

private:
	b2Body* body = nullptr;

	static b2World* GetOrCreatePhysicsWorld();
};
