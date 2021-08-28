#pragma once
/*
* Heightmap terrain generator
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanCore/Device.h"

namespace voe
{
	class HeightMap
	{
	public:
		struct Vertex
		{
			glm::vec3 position{};
			glm::vec3 normal{};
			glm::vec2 uv{};
		};

		enum Topology { Triangles, Quads };

		HeightMap(Device& device, VkQueue copyQueue);
		~HeightMap();

		float GetHeight(uint32_t x, uint32_t y);
		void LoadFromFile(const std::string filename, uint32_t patchsize, glm::vec3 scale, Topology topology);

	private:
		uint16_t* m_Heightdata;
		uint32_t m_Dim;
		uint32_t m_Scale;

		float m_HeightScale = 1.0f;
		float m_UvScale = 1.0f;

		Device& m_Device;
		VkQueue& m_CopyQueue;

		VkDeviceSize m_VertexBufferSize = 0;
		VkDeviceSize m_IndexBufferSize = 0;
		uint32_t m_IndexCount = 0;

		// vertex and index buffer.
		VkBuffer m_VertexBuffer;
		VkDeviceMemory m_VertexBufferMemory;
		uint32_t m_VertexCount;

		bool m_HasIndexBuffer = true;
		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;
	};
}