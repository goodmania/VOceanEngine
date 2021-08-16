#include "PreCompileHeader.h"

#include "Renderer/VulkanBase.h"
#include "VulkanCore/VulkanCoreHeader.h"

namespace voe {

	VulkanBase::VulkanBase(std::shared_ptr<Window> window) : m_Window(window)
	{
		InitVulkanDevice();
		CreateSwapchain();
		CreateCommandBuffers();
		CreateSyncObjects();
		CreatePipelineCache();
	}

	VulkanBase::~VulkanBase()
	{
		DestroyCommandBuffers();
		vkDestroySemaphore(m_Device->GetVkDevice(), m_Semaphores.presentComplete, nullptr);
		vkDestroySemaphore(m_Device->GetVkDevice(), m_Semaphores.renderComplete, nullptr);
		for (auto& fence : m_WaitFences) 
		{
			vkDestroyFence(m_Device->GetVkDevice(), fence, nullptr);
		}
		vkDestroyPipelineCache(m_Device->GetVkDevice(), m_PipelineCache, nullptr);
	}

	void VulkanBase::InitVulkanDevice()
	{
		m_Instance	= std::make_unique<Instance>();
		m_Surface	= std::make_unique<Surface>(m_Instance.get(), m_Window.get());
		m_PhDevice	= std::make_unique<PhDevice>(m_Instance.get(), m_Surface.get());
		m_Device	= std::make_unique<Device>(m_Instance.get(), m_PhDevice.get(), m_Surface.get());	
	}

	void VulkanBase::CreateSwapchain()
	{
		// When create the swapchain, also create the render pass and frame buffer.
		m_Swapchain = std::make_unique<Swapchain>(m_Device.get(), m_PhDevice.get(), m_Surface.get());
	}

	void VulkanBase::CreateCommandBuffers()
	{
		// Create a command buffer for each swap chain image and reuse it for rendering
		m_CommandBuffers.resize(m_Swapchain->GetSwapchainImageCount());

		VkCommandBufferAllocateInfo commandBufferAlocInfo = {};
		commandBufferAlocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAlocInfo.pNext = nullptr;
		commandBufferAlocInfo.commandPool = m_Device->GetCommandPool();
		commandBufferAlocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAlocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
		
		VOE_CHECK_RESULT(vkAllocateCommandBuffers(m_Device->GetVkDevice(), &commandBufferAlocInfo, m_CommandBuffers.data()));
	}

	void VulkanBase::CreateSyncObjects()
	{
		// Create synchronization objects
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		// Create a semaphore used to synchronize image presentation
		// Ensures that the image is displayed before we start submitting new commands to the queue
		VOE_CHECK_RESULT(vkCreateSemaphore(m_Device->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_Semaphores.presentComplete));
		// Create a semaphore used to synchronize command submission
		// Ensures that the image is not presented until all commands have been submitted and executed
		VOE_CHECK_RESULT(vkCreateSemaphore(m_Device->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_Semaphores.renderComplete));

		// Set up submit info structure
		// Semaphores will stay the same during application lifetime
		m_SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		m_SubmitInfo.pWaitDstStageMask = &m_SubmitPipelineStages;
		m_SubmitInfo.waitSemaphoreCount = 1;
		m_SubmitInfo.pWaitSemaphores = &m_Semaphores.presentComplete;
		m_SubmitInfo.signalSemaphoreCount = 1;
		m_SubmitInfo.pSignalSemaphores = &m_Semaphores.renderComplete;

		// Create fence to synchronize command buffer access
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = 0;

		m_WaitFences.resize(m_CommandBuffers.size());
		for (auto& fence : m_WaitFences)
		{
			VOE_CHECK_RESULT(vkCreateFence(m_Device->GetVkDevice(), &fenceCreateInfo, nullptr, &fence));
		}
	}

	void VulkanBase::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreatePipelineCache(m_Device->GetVkDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}

	void VulkanBase::DestroyCommandBuffers()
	{
		vkFreeCommandBuffers(m_Device->GetVkDevice(), m_Device->GetCommandPool(), static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
	}
}
