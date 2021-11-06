#pragma once

#include "VulkanCore/Device.h"
#include "Renderer/Buffer.h"

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

		struct ComputeUBO
		{
			float deltaT = 0.0f;
			uint32_t meshSize;
			uint32_t OceanSizeLx;
			uint32_t OceanSizeLz;			
		};

		HeightMap(Device& device, const VkQueue& copyQueue);
		~HeightMap();

		void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer);

		void CreateHeightMap(uint32_t size);
		void SetupComputeUniformBuffers(uint32_t meshSize, uint32_t lx, uint32_t lz);
		void CreateComputeUniformBuffers();
		void UpdateComputeUniformBuffers(float dt, int frameIndex);

		ComputeUBO GetUBO() { return m_ComputeUBO; }
		VkBuffer GetH0Buffer(uint32_t index) { return m_H0Buffers[index]->GetBuffer(); }
		VkBuffer GetHtBuffer(uint32_t index) { return m_HtBuffers[index]->GetBuffer(); }
		VkBuffer GetHt_dmyBuffer(uint32_t index) { return m_Ht_dmyBuffers[index]->GetBuffer(); }
		
		VkDescriptorBufferInfo* GetUniformBufferDscInfo() { return m_UniformBufferDscInfo; }
		VkDescriptorBufferInfo* GetH0BufferDscInfo() { return m_H0BufferDscInfo; }
		VkDescriptorBufferInfo* GetHtBufferDscInfo() { return m_HtBufferDscInfo; }
		VkDescriptorBufferInfo* GetHt_dmyBufferDscInfo() { return m_Ht_dmyBufferDscInfo; }
		
	private:
		void SetDescriptorBufferInfo(
			VkDescriptorBufferInfo* info,
			VkBuffer buffer,
			VkDeviceSize size = VK_WHOLE_SIZE,
			VkDeviceSize offset = 0);

		Device& m_Device;
		const VkQueue& m_CopyComputeQueue;

		ComputeUBO m_ComputeUBO;

		std::vector<std::shared_ptr<Buffer>> m_UniformBuffers;
		VkDescriptorBufferInfo* m_UniformBufferDscInfo = VK_NULL_HANDLE;

		// storage buffers
		std::vector<std::shared_ptr<Buffer>> m_H0Buffers;
		VkDescriptorBufferInfo* m_H0BufferDscInfo = VK_NULL_HANDLE;

		std::vector<std::shared_ptr<Buffer>> m_HtBuffers;
		VkDescriptorBufferInfo* m_HtBufferDscInfo = VK_NULL_HANDLE;

		std::vector<std::shared_ptr<Buffer>> m_Ht_dmyBuffers;
		VkDescriptorBufferInfo* m_Ht_dmyBufferDscInfo = VK_NULL_HANDLE;
	};
}