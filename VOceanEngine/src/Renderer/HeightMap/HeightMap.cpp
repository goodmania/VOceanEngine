#include "PreCompileHeader.h"
#include "Renderer/HeightMap/HeightMap.h"
#include "Renderer/HeightMap/TessendorfOceane.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace voe {

	HeightMap::HeightMap(Device& device, const VkQueue& copyQueue)
		: m_Device(device), m_CopyComputeQueue(copyQueue)
	{
		m_StorageBuffers.InputBufferDscInfo  = new VkDescriptorBufferInfo();
		m_StorageBuffers.OutputBufferDscInfo = new VkDescriptorBufferInfo();
		m_ComputeUniformBuffers.UniformBufferDscInfo = new VkDescriptorBufferInfo();
	}

	HeightMap::~HeightMap()
	{
		vkDestroyBuffer(m_Device.GetVkDevice(), m_StorageBuffers.InputBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_StorageBuffers.InputMemory, nullptr);
		delete m_StorageBuffers.InputBufferDscInfo;

		vkDestroyBuffer(m_Device.GetVkDevice(), m_StorageBuffers.OutputBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_StorageBuffers.OutputMemory, nullptr);
		delete m_StorageBuffers.OutputBufferDscInfo;

		vkDestroyBuffer(m_Device.GetVkDevice(), m_ComputeUniformBuffers.UniformBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_ComputeUniformBuffers.UniformBufferMemory, nullptr);
		delete m_ComputeUniformBuffers.UniformBufferDscInfo;
	}

	void HeightMap::AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer)
	{

	}

	void HeightMap::CreateUniformBuffers()
	{
		m_Device.CreateBuffer(
			sizeof(ComputeUBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_ComputeUniformBuffers.UniformBuffer,
			m_ComputeUniformBuffers.UniformBufferMemory);

		vkMapMemory(m_Device.GetVkDevice(), m_ComputeUniformBuffers.UniformBufferMemory, 0, sizeof(ComputeUBO), 0, &m_ComputeUniformBuffers.data);
		memcpy(m_ComputeUniformBuffers.data, &m_ComputeUniformBuffers.UniformBuffer, sizeof(ComputeUBO));
		vkUnmapMemory(m_Device.GetVkDevice(), m_ComputeUniformBuffers.UniformBufferMemory);
	}

	void HeightMap::UpdateUniformBuffers(float dt)
	{
		m_ComputeUniformBuffers.deltaT = dt;
		memcpy(m_ComputeUniformBuffers.data, &m_ComputeUniformBuffers.UniformBuffer, sizeof(ComputeUBO));
	}

	void HeightMap::CreateHeightMap(uint32_t gridSize)
	{
		VOE_CORE_ASSERT(m_Device);
		VOE_CORE_ASSERT(m_CopyQueue != nullptr);

		std::vector<Ocean> oceanBuffer(gridSize * gridSize);

		TessendorfOceane* tOceanManeger = new TessendorfOceane(gridSize);
		tOceanManeger->Generate(oceanBuffer);
		VkDeviceSize bufferSize = oceanBuffer.size() * sizeof(Ocean);

		// init ComputeUBO members
		m_ComputeUniformBuffers.meshSize = tOceanManeger->m_MeshSize;
		m_ComputeUniformBuffers.OceanSizeLx = tOceanManeger->m_OceanSizeLx;
		m_ComputeUniformBuffers.OceanSizeLz = tOceanManeger->m_OceanSizeLz;
		CreateUniformBuffers();
		SetDescriptorBufferInfo(m_ComputeUniformBuffers.UniformBufferDscInfo, m_ComputeUniformBuffers.UniformBuffer);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		m_Device.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(m_Device.GetVkDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, oceanBuffer.data(), static_cast<size_t>(oceanBuffer.size()));
		vkUnmapMemory(m_Device.GetVkDevice(), stagingBufferMemory);

		m_Device.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_StorageBuffers.InputBuffer,
			m_StorageBuffers.InputMemory);

		m_Device.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_StorageBuffers.OutputBuffer,
			m_StorageBuffers.OutputMemory);

		SetDescriptorBufferInfo(m_StorageBuffers.InputBufferDscInfo,  m_StorageBuffers.InputBuffer);
		SetDescriptorBufferInfo(m_StorageBuffers.OutputBufferDscInfo, m_StorageBuffers.OutputBuffer);

		// Copy from staging buffer
		VkCommandBuffer copyCmd = m_Device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer, m_StorageBuffers.InputBuffer, 1, &copyRegion);
		vkCmdCopyBuffer(copyCmd, stagingBuffer, m_StorageBuffers.OutputBuffer, 1, &copyRegion);

		AddGraphicsToComputeBarriers(copyCmd);
		m_Device.FlushCommandBuffer(copyCmd, m_CopyComputeQueue, true);

		vkDestroyBuffer(m_Device.GetVkDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), stagingBufferMemory, nullptr);

		// Indices
		std::vector<uint32_t> indices;
		for (uint32_t y = 0; y < gridSize - 1; y++) 
		{
			for (uint32_t x = 0; x < gridSize; x++) 
			{
				indices.push_back((y + 1) * gridSize + x);
				indices.push_back((y) * gridSize + x);
			}
			// Primitive restart (signaled by special value 0xFFFFFFFF)
			indices.push_back(0xFFFFFFFF);
		}

		uint32_t indexBufferSize = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);
		m_IndexCount = static_cast<uint32_t>(indices.size());

		m_Device.CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* idata;
		vkMapMemory(m_Device.GetVkDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &idata);
		memcpy(data, indices.data(), static_cast<size_t>(indices.size()));
		vkUnmapMemory(m_Device.GetVkDevice(), stagingBufferMemory);

		m_Device.CreateBuffer(
			indexBufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_IndexBuffer,
			m_IndexBufferMemory);
	}

	void HeightMap::SetDescriptorBufferInfo(VkDescriptorBufferInfo* info, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset)
	{
		info->buffer = buffer;
		info->offset = offset;
		info->range = size;
	}
}