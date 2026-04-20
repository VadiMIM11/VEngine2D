#include <iostream>
#include "glm/glm.hpp"
#include "Engine.h"
#include "rapidjson/document.h"
#include "lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"


int main(int argc, char* argv[]) 
{
	Engine engine;
	engine.Run();
	return 0;
}