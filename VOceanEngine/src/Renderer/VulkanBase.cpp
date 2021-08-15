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

		// When making the swap chain pointer, also create the render pass and frame buffer.
		m_Swapchain = std::make_unique<Swapchain>(m_Device.get(), m_PhDevice.get(), m_Surface.get());
	}

	void VulkanBase::CreateCommandPool()
	{
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_Device->GetGraphicsQueueFamily();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VOE_CHECK_RESULT(vkCreateCommandPool(m_Device->GetVkDevice(), &cmdPoolInfo, nullptr, &m_CommandPool));
	}

	void VulkanBase::CreateCommandBuffers()
	{
		// Create one command buffer for each swap chain image and reuse for rendering
		drawCmdBuffers.resize(m_Swapchain->GetSwapchainImageCount());

		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vks::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				static_cast<uint32_t>(drawCmdBuffers.size()));

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
	}

	void VulkanBase::destroyCommandBuffers()
	{
		vkFreeCommandBuffers(device, cmdPool, static_cast<uint32_t>(drawCmdBuffers.size()), drawCmdBuffers.data());
	}
}


