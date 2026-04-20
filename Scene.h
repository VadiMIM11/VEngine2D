#pragma once
#include <string>
#include "ActorDB.h"
struct Scene
{
	Scene(std::string scene_name)
	{
		name = scene_name;
	}
	std::string name = "";
	ActorDB actors;
};

