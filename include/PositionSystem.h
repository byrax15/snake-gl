#pragma once

#include <glm/vec2.hpp>
#include <array>

using Position = glm::vec2;

inline constexpr std::array positions{
	Position{ .1, .2 },
	Position{ -.2, -.2 },
	Position{ -.1, .2 },
	Position{ -.5, -.5 },
};