#pragma once

#include <glm/vec4.hpp>

struct Renderer {
	glm::vec4 color;

	static constexpr auto gridColor() { return glm::vec4{ .7, .7, .7, 1 }; };
	static constexpr auto headColor() { return Renderer{ { .3, .5, .3, 1 } }; };
	static constexpr auto tailColor() {
		auto r = headColor();
		r.color *= 1.3f;
		return r;
	}
	static constexpr auto red() { return Renderer{ { 1, 0, 0, 1 } }; };

	static constexpr auto random(const auto& colorGenerator) {
		Renderer r{ headColor() };
		r.color *= colorGenerator();
		return r;
	}
};
