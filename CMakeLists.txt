# CMakeList.txt : CMake project for snake-gl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.19)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

project ("snake-gl")

# libs
include(FetchContent)
find_package(flecs CONFIG REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
find_package(glbinding CONFIG REQUIRED)
find_package(globjects CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)


if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()


# Add source to this project's executable.
add_executable (snake-gl)
target_include_directories(snake-gl PRIVATE "include")
target_sources(snake-gl PRIVATE "snake-gl.cpp" "include/RenderSystem.h")
target_sources(snake-gl PRIVATE "include/GLstate.h" "include/PositionSystem.h" "include/Shader.h")

target_link_libraries(snake-gl PRIVATE $<IF:$<TARGET_EXISTS:flecs::flecs>,flecs::flecs,flecs::flecs_static>)
target_link_libraries(snake-gl PRIVATE sfml-system sfml-network sfml-graphics sfml-window)
target_link_libraries(snake-gl PRIVATE glbinding::glbinding glbinding::glbinding-aux)
target_link_libraries(snake-gl PRIVATE glm::glm)

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT snake-gl)
endif()

# copy resources
add_custom_command(
	TARGET snake-gl	COMMAND ${CMAKE_COMMAND} -E copy_directory 
	"${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:snake-gl>"
)
