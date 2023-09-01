// snake-gl.cpp : Defines the entry point for the application.
//

#include <span>
#include <iostream>
#include <algorithm>

#include <flecs.h>
#include <SFML/Window.hpp>
#include <glbinding/glbinding.h>
#include <glbinding/gl45core/gl.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "PositionSystem.h"
#include "RenderSystem.h"
#include "Shader.h"
#include "GLstate.h"

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

	Shader triangle{
		"triangle.vert",
		"triangle.frag"
	};
	triangle.use();

	using GLstate = GLstate<true>;
	GLstate state{
		std::span{ vertices },
		std::span{ indices },
	};

	flecs::world ecs;
	ecs.entity("Snake")
		.add<Player>()
		.set<Position>({ { 0, 0 } })
		.set<Velocity>({ { 0, 0 } })
		.set<Renderer>({ { 1, 1, .7, 1 } });

	ecs.system<const Player, Velocity>("ProcessInput")
		.each([&](const Player& p, Velocity& v) {
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
					gl::glViewport(0, 0, event.size.width, event.size.height);
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
	ecs.system<Position, const Velocity>("ProcessVelocities")
		.each([&](Position& p, const Velocity& v) {
			p.vec += v.vec;
			grid.clampIn(p.vec);
			//printf("p.vec {%d, %d}\n", p.vec.x, p.vec.y);
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
		.iter([&](flecs::iter& it, const Position* positions, const Renderer* renderers) {
			for (auto i : it) {
				// draw..
				triangle.setUniform<gl::glUniform2fv>("vTranslation", grid.toDeviceCoordinates(positions[i].vec));
				triangle.setUniform<gl::glUniform4fv>("fSquareColor", renderers[i].color);
				state.draw();
			}
			// end the current frame (internally swaps the front and back buffers)
			window.display();
		});
	ecs.app()
		.target_fps(15)
		//.enable_rest()
		//.enable_monitor()
		.run();
}