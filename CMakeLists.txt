cmake_minimum_required(VERSION 3.20)
project(VEngine2D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(VENGINE2D_WEB_OUTPUT_MODE "html" CACHE STRING "Emscripten output mode: html or js")
set_property(CACHE VENGINE2D_WEB_OUTPUT_MODE PROPERTY STRINGS html js)
set(VENGINE2D_WEB_BASENAME "VEngine2D" CACHE STRING "Base name for web artifacts (js/wasm/data)")

# --- Source Files ---
set(ENGINE_SRCS
    Actor.cpp
    ActorDB.cpp
    AudioDB.cpp
    Component.cpp
    ComponentDB.cpp
    Engine.cpp
    ImageDB.cpp
    InputManager.cpp
    main.cpp
    Renderer.cpp
    Rigidbody.cpp
    ScriptBindings.cpp
    TextDB.cpp
    ParticleSystem.cpp
)

file(GLOB BOX2D_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/third_party/box2d-2.4.1/src/collision/*.cpp"
    "${CMAKE_SOURCE_DIR}/third_party/box2d-2.4.1/src/common/*.cpp"
    "${CMAKE_SOURCE_DIR}/third_party/box2d-2.4.1/src/dynamics/*.cpp"
    "${CMAKE_SOURCE_DIR}/third_party/box2d-2.4.1/src/rope/*.cpp"
)

file(GLOB LUA_C_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/third_party/lua/*.c"
)

add_executable(VEngine2D
    ${ENGINE_SRCS}
    ${BOX2D_SRCS}
    ${LUA_C_SRCS}
)

# --- Include Paths ---
target_include_directories(VEngine2D PRIVATE
    .
    third_party
    third_party/SDL2
    third_party/SDL2_image
    third_party/SDL2_mixer
    third_party/SDL2_ttf
    third_party/lua
    third_party/LuaBridge
    third_party/box2d-2.4.1/include
    third_party/box2d-2.4.1/src
)

# --- Defines from existing native builds ---
target_compile_definitions(VEngine2D PRIVATE
    _CONSOLE
    _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
)

# --- Warnings / Config-specific options ---
if(MSVC)
    target_compile_options(VEngine2D PRIVATE /W3)
else()
    target_compile_options(VEngine2D PRIVATE -Wall -Wextra)
endif()

# --- Platform-specific linking ---
if(EMSCRIPTEN)
    if(VENGINE2D_WEB_OUTPUT_MODE STREQUAL "js")
        set_target_properties(VEngine2D PROPERTIES SUFFIX ".js")
    else()
        set_target_properties(VEngine2D PROPERTIES SUFFIX ".html")
    endif()

    # Emscripten SDL ports + virtual filesystem + packaged resources
    target_link_options(VEngine2D PRIVATE
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
    )

    if(VENGINE2D_WEB_OUTPUT_MODE STREQUAL "js")
        # JS module build: use configured basename for exported name and output files
        set_target_properties(VEngine2D PROPERTIES OUTPUT_NAME "${VENGINE2D_WEB_BASENAME}")
        target_link_options(VEngine2D PRIVATE
            "-sMODULARIZE=1"
            "-sEXPORT_ES6=1"
            "-sEXPORT_NAME=${VENGINE2D_WEB_BASENAME}"
            "-sUSE_SDL=2"
            "-sUSE_SDL_IMAGE=2"
            "-sSDL2_IMAGE_FORMATS=['png']"
            "-sUSE_SDL_TTF=2"
            "-sUSE_SDL_MIXER=2"
            "-sSDL2_MIXER_FORMATS=['wav','ogg','mp3']"
            "-sFORCE_FILESYSTEM=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sDISABLE_EXCEPTION_CATCHING=0"
            "--preload-file" "${CMAKE_SOURCE_DIR}/resources@/resources"
        )
    else()
        # HTML shell build: preserve original export name and behavior
        target_link_options(VEngine2D PRIVATE
            "-sUSE_SDL=2"
            "-sUSE_SDL_IMAGE=2"
            "-sSDL2_IMAGE_FORMATS=['png']"
            "-sUSE_SDL_TTF=2"
            "-sUSE_SDL_MIXER=2"
            "-sSDL2_MIXER_FORMATS=['wav','ogg','mp3']"
            "-sFORCE_FILESYSTEM=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sDISABLE_EXCEPTION_CATCHING=0"
            "--preload-file" "${CMAKE_SOURCE_DIR}/resources@/resources"
        )
    endif()
else()
    target_link_directories(VEngine2D PRIVATE
        third_party/SDL2/lib
        third_party/SDL2_image/lib
        third_party/SDL2_mixer/lib
        third_party/SDL2_ttf/lib
    )

    target_link_libraries(VEngine2D PRIVATE
        SDL2main
        SDL2
        SDL2_image
        SDL2_mixer
        SDL2_ttf
    )

    # Linux often needs pthread/dl for SDL stacks.
    if(UNIX AND NOT APPLE)
        target_link_libraries(VEngine2D PRIVATE pthread dl)
    endif()

    # Optional: copy runtime SDL DLLs for Windows local run.
    if(WIN32)
        add_custom_command(TARGET VEngine2D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/third_party/dlls"
                "$<TARGET_FILE_DIR:VEngine2D>"
        )
		# Copy resources folder
		if (EXISTS "${CMAKE_SOURCE_DIR}/resources")
			add_custom_command(TARGET VEngine2D POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_directory
					"${CMAKE_SOURCE_DIR}/resources"
					"$<TARGET_FILE_DIR:VEngine2D>/resources"
			)
		endif()
    endif()
endif()