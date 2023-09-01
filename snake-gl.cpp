// snake-gl.cpp : Defines the entry point for the application.
//

#include <span>
#include <iostream>

#include <flecs.h>
#include <SFML/Window.hpp>
#include <glbinding/glbinding.h>
#include <glbinding/gl45core/gl.h>
#include <glm/glm.hpp>

#include "PositionSystem.h"
#include "Shader.h"
#include "GLstate.h"


inline constexpr std::array vertices{
	glm::vec3{ 0.05, 0.05, 0. },
	glm::vec3{ -0.05, 0.05, 0. },
	glm::vec3{ -0.05, -0.05, 0. },
	glm::vec3{ 0.05, -0.05, 0. },
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
	GLstate<true> state{
		std::span{ vertices },
		std::span{ indices },
	};

	flecs::world ecs;

	for (const auto& p : positions)
		ecs.entity().set<Position>(p);

	ecs.system("ProcessInput")
		.iter([&](flecs::iter&) {
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
					if (event.key.code == sf::Keyboard::D)
						gl::glPolygonMode(gl::GL_FRONT, gl::GL_LINE);
					else if (event.key.code == sf::Keyboard::F)
						gl::glPolygonMode(gl::GL_FRONT, gl::GL_FILL);
				}
			}
		});
	ecs.system<Position const>("DrawLoop")
		.iter([&](flecs::iter& it, Position const* positions) {
			// clear the buffers
			gl::glClearColor(0, .1, .05, 1);
			gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

			for (auto i : it) {
				// draw..
				triangle.setUniform(gl::glUniform2fv, "vTranslation", positions[i]);
				state.draw();
			}

			// end the current frame (internally swaps the front and back buffers)
			window.display();
		});
	ecs.app()
		.target_fps(60)
		.enable_rest()
		.enable_monitor()
		.run();
}