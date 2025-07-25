#pragma once

#include <array>
#include <filesystem>
#include <iostream>
#include <vector>

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext/vector_float3.hpp"

#include "tiny_obj_loader.h"

#include "worldobject.hpp"

namespace tracer
{

// unused
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;

	Vertex(const glm::vec3 position, const glm::vec3 color, const glm::vec3 normal)
	    : position(position), color(color), normal(normal)
	{
	}

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}
};

// unused
class MeshObject : public WorldObject
{
  public:
	static MeshObject loadFromPath(std::filesystem::path filepath)
	{
		uint32_t primitiveCount;
		std::vector<float> vertices;
		std::vector<uint32_t> indices;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		loadOBJScene(filepath, primitiveCount, vertices, indices, shapes, materials);

		return MeshObject(glm::vec3{0, 0, 0}, primitiveCount, vertices, indices, shapes, materials);
	}

	~MeshObject() = default;
	MeshObject(const MeshObject&) = default;
	MeshObject& operator=(const MeshObject&) = delete;

	MeshObject(MeshObject&&) = default;
	MeshObject& operator=(MeshObject&&) = delete;

	void cleanup([[maybe_unused]] VkDevice device) const
	{
		// vkDestroyBuffer(device, indexBuffer, nullptr);
		// vkFreeMemory(device, indexBufferMemory, nullptr);

		// vkDestroyBuffer(device, vertexBuffer, nullptr);
		// vkFreeMemory(device, vertexBufferMemory, nullptr);

		// vkDestroyBuffer(device, normalBuffer, nullptr);
		// vkFreeMemory(device, normalBufferMemory, nullptr);
	}

	std::vector<glm::vec3> getNormals() const
	{
		return normals;
	}

  private:
	MeshObject(const glm::vec3& position,
	           const uint32_t primitiveCount,
	           const std::vector<float>& vertices,
	           const std::vector<uint32_t>& indices,
	           const std::vector<tinyobj::shape_t>& shapes,
	           const std::vector<tinyobj::material_t>& materials)
	    : WorldObject(position), primitiveCount(primitiveCount), vertices(vertices),
	      indices(indices), shapes(shapes), materials(materials) {};

	inline static void loadOBJScene(std::filesystem::path scenePath,
	                                uint32_t& primitiveCount,
	                                std::vector<float>& vertices,
	                                std::vector<uint32_t>& indexList,
	                                std::vector<tinyobj::shape_t>& shapes,
	                                std::vector<tinyobj::material_t>& materials)
	{
		// std::filesystem::path arrowPath = "3d-models/up_arrow.obj";
		// auto loaded_model = loadModel(arrowPath);

		// =========================================================================
		// OBJ Model

		tinyobj::ObjReaderConfig reader_config;
		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(scenePath.generic_string(), reader_config))
		{
			if (!reader.Error().empty())
			{
				std::cerr << "TinyObjReader: " << reader.Error();
			}
			exit(1);
		}

		if (!reader.Warning().empty())
		{
			std::cout << "TinyObjReader: " << reader.Warning();
		}

		const tinyobj::attrib_t& attrib = reader.GetAttrib();
		vertices = attrib.vertices;

		shapes = reader.GetShapes();
		materials = reader.GetMaterials();

		primitiveCount = 0;
		for (tinyobj::shape_t shape : shapes)
		{

			primitiveCount += static_cast<uint32_t>(shape.mesh.num_face_vertices.size());

			for (tinyobj::index_t index : shape.mesh.indices)
			{
				indexList.push_back(static_cast<uint32_t>(index.vertex_index));
			}
		}
	}

  private:
	const std::vector<glm::vec3> normals{};

	[[maybe_unused]] const uint32_t primitiveCount{};
	const std::vector<float> vertices{};
	const std::vector<uint32_t> indices{};
	const std::vector<tinyobj::shape_t> shapes{};
	const std::vector<tinyobj::material_t> materials{};

	// VkBuffer vertexBufferHandle = VK_NULL_HANDLE;
	// VkDeviceMemory vertexBufferDeviceMemoryHandle{};
	// VkDeviceAddress vertexBufferDeviceAddress{};

	// VkBuffer indexBufferHandle = VK_NULL_HANDLE;
	// VkDeviceMemory indexBufferDeviceMemoryHandle{};
	// VkDeviceAddress indexBufferDeviceAddress{};

	// VkBuffer normalBuffer;
	// VkDeviceMemory normalBufferMemory;

	// VkBuffer uniformBufferHandle;
};

} // namespace tracer
