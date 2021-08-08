#pragma once

#include "Platform/Windows/WindowsWindow.h"

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

		std::shared_ptr<Window> m_Window;

		std::shared_ptr<Instance>	m_Instance;
		std::shared_ptr<Surface>	m_Surface;
		std::shared_ptr<PhDevice>	m_PhDevice;
		std::shared_ptr<Device>		m_Device;
	};
}


