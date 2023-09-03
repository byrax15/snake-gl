// snake-gl.cpp : Defines the entry point for the application.
//

#include <span>
#include <iostream>
#include <algorithm>
#include <optional>
#include <random>

#include <flecs.h>
#include <SFML/Window.hpp>
#include <glbinding/glbinding.h>
#include <glbinding/gl45core/gl.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "Shader.h"
#include "GLstate.h"
#include "PositionSystem.h"
#include "RenderSystem.h"
#include "GrowthSystem.h"


inline constexpr Grid grid{ 24 };
inline constexpr auto squareSizeHalf = 1 / static_cast<gl::GLfloat>(grid.dim) / 2;

inline constexpr std::array vertices{
	glm::vec3{ squareSizeHalf, squareSizeHalf, 0. },
	glm::vec3{ -squareSizeHalf, squareSizeHalf, 0. },
	glm::vec3{ -squareSizeHalf, -squareSizeHalf, 0. },
	glm::vec3{ squareSizeHalf, -squareSizeHalf, 0. },
};

inline constexpr std::array indices{
	glm::uvec3{ 0, 1, 2 },
	glm::uvec3{ 0, 2, 3 },
};


int main(int argc, char** argv) {
	sf::ContextSettings settings;
	settings.majorVersion = 4;
	settings.minorVersion = 5;
	sf::Window window(sf::VideoMode(800, 600), "OpenGL", sf::Style::Default, settings);
	window.setActive();

	glbinding::initialize(sf::Context::getFunction);

	std::random_device					  rd;		 // a seed source for the random number engine
	std::mt19937						  gen(rd()); // mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<float> shouldSpawn{ 0, 1 };
	std::uniform_int_distribution<int>	  gridGenerator{ -grid.dim / 2, grid.dim / 2 - 1 };
	Shader								  triangle;
	{
		try {
			triangle = Shader{ "triangle.vert", "triangle.frag" };
		}
		catch (...) {
			return -1;
		}
		triangle.use();
	}

	std::vector<const Position*> applePositions;

	using GLstate = GLstate<true>;
	GLstate state{
		std::span{ vertices },
		std::span{ indices },
	};

	flecs::world ecs;

	const auto addApple = [&]() {
		const auto& apple = ecs.entity()
								.add<Apple>()
								.set<Position>({ { gridGenerator(gen), gridGenerator(gen) } })
								.set<Renderer>({ { 1, 0, 0, 1 } });
		applePositions.push_back(apple.get<Position>());
	};

	ecs.entity("Snake")
		.add<Player>()
		.set<Position>({ { 0, 0 } })
		.set<Velocity>({ { 0, 0 } })
		.set<Renderer>({ { 1, 1, .7, 1 } });
	addApple();
	ecs.system("AppleSpawner")
		.iter([&](flecs::iter& it) {
			if (shouldSpawn(gen) > .01f)
				return;
			addApple();
		});
	ecs.system<Player, Velocity>("ProcessInput")
		.each([&](const Player, Velocity& v) {
			// handle events
			sf::Event event{};
			while (window.pollEvent(event)) {
				if (event.type == sf::Event::Closed ||
					event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
					// end the program
					ecs.quit();
				}
				else if (event.type == sf::Event::Resized) {
					// adjust the viewport when the window is resized
					gl::glViewport(0, 0, static_cast<gl::GLsizei>(event.size.width), static_cast<gl::GLsizei>(event.size.height));
				}
				else if (event.type == sf::Event::KeyPressed) {
					switch (event.key.code) {
					case sf::Keyboard::O:
						gl::glPolygonMode(gl::GL_FRONT, gl::GL_LINE);
						break;
					case sf::Keyboard::P:
						gl::glPolygonMode(gl::GL_FRONT, gl::GL_FILL);
						break;
					case sf::Keyboard::W:
						v.vec = { 0, 1 };
						break;
					case sf::Keyboard::S:
						v.vec = { 0, -1 };
						break;
					case sf::Keyboard::A:
						v.vec = { -1, 0 };
						break;
					case sf::Keyboard::D:
						v.vec = { 1, 0 };
						break;
					default:
						break;
					}
				}
			}
		});
	ecs.system<Position, Velocity>("ProcessVelocities")
		.each([&](Position& p, const Velocity& v) {
			p.vec += v.vec;
			grid.clampIn(p.vec);
			printf("p.vec {%d, %d}\n", p.vec.x, p.vec.y);
		});
	ecs.system("DrawBackground")
		.iter([&](flecs::iter&) {
			// clear the buffers
			gl::glClearColor(0, .1, .05, 1);
			gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
			for (auto i = -grid.dim / 2; i < grid.dim / 2; ++i) {
				for (auto j = -grid.dim / 2; j < grid.dim / 2; ++j) {
					if ((i + j) % 2 == 0)
						continue;
					triangle.setUniform<gl::glUniform2fv>("vTranslation", grid.toDeviceCoordinates(glm::ivec2{ i, j }));
					triangle.setUniform<gl::glUniform4fv>("fSquareColor", glm::vec4{ 0, .1, .05, 1 } * 1.2f);
					state.draw();
				}
			}
		});
	ecs.system<const Position, const Renderer>("DrawLoop")
		.each([&](const Position& position, const Renderer& renderer) {
			// draw...
			triangle.setUniform<gl::glUniform2fv>("vTranslation", grid.toDeviceCoordinates(position.vec));
			triangle.setUniform<gl::glUniform4fv>("fSquareColor", renderer.color);
			state.draw();
		});
	ecs.system("EndFrame")
		.iter([&](flecs::iter&) {
			// end the current frame (internally swaps the front and back buffers)
			window.display();
		});
	return ecs.app()
		.target_fps(10)
		.enable_rest()
		//.enable_monitor()
		.run();
}