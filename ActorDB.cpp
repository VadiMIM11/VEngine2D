#include "ActorDB.h"

ActorDB::ActorDB(int max_actors) : max_actors(max_actors)
{
	actors.reserve(max_actors);
}

std::vector<Actor>* ActorDB::GetActors()
{
	return &actors;
}

Actor* ActorDB::GetActorById(int id)
{
	auto it = id_map.find(id);
	if (it != id_map.end()) return it->second;
	return nullptr;
}

Actor* ActorDB::GetActorByName(const std::string& name)
{
	auto it = name_map.find(name);
	if (it == name_map.end() || it->second.empty()) return nullptr;
	return it->second.front();
}

const std::vector<Actor*>* ActorDB::GetActorsByName(const std::string& name) const
{
	auto it = name_map.find(name);
	if (it == name_map.end()) return nullptr;
	return &(it->second);
}

void ActorDB::UnregisterActor(Actor* actor)
{
	if (actor == nullptr)
	{
		return;
	}

	auto it_name = name_map.find(actor->actor_name);
	if (it_name != name_map.end())
	{
		auto& actors_for_name = it_name->second;
		actors_for_name.erase(std::remove(actors_for_name.begin(), actors_for_name.end(), actor), actors_for_name.end());
		if (actors_for_name.empty())
		{
			name_map.erase(it_name);
		}
	}

	id_map.erase(actor->id);
}

Actor* ActorDB::AddActor(Actor actor)
{
	if (actor_counter >= max_actors)
	{
		std::cout << "Actor limit exceeded" << std::endl;
		exit(1);
	}

	actors.push_back(actor);
	Actor* stored_actor = &actors.back();
	stored_actor->id = last_id + 1;

	name_map[stored_actor->actor_name].push_back(stored_actor);
	id_map[stored_actor->id] = stored_actor;

	actor_counter++;
	last_id++;
	return stored_actor;
}

void ActorDB::Clear()
{
	last_id = 0;
	actor_counter = 0;
	actors.clear();
	name_map.clear();
	id_map.clear();
}

