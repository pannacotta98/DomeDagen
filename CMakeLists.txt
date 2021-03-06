cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project("Domedagen")
#
# Add the libraries that this application depends on,
#  1. SGCT (https://github.com/opensgct/sgct) for window handling and cluster support
#  2. libwebsockets (https://libwebsockets.org) for support for WebSockets
#  3. FBXSDK for importing models, textures, animation, etc
# Disable SSL requirement as it requires another library
set(LWS_WITH_SSL OFF CACHE BOOL "" FORCE)
set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "" FORCE)
add_subdirectory(ext/libwebsockets)
set_property(TARGET dist PROPERTY FOLDER "Dependencies")
set_property(TARGET websockets PROPERTY FOLDER "Dependencies")
set_property(TARGET websockets_shared PROPERTY FOLDER "Dependencies")
add_subdirectory(ext/sgct)
set_property(TARGET sgct PROPERTY FOLDER "Dependencies")
add_subdirectory(ext/assimp)
set_property(TARGET assimp PROPERTY FOLDER "Dependencies")
set_property(TARGET assimp PROPERTY CXX_STANDARD 17)
set_property(TARGET assimp PROPERTY CXX_STANDARD_REQUIRED ON)
#
# Adding the source files here that are compiled for this project
#
add_executable(${PROJECT_NAME}
  src/main.cpp
  src/constants.hpp
  src/websockethandler.h
  src/websockethandler.cpp
  src/utility.hpp
  src/utility.cpp
  src/renderable.hpp
  src/geometryhandler.hpp
  src/gameobject.hpp
  src/gameobject.cpp
  src/player.hpp
  src/player.cpp
  src/game.hpp
  src/game.cpp
  src/mesh.hpp
  src/mesh.cpp
  src/model.hpp
  src/model.cpp
  src/constants.hpp
  src/backgroundobject.hpp
  src/backgroundobject.cpp
  src/sceneobject.hpp
  src/sceneobject.cpp
  src/modelmanager.hpp
  src/modelmanager.cpp
  src/collectible.hpp
  src/collectible.cpp
  src/collectiblepool.hpp
  src/collectiblepool.cpp
  src/balljointconstraint.hpp
  src/balljointconstraint.cpp
  src/constants.hpp
  src/inireader.cpp
  src/inireader.h
  src/shaders/playervert.glsl
  src/shaders/playerfrag.glsl
  src/shaders/sceneobjectvert.glsl
  src/shaders/sceneobjectfrag.glsl
  src/shaders/testingvert.glsl
  src/shaders/testingfrag.glsl
  src/shaders/collectiblevert.glsl
  src/shaders/collectiblefrag.glsl
  src/shaders/backgroundvert.glsl
  src/shaders/backgroundfrag.glsl
  src/configs/fisheye_testing.xml
  src/configs/simple.xml
  src/configs/six_nodes.xml
  src/configs/two_fisheye_nodes.xml
  config.ini
)
target_include_directories(${PROJECT_NAME} PRIVATE
  src
  ext/sgct/include
  ext/libwebsockets/include
  ext/assimp/include
  ${LIBWEBSOCKETS_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME} PRIVATE sgct websockets assimp)
#
# Setting some compile settings for the project
#
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
if (MSVC)
  # Microsoft Visual Studio related compile options
  target_compile_options(${PROJECT_NAME} PRIVATE
    "/ZI"       # Edit and continue support
    "/MP"       # Multi-threading support
    "/W4"       # Highest warning level
    "/wd4201"   # nonstandard extension used : nameless struct/union    
    "/std:c++17"
    "/permissive-"
    "/Zc:strictStrings-"    # Windows header don't adhere to this
    "/Zc:__cplusplus" # Correctly set the __cplusplus macro
  )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # When compiling on Clang.  This most likely means compiling on MacOS
  target_compile_options(${PROJECT_NAME} PRIVATE
    "-stdlib=libc++"
    "-Wall"
    "-Wextra"
  )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  # Probably compiling on Linux
  target_compile_options(${PROJECT_NAME} PRIVATE
    "-ggdb"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
  )
endif ()
