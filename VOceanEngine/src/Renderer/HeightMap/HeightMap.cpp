#include "PreCompileHeader.h"
#include "Renderer/HeightMap/HeightMap.h"

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

	void HeightMap::LoadFromFile(const std::string filename, uint32_t patchsize, glm::vec3 scale, Topology topology)
	{

	}
}


