#pragma once

#include "Platform/Windows/WindowsWindow.h"
#include "Renderer/Swapchain.h"
#include "Renderer/VulkanRenderer.h"

namespace voe{
	
	class Instance;
	class PhDevice;
	class Surface;
	class Device;

	class VOE_API VulkanBase
	{
		public:
		VulkanBase(std::shared_ptr<Window> window);
		~VulkanBase();

	private:
		
		void InitVulkanCore();
		void CreateCommandPool();
		void CreateCommandBuffers();

		std::shared_ptr<Window> m_Window;

		std::unique_ptr<Instance>	m_Instance;
		std::unique_ptr<Surface>	m_Surface;
		std::unique_ptr<PhDevice>	m_PhDevice;
		std::unique_ptr<Device>		m_Device;
		
		VkCommandPool m_CommandPool;

		std::unique_ptr<Swapchain>  m_Swapchain;
		std::unique_ptr<VulkanRenderer> m_Renderer;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;
		size_t m_CurrentFrame = 0;
	};
}


