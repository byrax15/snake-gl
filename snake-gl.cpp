// snake-gl.cpp : Defines the entry point for the application.
//

#include <span>
#include <iostream>
#include <random>
#include <ranges>

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


inline constexpr Grid grid{ 24 };
inline constexpr auto squareSizeHalf = 1 / static_cast<gl::GLfloat>(grid.dim) / 2;
inline IndexedModel	  square{
	  std::vector{
		  glm::vec3{ squareSizeHalf, squareSizeHalf, 0. },
		  glm::vec3{ -squareSizeHalf, squareSizeHalf, 0. },
		  glm::vec3{ -squareSizeHalf, -squareSizeHalf, 0. },
		  glm::vec3{ squareSizeHalf, -squareSizeHalf, 0. },
	  },
	  std::vector{
		  glm::uvec3{ 0, 1, 2 },
		  glm::uvec3{ 0, 2, 3 },
	  }
};
const Random::Distribution randColor{ std::uniform_real_distribution<float>{ 1.1f, 1.5f } };
const Random::Distribution randGrid{ std::uniform_int_distribution<int>{ -grid.dim / 2, grid.dim / 2 - 1 } };

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
struct GameLoop {};
struct RenderSystem {};

struct GameState {
public:
	enum class State {
		Running,
		Paused,
		Restarting,
	};

	bool changed = false;
	bool startMove = false;

private:
	State s{ State::Running };

public:
	[[nodiscard]] auto state() const noexcept { return s; }

	auto pause() noexcept {
		if (s == State::Paused)
			return;

		s		= State::Paused;
		changed = true;
	}

	auto restart() noexcept {
		if (s == State::Restarting)
			return;

		s		= State::Restarting;
		changed = true;
	}

	auto resume() noexcept {
		if (s == State::Running)
			return;

		s		= State::Running;
		changed = true;
	}
};

int main() {
	sf::ContextSettings settings;
	settings.majorVersion = 4;
	settings.minorVersion = 5;
	sf::Window window(sf::VideoMode(800, 600), "SnakeGL", sf::Style::Default, settings);
	window.setActive();
	glbinding::initialize(sf::Context::getFunction);

	const TriangleShader triangle;
	triangle.use();

	GLstate		 glstate{ std::move(square) };
	flecs::world ecs;
	auto		 RenderPhase = ecs.entity().add(flecs::Phase).depends_on(flecs::PostUpdate);
	auto		 GamePhase	 = ecs.entity().add(flecs::Phase).depends_on(flecs::OnUpdate);

	Snake::addToWorld(ecs, randColor);
	Apple::create(ecs.entity("StartApple"), ecs, Position::random(randGrid), Velocity::zero(), Renderer::red());
	auto gameState = ecs.entity("GameState").add<GameState>();

	const auto headRenderers  = ecs.query<const Position, const Renderer, const Head>("HeadRenderersQuery");
	const auto appleRenderers = ecs.query<const Position, const Renderer, const Apple>("AppleRenderersQuery");
	const auto tailRenderers  = ecs.query<const Position, const Renderer, const Tail>("TailRenderersQuery");
	const auto tailPositions  = ecs.query<const Tail, const Position>("TailPositionsQuery");
	const auto headPositions  = ecs.query<const Head, const Position>("HeadPositionQuery");
	const auto headVelocity	  = ecs.query<const Head, Velocity>("HeadVelocityQuery");
	const auto gameSystems	  = ecs.query<const GameLoop>("GameLoopSystemsQuery");
	const auto snakeHeads	  = ecs.query<const Head>("SnakeHeadQuery");

	ecs.system<const Head, Velocity>("ProcessInput")
		.kind(flecs::PreUpdate)
		.iter([&](flecs::iter&) {
			auto* headVel = headVelocity.first().get_mut<Velocity>();

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
					case sf::Keyboard::I:
						gameState.get_mut<GameState>()->restart();
						break;
					case sf::Keyboard::W:
						headVel->vec = { 0, 1 };
						break;
					case sf::Keyboard::S:
						headVel->vec = { 0, -1 };
						break;
					case sf::Keyboard::A:
						headVel->vec = { -1, 0 };
						break;
					case sf::Keyboard::D:
						headVel->vec = { 1, 0 };
						break;
					default:
						break;
					}
				}
			}
		});


	ecs.system("GameStateSystem")
		.kind(flecs::PostUpdate)
		.iter([&](flecs::iter&) {
			auto* game = gameState.get_mut<GameState>();
			if (!game->changed)
				return;
			switch (game->state()) {
			case GameState::State::Paused:
				GamePhase.disable();
				break;
			case GameState::State::Restarting:
				Snake::reset(snakeHeads.first(), ecs, randColor);
				GamePhase.enable();
				game->startMove = false;
				game->resume();
				break;
			case GameState::State::Running:
				game->changed = false;
				break;
			}
		});

	ecs.system("RenderBackground")
		.kind(RenderPhase)
		.iter([&](flecs::iter&) {
			// clear the buffers
			gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

			// draw background
			for (auto i = -grid.dim / 2; i < grid.dim / 2; ++i) {
				for (auto j = -grid.dim / 2; j < grid.dim / 2; ++j) {
					if ((i + j) % 2 == 0)
						continue;
					gl::glUniform2fv(triangle.vTranslationLocation, 1, glm::value_ptr(grid.toDeviceCoordinates(glm::ivec2{ i, j })));
					gl::glUniform4fv(triangle.fSquareColorLocation, 1, glm::value_ptr(Renderer::gridColor() * 1.2f));
					glstate.draw();
				}
			}
		});
	ecs.system("RenderObjects")
		.kind(RenderPhase)
		.iter([&](flecs::iter&) {
			// draw...
			const auto drawFunc = [&](const Position& position, const Renderer& renderer) {
				gl::glUniform2fv(triangle.vTranslationLocation, 1, glm::value_ptr(grid.toDeviceCoordinates(position.vec)));
				gl::glUniform4fv(triangle.fSquareColorLocation, 1, glm::value_ptr(renderer.color));
				glstate.draw();
			};
			tailRenderers.each([&](const Position& p, const Renderer& r, const Tail) { drawFunc(p, r); });
			appleRenderers.each([&](const Position& p, const Renderer& r, const Apple) { drawFunc(p, r); });
			headRenderers.each([&](const Position& p, const Renderer& r, const Head) { drawFunc(p, r); });

			// display
			window.display();
		});


	bool ateApple  = false;
	ecs.system<const Head, Position, const Velocity>("MoveSnake")
		.kind(GamePhase)
		.each([&](flecs::entity head, const Head, Position& p, const Velocity& v) {
			std::vector<flecs::entity> nodes{ head };
			const auto				   getSnakeNodes = [&]() {
				while (true) {
					auto size = nodes.size();
					(nodes.end() - 1)->children([&](flecs::entity child) { nodes.push_back(child); });
					auto newSize = nodes.size();
					if (size == newSize)
						break;
				}
			};

			auto* state = gameState.get_mut<GameState>();
			if (ateApple) {
				auto newTail = Tail::create(ecs.entity(), ecs, *head.get<Position>(), Velocity::zero(), Renderer::random(randColor));
				getSnakeNodes();
				Snake::addTail(*(nodes.end() - 1), newTail);
				ateApple = false;
				goto GET_POSITIONS;
			}

			if (state->startMove) {
				getSnakeNodes();
			GET_POSITIONS:
				std::vector<Position*> positions;
				std::transform(nodes.begin(), nodes.end(), std::back_inserter(positions), [](flecs::entity n) { return n.get_mut<Position>(); });
				for (auto it{ positions.end() - 1 }; it > positions.begin(); --it) {
					(*it)->vec = (*(it - 1))->vec;
				}

				p.vec += v.vec;
				if (grid.outOfBounds(p.vec))
					gameState.get_mut<GameState>()->pause();
				else
					tailPositions.each([&](const Tail, const Position& tailP) {
						if (p.vec == tailP.vec)
							gameState.get_mut<GameState>()->pause();
					});
			}
			state->startMove = v.vec.x != 0 || v.vec.y != 0;
		});

	ecs.system<const Apple, const Position>("EatApple")
		.kind(GamePhase)
		.each([&](flecs::entity e, const Apple, const Position& aPos) {
			headPositions.each([&](const Head, const Position& pPos) {
				if (aPos.vec == pPos.vec) {
					Apple::create(ecs.entity(), ecs, Position::random(randGrid), Velocity::zero(), Renderer::red());
					e.destruct();
					ateApple = true;
				}
			});
		});

	return ecs.app()
		.target_fps(10)
		.enable_rest()
		//.enable_monitor()
		.run();
}