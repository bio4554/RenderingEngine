#pragma once

#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace star::graphics
{
	struct Vertex
	{
		Vertex() = default;
		Vertex(const float x, const float y, const float z, const float r, const float g, const float b) : position(x, y, z), normal(), tangent(), biTangent(), color(r, g, b, 1.f), uv()
		{

		}

		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 biTangent;
		glm::vec4 color;
		glm::vec2 uv;
	};

	struct Meshlet
	{
		uint32_t vertexCount;
		uint32_t triangleCount;
		uint32_t vertexIndices[64];
		uint32_t triangleIndices[124 * 3];
	};
}
