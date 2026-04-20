#pragma once
#include "Actor.h"
#include <vector>
#include <list>
#include <cassert>
#include <unordered_map>
#include <map>
#include <iostream>
#include <algorithm>
#include "glm/glm.hpp"

#define MAX_ACTORS 2000000

class ActorDB
{
public:
    ActorDB(int max_actors = MAX_ACTORS);

    Actor* AddActor(Actor actor);
    void Clear();

    /* Getters */
    std::vector<Actor>* GetActors();
    Actor* GetActorById(int id);
    Actor* GetActorByName(const std::string& name);
    const std::vector<Actor*>* GetActorsByName(const std::string& name) const;
    void UnregisterActor(Actor* actor);

private:
    int max_actors = MAX_ACTORS;
    int actor_counter = 0;
    int last_id = 0;
    std::vector<Actor> actors;

    std::unordered_map<std::string, std::vector<Actor*>> name_map;
    std::unordered_map<int, Actor*> id_map;
};

