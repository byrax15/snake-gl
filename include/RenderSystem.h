#pragma once

#include <glm/vec4.hpp>

struct Renderer {
	glm::vec4 color;

	static constexpr auto tailColor() { return Renderer{ { 1, 1, .7, 1 } }; };
	static constexpr auto red() { return Renderer{ { 1, 0, 0, 1 } }; };

	static constexpr auto random(const auto& colorGenerator) {
		Renderer r{ tailColor() };
		r.color *= colorGenerator();
		return r;
	}
};