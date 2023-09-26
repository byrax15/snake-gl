// snake-gl.cpp : Defines the entry point for the application.
//

#include <span>
#include <iostream>
#include <optional>
#include <random>

#include <flecs.h>
#include <SFML/Window.hpp>
#include <glbinding/glbinding.h>
#include <glbinding/gl45core/gl.h>

#include "Shader.h"
#include "GLstate.h"
#include "PositionSystem.h"
#include "RenderSystem.h"
#include "GrowthSystem.h"
#include "Random.h"


inline constexpr Grid	   grid{ 24 };
inline constexpr auto	   squareSizeHalf = 1 / static_cast<gl::GLfloat>(grid.dim) / 2;
const Random::Distribution randColor{ std::uniform_real_distribution<float>{ .8, 1.5 } };
const Random::Distribution randGrid{ std::uniform_int_distribution<int>{ -grid.dim / 2, grid.dim / 2 - 1 } };

inline IndexedModel square{
	{
		glm::vec3{ squareSizeHalf, squareSizeHalf, 0. },
		glm::vec3{ -squareSizeHalf, squareSizeHalf, 0. },
		glm::vec3{ -squareSizeHalf, -squareSizeHalf, 0. },
		glm::vec3{ squareSizeHalf, -squareSizeHalf, 0. },
	},
	{
		glm::uvec3{ 0, 1, 2 },
		glm::uvec3{ 0, 2, 3 },
	}
};

struct TriangleShader : Shader {
	gl::GLint vTranslationLocation, fSquareColorLocation;

	TriangleShader() : Shader{ "triangle.vert", "triangle.frag" } {
		using gl::glGetUniformLocation;

		const auto getUniform = [](auto ID, std::string_view name) {
			const auto location = glGetUniformLocation(ID, name.data());
			if (location < 0) {
				std::cerr << "Invalid uniform name : " << name << "\n";
				throw;
			}
			return location;
		};

		vTranslationLocation = getUniform(ID, "vTranslation");
		fSquareColorLocation = getUniform(ID, "fSquareColor");
	}
};

int main() {
	flecs::world ecs;

	sf::ContextSettings settings;
	settings.majorVersion = 4;
	settings.minorVersion = 5;
	sf::Window window(sf::VideoMode(800, 600), "SnakeGL", sf::Style::Default, settings);
	window.setActive();
	glbinding::initialize(sf::Context::getFunction);

	const TriangleShader triangle;
	triangle.use();
	GLstate state{ std::move(square) };

	auto head  = Head::create(ecs.entity("SnakeHead"), ecs, Position::zero(), Velocity::zero(), Renderer::headColor());
	auto tail0 = Tail::create(ecs.entity("Tail0"), ecs, Position{ { 0, -1 } }, Velocity::zero(), Renderer::tailColor());
	auto tail1 = Tail::create(ecs.entity("Tail1"), ecs, Position{ { 0, -2 } }, Velocity::zero(), Renderer::tailColor());

	auto snake = ecs.entity("Snake");
	snake.set<Snake>({ head, { tail0, tail1 } });

	Apple::create(ecs.entity("StartApple"), ecs, Position::random(randGrid), Velocity::zero(), Renderer::red());

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
					ecs.entity(), ecs, snakeStruct.freeEndPosition(), Velocity::zero(), Renderer::tailColor());
				snakeStruct.tail.push_back(newTail);
				ateApple = false;
			}

			for (auto it{ snakeStruct.tail.end() - 1 }; it > snakeStruct.tail.begin(); --it) {
				it->get_mut<Position>()->vec = (it - 1)->get<Position>()->vec;
			}
			snakeStruct.tail[0].get_mut<Position>()->vec = snakeStruct.head.get<Position>()->vec;

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
			gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

			for (auto i = -grid.dim / 2; i < grid.dim / 2; ++i) {
				for (auto j = -grid.dim / 2; j < grid.dim / 2; ++j) {
					if ((i + j) % 2 == 0)
						continue;
					gl::glUniform2fv(triangle.vTranslationLocation, 1, glm::value_ptr(grid.toDeviceCoordinates(glm::ivec2{ i, j })));
					gl::glUniform4fv(triangle.fSquareColorLocation, 1, glm::value_ptr(Renderer::gridColor() * 1.2f));
					state.draw();
				}
			}
		});

	const auto makeDrawFunc = [&]<typename Tag>() {
		return [&](const Position& position, const Renderer& renderer, const Tag) {
			// draw...
			gl::glUniform2fv(triangle.vTranslationLocation, 1, glm::value_ptr(grid.toDeviceCoordinates(position.vec)));
			gl::glUniform4fv(triangle.fSquareColorLocation, 1, glm::value_ptr(renderer.color));
			state.draw();
		};
	};
	makeDrawSystem<Tail>(ecs, "DrawTail", makeDrawFunc.operator()<Tail>());
	makeDrawSystem<Apple>(ecs, "DrawApple", makeDrawFunc.operator()<Apple>());
	makeDrawSystem<Head>(ecs, "DrawHead", makeDrawFunc.operator()<Head>());

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