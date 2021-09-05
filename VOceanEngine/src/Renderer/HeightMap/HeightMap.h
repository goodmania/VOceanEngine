#pragma once

#include "VulkanCore/Device.h"

namespace voe
{
	class HeightMap
	{
	public:
		struct Ocean
		{
			glm::vec4 Pos;
			glm::vec4 UV;
			glm::vec4 Normal;
		};

		struct StorageBuffers
		{
			VkBuffer InputBuffer = VK_NULL_HANDLE;
			VkDeviceMemory InputMemory = VK_NULL_HANDLE;
			mutable VkDescriptorBufferInfo* InputBufferDscInfo = VK_NULL_HANDLE;
			VkBuffer OutputBuffer = VK_NULL_HANDLE;
			VkDeviceMemory OutputMemory = VK_NULL_HANDLE;
			mutable VkDescriptorBufferInfo* OutputBufferDscInfo = VK_NULL_HANDLE;
		};

		HeightMap(Device& device, const VkQueue& copyQueue);
		~HeightMap();

		void CreateHeightMap(uint32_t gridsize);
		void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer);
		const StorageBuffers GetStorageBuffers() const { return m_StorageBuffers; }
		
	private:
		void SetDescriptorBufferInfo(VkDescriptorBufferInfo* info, VkBuffer buffer, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		uint32_t m_IndexCount;
		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;

		Device& m_Device;
		const VkQueue& m_CopyComputeQueue;

		StorageBuffers m_StorageBuffers;

		VkBuffer m_UniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_UniformBufferMemory = VK_NULL_HANDLE;
	};
}