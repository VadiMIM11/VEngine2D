CXX := clang++
CC  := clang
TARGET := game_engine_linux
BUILD ?= release
OBJDIR := .obj

# --- Source Files ---
# Main engine files (Added Rigidbody.cpp)
ENGINE_SRCS := Actor.cpp ActorDB.cpp AudioDB.cpp Component.cpp ComponentDB.cpp \
               Engine.cpp ImageDB.cpp InputManager.cpp main.cpp Renderer.cpp \
               Rigidbody.cpp ScriptBindings.cpp TextDB.cpp ParticleSystem.cpp

# Box2D Source files (Found in various subdirectories)
BOX2D_SRCS := $(wildcard third_party/box2d-2.4.1/src/collision/*.cpp) \
              $(wildcard third_party/box2d-2.4.1/src/common/*.cpp) \
              $(wildcard third_party/box2d-2.4.1/src/dynamics/*.cpp) \
              $(wildcard third_party/box2d-2.4.1/src/rope/*.cpp)

CPP_SRCS := $(ENGINE_SRCS) $(BOX2D_SRCS)

# Lua C Sources
C_SRCS := $(wildcard third_party/lua/*.c)

# --- Paths and Definitions ---
# Added Box2D include and src paths from vcxproj
INCLUDES := -I./third_party \
            -I./third_party/SDL2 \
            -I./third_party/SDL2_image \
            -I./third_party/SDL2_mixer \
            -I./third_party/SDL2_ttf \
            -I./third_party/lua \
            -I./third_party/LuaBridge \
            -I./third_party/box2d-2.4.1/include \
            -I./third_party/box2d-2.4.1/src

COMMON_DEFS := -D_CONSOLE -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS \
               -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

ifeq ($(BUILD),debug)
OPT := -O0 -g -DDEBUG=1 -D_DEBUG
else
OPT := -O3 -DNDEBUG
endif

CXXFLAGS := -std=c++17 $(INCLUDES) $(COMMON_DEFS) $(OPT) -Wall -Wextra
CFLAGS   := -std=c17   $(INCLUDES) $(COMMON_DEFS) $(OPT) -Wall -Wextra

LDFLAGS := -L./third_party/SDL2/lib \
           -L./third_party/SDL2_image/lib \
           -L./third_party/SDL2_mixer/lib \
           -L./third_party/SDL2_ttf/lib
LDLIBS  := -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer -lSDL2_ttf

# --- Object File Mapping ---
# This mirrors the source directory structure inside $(OBJDIR)
CPP_OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(CPP_SRCS))
C_OBJS   := $(patsubst %.c,$(OBJDIR)/%.o,$(C_SRCS))
OBJS := $(CPP_OBJS) $(C_OBJS)
DEPS := $(OBJS:.o=.d)

# --- Rules ---
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $(TARGET)

# Rule for all C++ files (Engine + Box2D)
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Rule for all C files (Lua)
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all debug release clean
-include $(DEPS)