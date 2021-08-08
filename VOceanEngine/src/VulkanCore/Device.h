#pragma once

#include "Platform/Windows/WindowsWindow.h"

namespace voe {

	class Instance;
	class PhDevice;
	class Surface;

	class VOE_API Device
	{
	public:
		Device(const Instance* instance, PhDevice* phDevice, const Surface* surface);
		~Device();

		const VkQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
		const VkQueue& GetPresentQueue() const	{ return m_PresentQueue; }
		// const VkQueue& GetTransferQueue() const { return m_TransferQueue; }
		// const VkQueue& GetComputeQueue()  const { return m_ComputeQueue; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;

		const Instance* m_Instance;
		const PhDevice* m_PhDevice;
		const Surface*	m_Surface;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue	= VK_NULL_HANDLE;
		// VkQueue m_TransferQueue = VK_NULL_HANDLE;
		// VkQueue m_ComputeQueue  = VK_NULL_HANDLE;
	};
}



