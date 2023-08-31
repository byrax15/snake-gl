#include <string>

inline constexpr auto triangle_vert = R"(
#version 450 core

in vec2 vPosition;

void main() {
	gl_Position = vec4(vPosition.x, vPosition.y, 0, 1);
}
)";

inline constexpr auto triangle_frag = R"(
#version 450 core

out vec4 finalColor;

void main() {
	finalColor = vec4(1,0,0,1);
}
)";
