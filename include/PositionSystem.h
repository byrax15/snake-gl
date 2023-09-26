#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>

#include <glbinding/gl45core/types.h>
#include <glm/vec2.hpp>

struct Position {
	glm::ivec2			  vec;
	static constexpr auto zero() { return Position{ { 0, 0 } }; }
	static constexpr auto random(const auto& generator) {
		return Position{ { generator(), generator() } };
	}
};

struct Velocity {
	glm::ivec2			  vec;
	static constexpr auto zero() { return Velocity{ { 0, 0 } }; }
};

struct Grid {
	gl::GLint dim;

	struct SizeException : std::exception {};

	explicit constexpr Grid(gl::GLint dim) : dim(dim) {
		if (dim % 2 != 0)
			throw SizeException();
	}

	[[nodiscard]] constexpr auto toDeviceCoordinates(const glm::ivec2 position) const noexcept {
		return glm::vec2{
			static_cast<gl::GLfloat>(position.x) / static_cast<gl::GLfloat>(dim),
			static_cast<gl::GLfloat>(position.y) / static_cast<gl::GLfloat>(dim)
		};
	}

	[[nodiscard]] constexpr auto outOfBounds(glm::ivec2 position) const noexcept {
		if (-dim / 2 > position.x || position.x > dim / 2 - 1)
			return true;
		if (-dim / 2 > position.y || position.y > dim / 2 - 1)
			return true;
		return false;
	}
};
