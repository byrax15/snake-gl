#version 450 core

in vec3 vPosition;

uniform vec2 vTranslation;

void main() {
	vec2 transl = vPosition.xy + vTranslation.xy;
	gl_Position = vec4(transl.xy, vPosition.z, 1);
}