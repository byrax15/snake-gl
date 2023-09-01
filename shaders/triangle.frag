#version 450 core

out vec4 finalColor;
uniform vec4 fSquareColor;

void main() {
	finalColor = fSquareColor;
}