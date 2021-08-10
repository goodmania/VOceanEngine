#pragma once

#include "Platform/Windows/WindowsWindow.h"

namespace voe{
	
	class Instance;
	class PhDevice;
	class Surface;
	class Device;
	class Swapchain;

	class VOE_API VulkanBase
	{
	public:
		VulkanBase(std::shared_ptr<Window> window);
		~VulkanBase();

	private:
		void InitVulkanCore();

		std::shared_ptr<Window> m_Window;

		std::unique_ptr<Instance>	m_Instance;
		std::unique_ptr<Surface>	m_Surface;
		std::unique_ptr<PhDevice>	m_PhDevice;
		std::unique_ptr<Device>		m_Device;

		std::unique_ptr<Swapchain>  m_Swapchain;
	};
}


