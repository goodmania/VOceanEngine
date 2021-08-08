#include "PreCompileHeader.h"
#include "VulkanBase.h"

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
		m_Instance	= std::make_shared<Instance>();
		m_Surface	= std::make_shared<Surface>(m_Instance.get(), m_Window.get());
		m_PhDevice	= std::make_shared<PhDevice>(m_Instance.get(), m_Surface.get());
		m_Device	= std::make_shared<Device>(m_Instance.get(), m_PhDevice.get(), m_Surface.get());
	}
}


