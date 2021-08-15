#pragma once

#include "Platform/Windows/WindowsWindow.h"
#include "VulkanCore/PhDevice.h"

namespace voe {

	class Instance;
	class Surface;

	class VOE_API Device
	{
		friend class VulkanBase;
	public:
		Device(const Instance* instance, PhDevice* phDevice, const Surface* surface);
		~Device();

		const VkDevice& GetVkDevice() const { return m_Device; }

		const VkQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
		const VkQueue& GetPresentQueue() const	{ return m_PresentQueue; }
		const VkQueue& GetComputeQueue()  const { return m_ComputeQueue; }
		const VkQueue& GetTransferQueue() const { return m_TransferQueue; }

		uint32_t GetGraphicsQueueFamily() const { return m_Indices.graphicsFamily; }
		uint32_t GetPresentQueueFamily() const  { return m_Indices.presentFamily; }
		uint32_t GetComputeQueueFamily()  const { return m_Indices.computeFamily; }
		uint32_t GetTransferQueueFamily() const { return m_Indices.transferFamily; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;

		const Instance* m_Instance;
		const PhDevice* m_PhDevice;
		const Surface*	m_Surface;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue	= VK_NULL_HANDLE;
		VkQueue m_ComputeQueue  = VK_NULL_HANDLE;
		VkQueue m_TransferQueue = VK_NULL_HANDLE;

		QueueFamilyIndices m_Indices;
	};
}



