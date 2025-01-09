#pragma once

#include <array>
#include "glm/ext/vector_float3.hpp"
#include "src/worldobject.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ltracer
{

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

class MeshObject : public WorldObject
{
  public:
	MeshObject(const glm::vec3& position,
	           const std::vector<Vertex>& vertices,
	           const std::vector<unsigned int>& indices)
	    : WorldObject(position), vertices(vertices), indices(indices) {};

	MeshObject(MeshObject&&) = default;
	MeshObject(const MeshObject&) = delete;
	MeshObject& operator=(MeshObject&&) = delete;
	MeshObject& operator=(const MeshObject&) = delete;
	~MeshObject() = default;

	const std::vector<Vertex> vertices;
	const std::vector<uint32_t> indices;
	const std::vector<glm::vec3> normals;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	// VkBuffer normalBuffer;
	// VkDeviceMemory normalBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	void cleanup(VkDevice device) const
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
};

} // namespace ltracer
