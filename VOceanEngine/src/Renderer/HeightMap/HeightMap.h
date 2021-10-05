#pragma once

#include "VulkanCore/Device.h"

namespace voe
{
	class VOE_API HeightMap
	{
	public:
		struct Ocean
		{
			glm::vec2 h;
			glm::vec4 Pos;
			glm::vec4 UV;
			glm::vec4 Normal;
		};

		struct StorageBuffers
		{
			VkBuffer H0Buffer = VK_NULL_HANDLE;
			VkDeviceMemory H0Memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo* H0BufferDscInfo = VK_NULL_HANDLE;

			VkBuffer HtBuffer = VK_NULL_HANDLE;
			VkDeviceMemory HtMemory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo* HtBufferDscInfo = VK_NULL_HANDLE;

			VkBuffer Ht_dmyBuffer = VK_NULL_HANDLE;
			VkDeviceMemory Ht_dmyMemory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo* Ht_dmyBufferDscInfo = VK_NULL_HANDLE;
		};

		struct ComputeUBO
		{
			float deltaT;
			uint32_t meshSize;
			uint32_t OceanSizeLx;
			uint32_t OceanSizeLz;			
		};

		HeightMap(Device& device, const VkQueue& copyQueue);
		~HeightMap();

		void CreateHeightMap(uint32_t size);
		void CreateUniformBuffers();
		void UpdateUniformBuffers(float dt = 0);
		void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer);
		StorageBuffers GetStorageBuffers() { return m_StorageBuffers; }
		ComputeUBO GetUBOBuffers() { return m_ComputeUniformBuffers; }
		VkDescriptorBufferInfo* GetUniformBufferDscInfo() { return m_UniformBufferDscInfo; }
		
	private:
		void SetDescriptorBufferInfo(
			VkDescriptorBufferInfo* info,
			VkBuffer buffer,
			VkDeviceSize size = VK_WHOLE_SIZE,
			VkDeviceSize offset = 0);

		uint32_t m_IndexCount;
		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;

		Device& m_Device;
		const VkQueue& m_CopyComputeQueue;

		StorageBuffers m_StorageBuffers;
		ComputeUBO m_ComputeUniformBuffers;

		VkBuffer m_UniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_UniformBufferMemory = VK_NULL_HANDLE;
		VkDescriptorBufferInfo* m_UniformBufferDscInfo = VK_NULL_HANDLE;
		void* m_Udata = nullptr;
	};
}