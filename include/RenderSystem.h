#pragma once

#include <glm/vec4.hpp>

struct Renderer {
	glm::vec4	color;

	static constexpr auto gridColor() { return glm::vec4{ .7, .7, .7, 1 }; };
	static constexpr auto tailColor() { return Renderer{ { .5, .7, .5, 1 } }; };
	static constexpr auto headColor() { return Renderer{ { .3, .5, .3, 1 } }; };
	static constexpr auto red() { return Renderer{ { 1, 0, 0, 1 } }; };

	static constexpr auto random(const auto& colorGenerator) {
		Renderer r{ tailColor() };
		r.color *= colorGenerator();
		return r;
	}
};

template <typename Tag>
constexpr auto makeDrawSystem(flecs::world& ecs, std::string_view systemName, auto&& drawFunc) {
	ecs.system<const Position, const Renderer, const Tag>(systemName.data())
		.each(drawFunc);
}
