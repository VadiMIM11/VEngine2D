#include "Rigidbody.h"
#include <memory>
#include <cstdint>
#include "glm/glm.hpp"
#include "Actor.h"
#include "LuaBridge/LuaBridge.h"

namespace
{
	std::unique_ptr<b2World> g_physics_world;

	constexpr uint16 COLLIDER_CATEGORY = 0x0001;
	constexpr uint16 TRIGGER_CATEGORY = 0x0002;
	constexpr uint16 PHANTOM_CATEGORY = 0x0004;

	float ClockwiseDegreesToBox2DRadians(float clockwise_degrees)
	{
		return clockwise_degrees * (b2_pi / 180.0f);
	}

	float Box2DRadiansToClockwiseDegrees(float box2d_radians)
	{
		return box2d_radians * (180.0f / b2_pi);
	}

	void DispatchCollisionLifecycle(
		Actor* self_actor,
		Actor* other_actor,
		b2Fixture* self_fixture,
		b2Fixture* other_fixture,
		b2Contact* contact,
		const char* fn_name,
		bool point_and_normal_are_sentinel)
	{
		if (self_actor == nullptr || self_fixture == nullptr || other_fixture == nullptr)
		{
			return;
		}

		b2WorldManifold world_manifold;
		contact->GetWorldManifold(&world_manifold);

		const b2Vec2 sentinel(-999.0f, -999.0f);
		const b2Vec2 point = point_and_normal_are_sentinel ? sentinel : world_manifold.points[0];
		const b2Vec2 normal = point_and_normal_are_sentinel ? sentinel : world_manifold.normal;
		const b2Fixture* contact_fixture_a = contact->GetFixtureA();
		const b2Fixture* contact_fixture_b = contact->GetFixtureB();
		const b2Vec2 relative_velocity =
			(contact_fixture_a != nullptr && contact_fixture_b != nullptr)
			? (contact_fixture_a->GetBody()->GetLinearVelocity() - contact_fixture_b->GetBody()->GetLinearVelocity())
			: b2Vec2_zero;

		for (const auto& [component_key, instance_ptr] : self_actor->component_instances)
		{
			if (!instance_ptr)
			{
				continue;
			}

			luabridge::LuaRef& instance = *instance_ptr;
			luabridge::LuaRef enabled = instance["enabled"];
			if (enabled.isBool() && enabled.cast<bool>() == false)
			{
				continue;
			}

			luabridge::LuaRef fn = instance[fn_name];
			if (!fn.isFunction())
			{
				continue;
			}

			lua_State* L = instance.state();
			luabridge::LuaRef collision = luabridge::newTable(L);
			collision["other"] = other_actor;
			collision["point"] = point;
			collision["relative_velocity"] = relative_velocity;
			collision["normal"] = normal;

			try
			{
				fn(instance, collision);
			}
			catch (const luabridge::LuaException&)
			{
			}
		}
	}

	class PhysicsContactListener final : public b2ContactListener
	{
	public:
		void BeginContact(b2Contact* contact) override
		{
			if (contact == nullptr)
			{
				return;
			}

			b2Fixture* fixture_a = contact->GetFixtureA();
			b2Fixture* fixture_b = contact->GetFixtureB();
			if (fixture_a == nullptr || fixture_b == nullptr)
			{
				return;
			}

			Actor* actor_a = reinterpret_cast<Actor*>(fixture_a->GetUserData().pointer);
			Actor* actor_b = reinterpret_cast<Actor*>(fixture_b->GetUserData().pointer);

			const bool a_is_sensor = fixture_a->IsSensor();
			const bool b_is_sensor = fixture_b->IsSensor();
			if (a_is_sensor && b_is_sensor)
			{
				DispatchCollisionLifecycle(actor_a, actor_b, fixture_a, fixture_b, contact, "OnTriggerEnter", true);
				DispatchCollisionLifecycle(actor_b, actor_a, fixture_b, fixture_a, contact, "OnTriggerEnter", true);
			}
			else if (!a_is_sensor && !b_is_sensor)
			{
				DispatchCollisionLifecycle(actor_a, actor_b, fixture_a, fixture_b, contact, "OnCollisionEnter", false);
				DispatchCollisionLifecycle(actor_b, actor_a, fixture_b, fixture_a, contact, "OnCollisionEnter", false);
			}
		}

		void EndContact(b2Contact* contact) override
		{
			if (contact == nullptr)
			{
				return;
			}

			b2Fixture* fixture_a = contact->GetFixtureA();
			b2Fixture* fixture_b = contact->GetFixtureB();
			if (fixture_a == nullptr || fixture_b == nullptr)
			{
				return;
			}

			Actor* actor_a = reinterpret_cast<Actor*>(fixture_a->GetUserData().pointer);
			Actor* actor_b = reinterpret_cast<Actor*>(fixture_b->GetUserData().pointer);

			const bool a_is_sensor = fixture_a->IsSensor();
			const bool b_is_sensor = fixture_b->IsSensor();
			if (a_is_sensor && b_is_sensor)
			{
				DispatchCollisionLifecycle(actor_a, actor_b, fixture_a, fixture_b, contact, "OnTriggerExit", true);
				DispatchCollisionLifecycle(actor_b, actor_a, fixture_b, fixture_a, contact, "OnTriggerExit", true);
			}
			else if (!a_is_sensor && !b_is_sensor)
			{
				DispatchCollisionLifecycle(actor_a, actor_b, fixture_a, fixture_b, contact, "OnCollisionExit", true);
				DispatchCollisionLifecycle(actor_b, actor_a, fixture_b, fixture_a, contact, "OnCollisionExit", true);
			}
		}
	};

	PhysicsContactListener g_contact_listener;
}

b2World* Rigidbody::GetOrCreatePhysicsWorld()
{
	if (!g_physics_world)
	{
		g_physics_world = std::make_unique<b2World>(b2Vec2(0.0f, 9.8f));
		g_physics_world->SetContactListener(&g_contact_listener);
	}

	return g_physics_world.get();
}

b2World* Rigidbody::GetPhysicsWorld()
{
	return g_physics_world.get();
}

void Rigidbody::OnStart()
{
	if (body != nullptr)
	{
		return;
	}

	b2World* world = GetOrCreatePhysicsWorld();
	if (world == nullptr)
	{
		return;
	}

	b2BodyDef body_def;
	body_def.position.Set(x, y);

	if (body_type == "static")
	{
		body_def.type = b2_staticBody;
	}
	else if (body_type == "kinematic")
	{
		body_def.type = b2_kinematicBody;
	}
	else
	{
		body_def.type = b2_dynamicBody;
	}

	body_def.bullet = precise;
	body_def.gravityScale = gravity_scale;
	body_def.angularDamping = angular_friction;
	body_def.angle = ClockwiseDegreesToBox2DRadians(rotation);

	body = world->CreateBody(&body_def);
	if (body == nullptr)
	{
		return;
	}

	bool created_fixture = false;

	if (has_collider)
	{
		b2FixtureDef fixture_def;
		fixture_def.isSensor = false;
		fixture_def.density = density;
		fixture_def.friction = friction;
		fixture_def.restitution = bounciness;
		fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
		fixture_def.filter.categoryBits = COLLIDER_CATEGORY;
		fixture_def.filter.maskBits = COLLIDER_CATEGORY;

		if (collider_type == "circle")
		{
			b2CircleShape circle_shape;
			circle_shape.m_radius = radius;
			fixture_def.shape = &circle_shape;
			body->CreateFixture(&fixture_def);
		}
		else
		{
			b2PolygonShape box_shape;
			box_shape.SetAsBox(width * 0.5f, height * 0.5f);
			fixture_def.shape = &box_shape;
			body->CreateFixture(&fixture_def);
		}

		created_fixture = true;
	}

	if (has_trigger)
	{
		b2FixtureDef trigger_fixture_def;
		trigger_fixture_def.isSensor = true;
		trigger_fixture_def.density = density;
		trigger_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
		trigger_fixture_def.filter.categoryBits = TRIGGER_CATEGORY;
		trigger_fixture_def.filter.maskBits = TRIGGER_CATEGORY;

		if (trigger_type == "circle")
		{
			b2CircleShape trigger_circle_shape;
			trigger_circle_shape.m_radius = trigger_radius;
			trigger_fixture_def.shape = &trigger_circle_shape;
			body->CreateFixture(&trigger_fixture_def);
		}
		else
		{
			b2PolygonShape trigger_box_shape;
			trigger_box_shape.SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f);
			trigger_fixture_def.shape = &trigger_box_shape;
			body->CreateFixture(&trigger_fixture_def);
		}

		created_fixture = true;
	}

	if (!created_fixture)
	{
		b2PolygonShape phantom_shape;
		phantom_shape.SetAsBox(width * 0.5f, height * 0.5f);

		b2FixtureDef phantom_fixture_def;
		phantom_fixture_def.shape = &phantom_shape;
		phantom_fixture_def.density = density;
		phantom_fixture_def.isSensor = true;
		phantom_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
		phantom_fixture_def.filter.categoryBits = PHANTOM_CATEGORY;
		phantom_fixture_def.filter.maskBits = 0x0000;
		body->CreateFixture(&phantom_fixture_def);
	}
}

void Rigidbody::OnUpdate()
{
}

void Rigidbody::OnLateUpdate()
{
}

void Rigidbody::OnDestroy()
{
	if (body == nullptr)
	{
		return;
	}

	b2World* world = GetPhysicsWorld();
	if (world == nullptr)
	{
		body = nullptr;
		return;
	}

	world->DestroyBody(body);
	body = nullptr;
}

void Rigidbody::AddForce(const b2Vec2& force)
{
	if (body == nullptr)
	{
		return;
	}

	body->ApplyForceToCenter(force, true);
}

void Rigidbody::SetVelocity(const b2Vec2& velocity)
{
	if (body == nullptr)
	{
		return;
	}

	body->SetLinearVelocity(velocity);
}

void Rigidbody::SetPosition(const b2Vec2& position)
{
	x = position.x;
	y = position.y;

	if (body == nullptr)
	{
		return;
	}

	body->SetTransform(position, body->GetAngle());
}

void Rigidbody::SetRotation(float degrees_clockwise)
{
	rotation = degrees_clockwise;
	float angle_radians = ClockwiseDegreesToBox2DRadians(degrees_clockwise);

	if (body == nullptr)
	{
		return;
	}

	body->SetTransform(body->GetPosition(), angle_radians);
}

void Rigidbody::SetAngularVelocity(float degrees_clockwise)
{
	if (body == nullptr)
	{
		return;
	}

	body->SetAngularVelocity(ClockwiseDegreesToBox2DRadians(degrees_clockwise));
}

void Rigidbody::SetGravityScale(float value)
{
	gravity_scale = value;

	if (body == nullptr)
	{
		return;
	}

	body->SetGravityScale(value);
}

void Rigidbody::SetUpDirection(b2Vec2 direction)
{
	if (direction.LengthSquared() <= 0.0f)
	{
		return;
	}

	direction.Normalize();
	float clockwise_radians = glm::atan(direction.x, -direction.y);
	SetRotation(clockwise_radians * (180.0f / b2_pi));
}

void Rigidbody::SetRightDirection(b2Vec2 direction)
{
	if (direction.LengthSquared() <= 0.0f)
	{
		return;
	}

	direction.Normalize();
	float clockwise_radians = glm::atan(direction.x, -direction.y) - (b2_pi / 2.0f);
	SetRotation(clockwise_radians * (180.0f / b2_pi));
}

b2Vec2 Rigidbody::GetPosition() const
{
	if (body != nullptr)
	{
		return body->GetPosition();
	}

	return b2Vec2(x, y);
}

float Rigidbody::GetRotation() const
{
	if (body != nullptr)
	{
		return Box2DRadiansToClockwiseDegrees(body->GetAngle());
	}

	return rotation;
}

b2Vec2 Rigidbody::GetVelocity() const
{
	if (body != nullptr)
	{
		return body->GetLinearVelocity();
	}

	return b2Vec2_zero;
}

float Rigidbody::GetAngularVelocity() const
{
	if (body != nullptr)
	{
		return Box2DRadiansToClockwiseDegrees(body->GetAngularVelocity());
	}

	return 0.0f;
}

float Rigidbody::GetGravityScale() const
{
	if (body != nullptr)
	{
		return body->GetGravityScale();
	}

	return gravity_scale;
}

b2Vec2 Rigidbody::GetUpDirection() const
{
	float angle = 0.0f;
	if (body != nullptr)
	{
		angle = body->GetAngle();
	}
	else
	{
		angle = ClockwiseDegreesToBox2DRadians(rotation);
	}

	b2Vec2 result = b2Vec2(glm::sin(angle), -glm::cos(angle));
	result.Normalize();
	return result;
}

b2Vec2 Rigidbody::GetRightDirection() const
{
	float angle = 0.0f;
	if (body != nullptr)
	{
		angle = body->GetAngle();
	}
	else
	{
		angle = ClockwiseDegreesToBox2DRadians(rotation);
	}

	b2Vec2 result = b2Vec2(glm::cos(angle), glm::sin(angle));
	result.Normalize();
	return result;
}

void Rigidbody::StepPhysicsWorld()
{
	b2World* world = GetPhysicsWorld();
	if (world == nullptr)
	{
		return;
	}

	world->Step(1.0f / 60.0f, 8, 3);
}
