#include "PreCompileHeader.h"
#include "Renderer/VulkanBase.h"

#include "VulkanCore/VulkanCoreHeader.h"

namespace voe {

	VulkanBase::VulkanBase(std::shared_ptr<Window> window) : m_Window(window)
	{
		InitVulkanCore();
	}

	VulkanBase::~VulkanBase()
	{

	}

	void VulkanBase::InitVulkanCore()
	{
		m_Instance	= std::make_unique<Instance>();
		m_Surface	= std::make_unique<Surface>(m_Instance.get(), m_Window.get());
		m_PhDevice	= std::make_unique<PhDevice>(m_Instance.get(), m_Surface.get());
		m_Device	= std::make_unique<Device>(m_Instance.get(), m_PhDevice.get(), m_Surface.get());
		m_Swapchain = std::make_unique<Swapchain>(m_Device.get(), m_PhDevice.get(), m_Surface.get());
	}
}


