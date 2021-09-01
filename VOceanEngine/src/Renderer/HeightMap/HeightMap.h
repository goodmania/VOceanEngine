#pragma once

#include "VulkanCore/Device.h"

namespace voe
{
	class HeightMap
	{
	public:
		//struct OceanSize 
		//{
		//	glm::uvec2 gridsize = glm::uvec2(512, 512);
		//	glm::vec2  size = glm::vec2(5.0f);
		//} m_Ocean;

	public:
		struct Ocean
		{
			glm::vec4 Pos;
			glm::vec4 UV;
			glm::vec4 Normal;
		};

		struct StorageBuffers 
		{
			VkBuffer InputBuffer;
			VkDeviceMemory InputMemory;
			VkBuffer OutputBuffer;
			VkDeviceMemory OutputMemory;

		} m_StorageBuffers;

		struct Semaphores 
		{
			VkSemaphore Ready{ 0L };
			VkSemaphore Complete{ 0L };
		} m_Semaphores;

		HeightMap(Device& device, VkQueue copyQueue);
		~HeightMap();

		void  CreateHeightMapSSBO(uint32_t gridsize);

	private:
		uint32_t m_IndexCount;

		Device& m_Device;
		VkQueue& m_CopyQueue;

	};
}