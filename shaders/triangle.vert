#version 450 core

in vec3 vPosition;

uniform float vZOffset;
uniform vec2 vTranslation;

void main() {
	gl_Position = vec4(vPosition.xy + vTranslation.xy , vZOffset, 1);
}