# CMakeList.txt : CMake project for snake-gl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.19)
project ("snake-gl")


# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)


# libs
include(FetchContent)
find_package(flecs CONFIG REQUIRED)
find_package(SFML COMPONENTS system window graphics CONFIG REQUIRED)
find_package(glbinding CONFIG REQUIRED)
find_package(globjects CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)


# Add source to this project's executable.
add_executable (snake-gl)
target_include_directories(snake-gl PRIVATE "include")
target_sources(snake-gl PRIVATE "snake-gl.cpp" "include/RenderSystem.h" "include/GrowthSystem.h")
target_sources(snake-gl PRIVATE "include/GLstate.h" "include/PositionSystem.h" "include/Shader.h")
target_sources(snake-gl PRIVATE "shaders/triangle.vert" "shaders/triangle.frag")

target_link_libraries(snake-gl PRIVATE $<IF:$<TARGET_EXISTS:flecs::flecs>,flecs::flecs,flecs::flecs_static>)
target_link_libraries(snake-gl PRIVATE sfml-system sfml-network sfml-graphics sfml-window)
target_link_libraries(snake-gl PRIVATE glbinding::glbinding glbinding::glbinding-aux)
target_link_libraries(snake-gl PRIVATE glm::glm)

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

# copy resources
add_custom_command(
	TARGET snake-gl	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:snake-gl>"
)
