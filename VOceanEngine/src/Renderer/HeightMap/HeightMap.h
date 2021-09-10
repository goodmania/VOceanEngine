#pragma once

#include "VulkanCore/Device.h"

namespace voe
{
	class HeightMap
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
			VkBuffer InputBuffer = VK_NULL_HANDLE;
			VkDeviceMemory InputMemory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo* InputBufferDscInfo = VK_NULL_HANDLE;
			VkBuffer OutputBuffer = VK_NULL_HANDLE;
			VkDeviceMemory OutputMemory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo* OutputBufferDscInfo = VK_NULL_HANDLE;
		};

		struct ComputeUBO
		{
			float deltaT = 0.0f;
			uint32_t meshSize;
			uint32_t OceanSizeLx;
			uint32_t OceanSizeLz;
			void* data = nullptr;
			VkBuffer UniformBuffer = VK_NULL_HANDLE;
			VkDeviceMemory UniformBufferMemory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo* UniformBufferDscInfo = VK_NULL_HANDLE;
		};

		HeightMap(Device& device, const VkQueue& copyQueue);
		~HeightMap();

		void CreateHeightMap(uint32_t gridsize);
		void CreateUniformBuffers();
		void UpdateUniformBuffers(float dt);
		void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer);
		StorageBuffers GetStorageBuffers() { return m_StorageBuffers; }
		ComputeUBO GetUBOBuffers() { return m_ComputeUniformBuffers; }
		
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
	};
}