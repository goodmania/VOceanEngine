#include "PreCompileHeader.h"
#include "Renderer/HeightMap/HeightMap.h"
#include "Renderer/HeightMap/TessendorfOceane.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace voe {

	HeightMap::HeightMap(Device& device, const VkQueue& copyQueue)
		: m_Device(device), m_CopyComputeQueue(copyQueue)
	{
		m_StorageBuffers.H0BufferDscInfo  = new VkDescriptorBufferInfo();
		m_StorageBuffers.HtBufferDscInfo = new VkDescriptorBufferInfo();
		m_StorageBuffers.Ht_dmyBufferDscInfo = new VkDescriptorBufferInfo();
		m_UniformBufferDscInfo = new VkDescriptorBufferInfo();
	}

	HeightMap::~HeightMap()
	{
		vkDestroyBuffer(m_Device.GetVkDevice(), m_StorageBuffers.H0Buffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_StorageBuffers.H0Memory, nullptr);
		delete m_StorageBuffers.H0BufferDscInfo;

		vkDestroyBuffer(m_Device.GetVkDevice(), m_StorageBuffers.HtBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_StorageBuffers.HtMemory, nullptr);
		delete m_StorageBuffers.HtBufferDscInfo;

		vkDestroyBuffer(m_Device.GetVkDevice(), m_StorageBuffers.Ht_dmyBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_StorageBuffers.Ht_dmyMemory, nullptr);
		delete m_StorageBuffers.Ht_dmyBufferDscInfo;

		vkDestroyBuffer(m_Device.GetVkDevice(), m_UniformBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_UniformBufferMemory, nullptr);
		delete m_UniformBufferDscInfo;
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
			m_UniformBuffer,
			m_UniformBufferMemory);

		vkMapMemory(m_Device.GetVkDevice(), m_UniformBufferMemory, 0, sizeof(ComputeUBO), 0, &m_Udata);
		memcpy(m_Udata, &m_UniformBuffer, sizeof(ComputeUBO));
		vkUnmapMemory(m_Device.GetVkDevice(), m_UniformBufferMemory);
	}

	void HeightMap::UpdateUniformBuffers(float dt)
	{
		m_ComputeUniformBuffers.deltaT = dt;
		memcpy(m_Udata, &m_UniformBuffer, sizeof(ComputeUBO));
	}

	void HeightMap::CreateHeightMap(uint32_t size)
	{
		VOE_CORE_ASSERT(m_Device);
		VOE_CORE_ASSERT(m_CopyQueue != nullptr);

		//
		// 
		//			must reconstructing!!!!!
		// 
		// 

		// init ComputeUBO members
		std::vector<glm::vec2> h0Buffer(size * size);
		TessendorfOceane* tOceanManeger = new TessendorfOceane(size);
		tOceanManeger->Generate(h0Buffer);
		VkDeviceSize bufferSize = h0Buffer.size() * sizeof(glm::vec2);
		
		m_ComputeUniformBuffers.meshSize = tOceanManeger->m_MeshSize;
		m_ComputeUniformBuffers.OceanSizeLx = tOceanManeger->m_OceanSizeLx;
		m_ComputeUniformBuffers.OceanSizeLz = tOceanManeger->m_OceanSizeLz;
		CreateUniformBuffers();
		SetDescriptorBufferInfo(m_UniformBufferDscInfo, m_UniformBuffer);

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
		memcpy(data, h0Buffer.data(), static_cast<size_t>(h0Buffer.size()));
		vkUnmapMemory(m_Device.GetVkDevice(), stagingBufferMemory);

		m_Device.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_StorageBuffers.H0Buffer,
			m_StorageBuffers.H0Memory);

		m_Device.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_StorageBuffers.HtBuffer,
			m_StorageBuffers.HtMemory);

		m_Device.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_StorageBuffers.Ht_dmyBuffer,
			m_StorageBuffers.Ht_dmyMemory);

		SetDescriptorBufferInfo(m_StorageBuffers.H0BufferDscInfo,  m_StorageBuffers.H0Buffer);
		SetDescriptorBufferInfo(m_StorageBuffers.HtBufferDscInfo, m_StorageBuffers.HtBuffer);
		SetDescriptorBufferInfo(m_StorageBuffers.Ht_dmyBufferDscInfo, m_StorageBuffers.Ht_dmyBuffer);

		// Copy from staging buffer
		VkCommandBuffer copyCmd = m_Device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};
		copyRegion.size = bufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer, m_StorageBuffers.H0Buffer, 1, &copyRegion);
		// vkCmdCopyBuffer(copyCmd, stagingBuffer, m_StorageBuffers.HtBuffer, 1, &copyRegion);

		AddGraphicsToComputeBarriers(copyCmd);
		m_Device.FlushCommandBuffer(copyCmd, m_CopyComputeQueue, true);

		vkDestroyBuffer(m_Device.GetVkDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), stagingBufferMemory, nullptr);

		// Indices
		std::vector<uint32_t> indices;
		for (uint32_t y = 0; y < size - 1; y++) 
		{
			for (uint32_t x = 0; x < size; x++) 
			{
				indices.push_back((y + 1) * size + x);
				indices.push_back((y) * size + x);
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