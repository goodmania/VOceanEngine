#include "PreCompileHeader.h"


#include "Renderer/Swapchain.h"
#include "Renderer/HeightMap/HeightMap.h"
#include "Renderer/HeightMap/TessendorfOceane.h"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace voe {

	HeightMap::HeightMap(Device& device, const VkQueue& copyQueue)
		: m_Device(device), m_CopyComputeQueue(copyQueue)
	{
		m_H0BufferDscInfo  = new VkDescriptorBufferInfo();
		m_HtBufferDscInfo = new VkDescriptorBufferInfo();
		m_Ht_dmyBufferDscInfo = new VkDescriptorBufferInfo();
		m_UniformBufferDscInfo = new VkDescriptorBufferInfo();
	}

	HeightMap::~HeightMap()
	{
		delete m_H0BufferDscInfo;
		delete m_HtBufferDscInfo;
		delete m_Ht_dmyBufferDscInfo;
		delete m_UniformBufferDscInfo;
	}

	void HeightMap::AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer)
	{

	}

	void HeightMap::SetupComputeUniformBuffers(uint32_t meshSize, uint32_t lx, uint32_t lz)
	{
		m_ComputeUBO.meshSize = meshSize;
		m_ComputeUBO.OceanSizeLx = lx;
		m_ComputeUBO.OceanSizeLz = lz;
		CreateComputeUniformBuffers();
	}

	void HeightMap::CreateComputeUniformBuffers()
	{
		m_UniformBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT); // MAX_FRAMES_IN_FLIGHT

		for (int i = 0; i < m_UniformBuffers.size(); i++)
		{
			m_UniformBuffers[i] = std::make_unique<Buffer>(
				m_Device,
				sizeof(ComputeUBO),
				1, 
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				m_Device.GetMinUniformBufferOffsetAlignment());

			m_UniformBuffers[i]->Map();
			SetDescriptorBufferInfo(m_UniformBufferDscInfo, m_UniformBuffers[i]->GetBuffer());
		}
	}

	void HeightMap::UpdateComputeUniformBuffers(float dt, int frameIndex)
	{
		m_ComputeUBO.deltaT += dt;
		m_UniformBuffers[frameIndex]->WriteToBuffer(&m_ComputeUBO);
		m_UniformBuffers[frameIndex]->Flush();
	}

	void HeightMap::CreateHeightMap(uint32_t size)
	{
		VOE_CORE_ASSERT(m_Device);
		VOE_CORE_ASSERT(m_CopyQueue != nullptr);

		std::vector<glm::vec2> h0Buffer(size * size);
		TessendorfOceane tOceanManeger(size);
		tOceanManeger.Generate(h0Buffer);
		VkDeviceSize bufferSize = static_cast<uint32_t>(h0Buffer.size()) * sizeof(glm::vec2);
		uint32_t elementSize = sizeof(glm::vec2);

		SetupComputeUniformBuffers(
			tOceanManeger.m_MeshSize,
			tOceanManeger.m_OceanSizeLx,
			tOceanManeger.m_OceanSizeLz);

		Buffer stagingBuffer
		{
			m_Device,
			elementSize,
			static_cast<uint32_t>(h0Buffer.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)h0Buffer.data());

		m_H0Buffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
		m_HtBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
		m_Ht_dmyBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

		for (int i = 0; i < Swapchain::MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_H0Buffers[i] = std::make_shared<Buffer>(
				m_Device,
				elementSize,
				static_cast<uint32_t>(h0Buffer.size()),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			m_HtBuffers[i] = std::make_shared<Buffer>(
				m_Device,
				elementSize,
				static_cast<uint32_t>(h0Buffer.size()),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			m_Ht_dmyBuffers[i] = std::make_shared<Buffer>(
				m_Device,
				elementSize,
				static_cast<uint32_t>(h0Buffer.size()),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			SetDescriptorBufferInfo(m_H0BufferDscInfo, m_H0Buffers[i]->GetBuffer());
			SetDescriptorBufferInfo(m_HtBufferDscInfo, m_HtBuffers[i]->GetBuffer());
			SetDescriptorBufferInfo(m_Ht_dmyBufferDscInfo, m_Ht_dmyBuffers[i]->GetBuffer());

			// Copy from staging buffer
			VkCommandBuffer copyCmd = m_Device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkBufferCopy copyRegion = {};
			copyRegion.size = bufferSize;

			// Should also initialize ht and h_dmy with stagingbuffer?
			vkCmdCopyBuffer(copyCmd, stagingBuffer.GetBuffer(), m_H0Buffers[i]->GetBuffer(), 1, &copyRegion);
			vkCmdCopyBuffer(copyCmd, stagingBuffer.GetBuffer(), m_HtBuffers[i]->GetBuffer(), 1, &copyRegion);
			vkCmdCopyBuffer(copyCmd, stagingBuffer.GetBuffer(), m_Ht_dmyBuffers[i]->GetBuffer(), 1, &copyRegion);

			// Execute a transfer barrier to the compute queue, if necessary
			m_Device.FlushCommandBuffer(copyCmd, m_CopyComputeQueue, true);
		}
	}

	void HeightMap::SetDescriptorBufferInfo(VkDescriptorBufferInfo* info, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset)
	{
		info->buffer = buffer;
		info->range = size;
		info->offset = offset;
	}
}