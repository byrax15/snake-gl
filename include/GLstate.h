#pragma once

#include <span>
#include <glbinding/gl45core/gl.h>
#include <glm/glm.hpp>


struct BindBuffer {
	gl::GLenum	   target, usage;
	gl::GLsizeiptr size_bytes;
	void*		   data;

	explicit constexpr BindBuffer(gl::GLenum target, gl::GLenum usage, const auto& container)
		: target(target),
		  usage(usage),
		  size_bytes(container.size() * sizeof(decltype(container[0]))),
		  data((void*)&container[0]) {}
};

struct VertexAttribPointer {
	gl::GLuint	  index;
	gl::GLint	  size;
	gl::GLenum	  type;
	gl::GLboolean normalized;
	gl::GLsizei	  stride;
	void*		  pointer;
};

struct IndexedModel {
	std::vector<glm::vec3>	vertices;
	std::vector<glm::uvec3> indices;

	[[nodiscard]] auto indexCount() const noexcept -> gl::GLsizei {
		return static_cast<gl::GLsizei>(indices.size()) * static_cast<gl::GLsizei>(std::remove_cvref_t<decltype(indices[0])>::length());
	}
	[[nodiscard]] constexpr auto vertexData() const noexcept {
		return BindBuffer{ gl::GL_ARRAY_BUFFER, gl::GL_STATIC_DRAW, vertices };
	}
	[[nodiscard]] constexpr auto indexData() const noexcept {
		return BindBuffer{ gl::GL_ELEMENT_ARRAY_BUFFER, gl::GL_STATIC_DRAW, indices };
	}
	[[nodiscard]] constexpr auto vertexPointer() const noexcept {
		return VertexAttribPointer{
			0,
			std::remove_cvref_t<decltype(vertices[0])>::length(),
			gl::GL_FLOAT,
			gl::GL_FALSE,
			sizeof(decltype(vertices[0])),
			(void*)0
		};
	}
};

struct GLstate {
	gl::GLuint	 vao = 0, vbo = 0, ebo = 0;
	IndexedModel model;

	explicit GLstate(IndexedModel&& model) : model(model) {
		using namespace gl;
		gl::glClearDepth(1.f);
		gl::glClearColor(.7, .7, .7, 1);
		gl::glEnable(gl::GL_DEPTH_TEST);


		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);
		// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(vao);

		constexpr auto bindBuffer = [](GLuint& buffer, BindBuffer&& bufferData) {
			const auto [target, usage, sizeBytes, data] = bufferData;
			glBindBuffer(target, buffer);
			glBufferData(target, sizeBytes, data, usage);
		};

		constexpr auto enableVertexAttrib = [](VertexAttribPointer&& pointerData) {
			const auto [index, size, type, normalized, stride, pointer] = pointerData;
			glVertexAttribPointer(index, size, type, normalized, stride, pointer);
			glEnableVertexAttribArray(index);
		};

		bindBuffer(vbo, model.vertexData());
		bindBuffer(ebo, model.indexData());
		enableVertexAttrib(model.vertexPointer());
	}

	auto draw() const -> void {
		using namespace gl;
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, model.indexCount(), GL_UNSIGNED_INT, nullptr);
	}

	~GLstate() {
		using namespace gl;
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
};
