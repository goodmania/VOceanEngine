#include "PreCompileHeader.h"
#include "Renderer/HeightMap/HeightMap.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace voe {

	HeightMap::HeightMap(Device& device, VkQueue copyQueue)
		: m_Device(device), m_CopyQueue(copyQueue)
	{

	}

	HeightMap::~HeightMap()
	{
		vkDestroyBuffer(m_Device.GetVkDevice(), m_VertexBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_VertexBufferMemory, nullptr);

		if (m_HasIndexBuffer)
		{
			vkDestroyBuffer(m_Device.GetVkDevice(), m_IndexBuffer, nullptr);
			vkFreeMemory(m_Device.GetVkDevice(), m_IndexBufferMemory, nullptr);
		}
	}

	float HeightMap::GetHeight(uint32_t x, uint32_t y)
	{
		glm::ivec2 rpos = glm::ivec2(x, y) * glm::ivec2(m_Scale);
		rpos.x = std::max(0, std::min(rpos.x, (int)m_Dim - 1));
		rpos.y = std::max(0, std::min(rpos.y, (int)m_Dim - 1));
		rpos /= glm::ivec2(m_Scale);
		return *(m_Heightdata + (rpos.x + rpos.y * m_Dim) * m_Scale) / 65535.0f * m_HeightScale;
	}

	void HeightMap::LoadFromFile(const std::string filepath, uint32_t patchsize, glm::vec3 scale, Topology topology)
	{
		VOE_CORE_ASSERT(device);
		VOE_CORE_ASSERT(m_CopyQueue != nullptr);

		int32_t texWidth, texHeight, texChannels;

		stbi_uc* stbImage = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!stbImage)
		{
			throw std::runtime_error("failed to load texture image!");
		}

		m_Dim = texWidth; // texture base width.
		m_Heightdata = new uint16_t[m_Dim * m_Dim];
		memcpy(m_Heightdata, stbImage, imageSize);
		m_Scale = m_Dim / patchsize;
		stbi_image_free(stbImage);

		// Generate vertices (remind -- dont forget free vertices)
		Vertex* vertices = new Vertex[patchsize * patchsize * 4];

		const float wx = 2.0f;
		const float wy = 2.0f;

		for (uint32_t x = 0; x < patchsize; x++)
		{
			for (uint32_t y = 0; y < patchsize; y++)
			{
				uint32_t index = (x + y * patchsize);
				vertices[index].position[0] = (x * wx + wx / 2.0f - (float)patchsize * wx / 2.0f) * scale.x;
				vertices[index].position[1] = -GetHeight(x, y);
				vertices[index].position[2] = (y * wy + wy / 2.0f - (float)patchsize * wy / 2.0f) * scale.z;
				vertices[index].uv = glm::vec2((float)x / patchsize, (float)y / patchsize) * m_UvScale;
			}
		}

		for (uint32_t y = 0; y < patchsize; y++)
		{
			for (uint32_t x = 0; x < patchsize; x++)
			{
				float dx = GetHeight(x < patchsize - 1 ? x + 1 : x, y) - GetHeight(x > 0 ? x - 1 : x, y);
				if (x == 0 || x == patchsize - 1)
					dx *= 2.0f;

				float dy = GetHeight(x, y < patchsize - 1 ? y + 1 : y) - GetHeight(x, y > 0 ? y - 1 : y);
				if (y == 0 || y == patchsize - 1)
					dy *= 2.0f;

				glm::vec3 A = glm::vec3(1.0f, 0.0f, dx);
				glm::vec3 B = glm::vec3(0.0f, 1.0f, dy);

				glm::vec3 normal = (glm::normalize(glm::cross(A, B)) + 1.0f) * 0.5f;

				vertices[x + y * patchsize].normal = glm::vec3(normal.x, normal.z, normal.y);
			}
		}

		// Generate indices
		const uint32_t w = (patchsize - 1);
		uint32_t* indices;

		switch (topology)
		{
			// Indices for triangles
		case Triangles:
		{
			indices = new uint32_t[w * w * 6];
			for (uint32_t x = 0; x < w; x++)
			{
				for (uint32_t y = 0; y < w; y++)
				{
					uint32_t index = (x + y * w) * 6;
					indices[index] = (x + y * patchsize);
					indices[index + 1] = indices[index] + patchsize;
					indices[index + 2] = indices[index + 1] + 1;

					indices[index + 3] = indices[index + 1] + 1;
					indices[index + 4] = indices[index] + 1;
					indices[index + 5] = indices[index];
				}
			}
			m_IndexCount = (patchsize - 1) * (patchsize - 1) * 6;
			m_IndexBufferSize = (w * w * 6) * sizeof(uint32_t);
			break;
		}

		// Indices for quad patches (tessellation)
		case Quads:
		{
			indices = new uint32_t[w * w * 4];
			for (uint32_t x = 0; x < w; x++)
			{
				for (uint32_t y = 0; y < w; y++)
				{
					uint32_t index = (x + y * w) * 4;
					indices[index] = (x + y * patchsize);
					indices[index + 1] = indices[index] + patchsize;
					indices[index + 2] = indices[index + 1] + 1;
					indices[index + 3] = indices[index] + 1;
				}
			}
			m_IndexCount = (patchsize - 1) * (patchsize - 1) * 4;
			m_IndexBufferSize = (w * w * 4) * sizeof(uint32_t);
			break;
		}
		}

		VOE_CORE_ASSERT(m_IndexBufferSize > 0);

		m_VertexBufferSize = (patchsize * patchsize * 4) * sizeof(Vertex);

		// create vertex buffers
		VkBuffer stagingVertexBuffer;
		VkDeviceMemory stagingVertexBufferMemory;

		m_Device.CreateBuffer(
			m_VertexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingVertexBuffer,
			stagingVertexBufferMemory);

		void* vdata;
		vkMapMemory(m_Device.GetVkDevice(), stagingVertexBufferMemory, 0, m_VertexBufferSize, 0, &vdata);
		memcpy(vdata, vertices, static_cast<size_t>(m_VertexBufferSize));
		vkUnmapMemory(m_Device.GetVkDevice(), stagingVertexBufferMemory);

		m_Device.CreateBuffer(
			m_VertexBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_VertexBuffer,
			m_VertexBufferMemory);

		m_Device.CopyBuffer(stagingVertexBuffer, m_VertexBuffer, m_VertexBufferSize);

		vkDestroyBuffer(m_Device.GetVkDevice(), stagingVertexBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), stagingVertexBufferMemory, nullptr);

		// create index buffers 
		VkBuffer stagingIndexBuffer;
		VkDeviceMemory stagingIndexBufferMemory;

		m_Device.CreateBuffer(
			m_IndexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingIndexBuffer,
			stagingIndexBufferMemory);

		void* idata;
		vkMapMemory(m_Device.GetVkDevice(), stagingIndexBufferMemory, 0, m_IndexBufferSize, 0, &idata);
		memcpy(idata, vertices, static_cast<size_t>(m_IndexBufferSize));
		vkUnmapMemory(m_Device.GetVkDevice(), stagingIndexBufferMemory);

		m_Device.CreateBuffer(
			m_IndexBufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_IndexBuffer,
			m_IndexBufferMemory);

		m_Device.CopyBuffer(stagingIndexBuffer, m_IndexBuffer, m_IndexBufferSize);

		vkDestroyBuffer(m_Device.GetVkDevice(), stagingIndexBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), stagingIndexBufferMemory, nullptr);
	}
}


