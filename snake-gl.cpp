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
	flecs::world ecs;

	sf::ContextSettings settings;
	settings.majorVersion = 4;
	settings.minorVersion = 5;
	sf::Window window(sf::VideoMode(800, 600), "SnakeGL", sf::Style::Default, settings);
	window.setActive();
	glbinding::initialize(sf::Context::getFunction);

	std::random_device					  rd;		 // a seed source for the random number engine
	std::mt19937						  gen(rd()); // mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<float> colorOffset{ .8, 1.5 };
	const auto							  randColor = [&]() { return colorOffset(gen); };
	std::uniform_int_distribution<int>	  gridGenerator{ -grid.dim / 2, grid.dim / 2 - 1 };
	const auto							  randGrid = [&]() { return gridGenerator(gen); };


	Shader triangle;
	{
		try {
			triangle = Shader{ "triangle.vert", "triangle.frag" };
		}
		catch (...) {
			return -1;
		}
		triangle.use();
	}

	using GLstate = GLstate<true>;
	GLstate state{
		std::span{ vertices },
		std::span{ indices },
	};

	auto head  = Head::create(ecs.entity("SnakeHead"), ecs, Position::zero(), Velocity::zero(), Renderer::tailColor());
	auto tail0 = Tail::create(ecs.entity("Tail0"), ecs, Position{ { 0, -1 } }, Velocity::zero(), Renderer::random(randColor));
	auto tail1 = Tail::create(ecs.entity("Tail1"), ecs, Position{ { 0, -2 } }, Velocity::zero(), Renderer::random(randColor));

	auto snake = ecs.entity("Snake");
	snake.set<Snake>({ head, { tail0, tail1 } });

	auto apple = Apple::create(ecs.entity("StartApple"), ecs, Position::random(randGrid), Velocity::zero(), Renderer::red());

	// ecs.system("AppleSpawner")
	//	.iter([&](flecs::iter&) {
	//		if (shouldSpawn(gen) > .01f)
	//			return;
	//		addApple();
	//	});
	ecs.system<const Head, Velocity>("ProcessInput")
		.each([&](const Head, Velocity& v) {
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

	bool ateApple = false;
	ecs.system<const Head, Position, const Velocity>("MoveHead")
		.each([&](const Head, Position& p, const Velocity& v) {
			auto& snakeStruct = *snake.get_mut<Snake>();
			if (ateApple) {
				auto newTail = Tail::create(
					ecs.entity(), ecs, snakeStruct.freeEndPosition(), Velocity::zero(), Renderer::random(randColor));
				snakeStruct.tail.push_back(newTail);
				ateApple = false;
			}
			else {
				for (auto it{ snakeStruct.tail.end() - 1 }; it > snakeStruct.tail.begin(); --it) {
					it->get_mut<Position>()->vec = (it - 1)->get<Position>()->vec;
				}
				snakeStruct.tail[0].get_mut<Position>()->vec = snakeStruct.head.get<Position>()->vec;
			}

			p.vec += v.vec;
			grid.clampIn(p.vec);
			// printf("p.vec {%d, %d}\n", p.vec.x, p.vec.y);
		});
	const auto headPositions = ecs.query<const Head, const Position>("HeadPositionQuery");
	ecs.system<const Apple, const Position>("EatApple")
		.each([&](flecs::entity e, const Apple, const Position& aPos) {
			headPositions.each([&](const Head, const Position& pPos) {
				if (aPos.vec == pPos.vec) {
					Apple::create(ecs.entity(), ecs, Position::random(randGrid), Velocity::zero(), Renderer::red());
					e.destruct();
					ateApple = true;
				}
			});
		});
	ecs.system("DrawBackground")
		.iter([&](flecs::iter&) {
			// clear the buffers
			gl::glClearColor(.7, .7, .7, 1);
			gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);
			for (auto i = -grid.dim / 2; i < grid.dim / 2; ++i) {
				for (auto j = -grid.dim / 2; j < grid.dim / 2; ++j) {
					if ((i + j) % 2 == 0)
						continue;
					triangle.setUniform<gl::glUniform2fv>("vTranslation", grid.toDeviceCoordinates(glm::ivec2{ i, j }));
					triangle.setUniform<gl::glUniform4fv>("fSquareColor", glm::vec4{ .7, .7, .7, 1 } * 1.2f);
					state.draw();
				}
			}
			triangle.setUniform<gl::glUniform2fv>("vTranslation", grid.toDeviceCoordinates(glm::ivec2{ 0, 0 }));
			triangle.setUniform<gl::glUniform4fv>("fSquareColor", glm::vec4{ .7, .7, .7, 1 } * 5.f);
			state.draw();
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