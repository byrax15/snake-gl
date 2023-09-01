#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>

#include <glbinding/gl45core/types.h>
#include <glm/vec2.hpp>

struct Player {};

struct Position {
	glm::ivec2 vec;
};

struct Velocity {
	glm::ivec2 vec;
};

struct Grid {
	gl::GLint dim;

	constexpr Grid(gl::GLint dim) : dim(dim) {
		if (dim % 2 != 0)
			throw "Grid must have an even size";
	}

	[[nodiscard]] constexpr auto toDeviceCoordinates(const glm::ivec2 position) const noexcept {
		return glm::vec2{
			static_cast<gl::GLfloat>(position.x) / static_cast<gl::GLfloat>(dim),
			static_cast<gl::GLfloat>(position.y) / static_cast<gl::GLfloat>(dim)
		};
	}

	constexpr auto clampIn(glm::ivec2& position) const noexcept {
		position.x = std::clamp(position.x, -dim / 2, dim / 2 - 1);
		position.y = std::clamp(position.y, -dim / 2, dim / 2 - 1);
	};
};
