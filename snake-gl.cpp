// snake-gl.cpp : Defines the entry point for the application.
//

#include <flecs.h>
#include <SFML/Window.hpp>

#include <glbinding/glbinding.h>
#include <glbinding/gl45core/gl.h>

#include <globjects/globjects.h>
#include <globjects/base/File.h>
#include <globjects/base/StaticStringSource.h>

#include <glm/glm.hpp>

#include <span>
#include <iostream>

template <bool IndexedRendering>
struct GLstate {
	gl::GLuint vao;
	gl::GLuint vbo;
	gl::GLuint ebo;
	gl::GLuint trianglesCount;
	gl::GLuint indicesCount;

	explicit GLstate(
		const std::span<const glm::vec3>  vertices,
		const std::span<const glm::uvec3> indices)
		: trianglesCount(vertices.size()),
		  indicesCount(indices.size() * glm::uvec3::length()) {

		using namespace gl;
		glEnable(gl::GL_DEPTH_TEST);

		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);
		// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size_bytes(), vertices.data(), GL_STATIC_DRAW);

		if constexpr (IndexedRendering) {
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size_bytes(), indices.data(), GL_STATIC_DRAW);
		}

		glVertexAttribPointer(0, glm::vec3::length(), GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glEnableVertexAttribArray(0);
	}

	~GLstate() {
		using namespace gl;
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		if constexpr (IndexedRendering)
			glDeleteBuffers(1, &ebo);
	}

	auto draw() const -> void {
		using namespace gl;
		glBindVertexArray(vao);
		if constexpr (IndexedRendering) {
			glDrawElements(GL_TRIANGLES, indicesCount, GL_UNSIGNED_INT, 0);
		}
		else {
			glDrawArrays(GL_TRIANGLES, 0, trianglesCount);
		}
	}
};

auto makeShader(const std::string& vsName, const std::string& fsName) {
	auto vsSrc = globjects::Shader::sourceFromFile(vsName);
	auto vs	   = globjects::Shader::create(gl::GL_VERTEX_SHADER, vsSrc.get());

	auto fsSrc = globjects::Shader::sourceFromFile(fsName);
	auto fs	   = globjects::Shader::create(gl::GL_FRAGMENT_SHADER, fsSrc.get());

	auto program = globjects::Program::create();
	program->attach(vs.get(), fs.get());
	program->use();

	return program;
}

inline constexpr std::array positions{
	glm::vec3{ 0.05, 0.05, 0. },
	glm::vec3{ -0.05, 0.05, 0. },
	glm::vec3{ -0.05, -0.05, 0. },
	glm::vec3{ 0.05, -0.05, 0. },
};

inline constexpr std::array indices{
	glm::uvec3{ 0, 1, 2 },
	glm::uvec3{ 0, 2, 3 },
};

int main(int argc, char* argv[]) {
	sf::ContextSettings settings;
	settings.majorVersion = 4;
	settings.minorVersion = 5;
	sf::Window window(sf::VideoMode(800, 600), "OpenGL", sf::Style::Default, settings);
	window.setVerticalSyncEnabled(true);
	window.setActive();

	globjects::init(sf::Context::getFunction);

	GLstate<true> state{
		std::span{ positions },
		std::span{ indices },
	};

	auto triangle = makeShader("triangle.vert", "triangle.frag");

	bool running = true;
	while (running) {
		// handle events
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				// end the program
				running = false;
			}
			else if (event.type == sf::Event::Resized) {
				// adjust the viewport when the window is resized
				gl::glViewport(0, 0, event.size.width, event.size.height);
			}
		}

		// clear the buffers
		gl::glClearColor(0, .1, .05, 1);
		gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

		// draw...
		state.draw();

		// end the current frame (internally swaps the front and back buffers)
		window.display();
	}
}
