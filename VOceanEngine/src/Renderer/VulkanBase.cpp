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
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_Device->GetVkDevice(), m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device->GetVkDevice(), m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_Device->GetVkDevice(), m_InFlightFences[i], nullptr);
		}
		
		vkDestroyPipelineCache(m_Device->GetVkDevice(), m_PipelineCache, nullptr);
	}

	VkCommandBuffer VulkanBase::BeginFrame()
	{
		VOE_CORE_ASSERT(!m_IsFrameStarted && "Can't call beginFrame while already in progress");

		auto result = m_Swapchain->AcquireNextImage(
			m_ImageAvailableSemaphores[m_CurrentFrameIndex],
			m_InFlightFences[m_CurrentFrameIndex],
			&m_CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) 
		{
			RecreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		m_IsFrameStarted = true;

		auto commandBuffer = GetCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VOE_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		return commandBuffer;
	}

	void VulkanBase::EndFrame()
	{
		VOE_CORE_ASSERT(isFrameStarted && "Can't call endFrame while frame is not in progress");
		auto commandBuffer = GetCurrentCommandBuffer();
		VOE_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		auto result = SubmitCommandBuffers();
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_Window->WasWindowResized())
		{
			m_Window->ResetWindowResizedFlag();
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}

		m_IsFrameStarted = false;
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanBase::BeginSwapchainRenderPass(VkCommandBuffer commandBuffer)
	{
		VOE_CORE_ASSERT(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");

		VOE_CORE_ASSERT(
			commandBuffer == getCurrentCommandBuffer() &&
			"Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_Swapchain->GetRenderPass();
		renderPassInfo.framebuffer = m_Swapchain->GetFramebuffer(m_CurrentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_Swapchain->GetSwapchainExtent();

		auto sampleCount = m_PhDevice->GetMsaaSamples();

		VkClearValue clearValues[3];
		if (sampleCount != VK_SAMPLE_COUNT_1_BIT) 
		{
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[2].depthStencil = { 1.0f, 0 };
		}
		else 
		{
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].depthStencil = { 1.0f, 0 };
		}

		renderPassInfo.clearValueCount = sampleCount != VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_Swapchain->GetSwapchainExtent().width);
		viewport.height = static_cast<float>(m_Swapchain->GetSwapchainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, m_Swapchain->GetSwapchainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VulkanBase::EndSwapchainRenderPass(VkCommandBuffer commandBuffer)
	{
		VOE_CORE_ASSERT(m_IsFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		VOE_CORE_ASSERT(commandBuffer == GetCurrentCommandBuffer() &&
			"Can't end render pass on command buffer from a different frame");
		vkCmdEndRenderPass(commandBuffer);
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
		m_Swapchain = std::make_unique<Swapchain>(m_Device.get(), m_PhDevice.get(), m_Surface.get(), m_Window->GetExtent());
	}

	void VulkanBase::RecreateSwapChain()
	{
		auto extent = m_Window->GetExtent();

		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_Window->GetExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Device->GetVkDevice());

		// init swapChain
		if (m_Swapchain == nullptr)
		{
			CreateSwapchain();
		}
		else
		{
			std::shared_ptr<Swapchain> oldSwapchain = std::move(m_Swapchain);
			m_Swapchain = std::make_unique<Swapchain>(
				m_Device.get(), m_PhDevice.get(), m_Surface.get(), extent, oldSwapchain);

			if (!oldSwapchain->CompareSwapFormats(*m_Swapchain.get()))
			{
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}
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
		// Create a semaphore used to synchronize image presentation

		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_ImagesInFlight.resize(m_Swapchain->GetSwapchainImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		// Ensures that the image is displayed before we start submitting new commands to the queue
		// Semaphores will stay the same during application lifetime
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VOE_CHECK_RESULT(vkCreateSemaphore(m_Device->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i]));
			VOE_CHECK_RESULT(vkCreateSemaphore(m_Device->GetVkDevice(), &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]));
			VOE_CHECK_RESULT(vkCreateFence(m_Device->GetVkDevice(), &fenceCreateInfo, nullptr, &m_InFlightFences[i]));
		}
	}

	void VulkanBase::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreatePipelineCache(m_Device->GetVkDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}

	VkResult VulkanBase::SubmitCommandBuffers()
	{
		if (m_ImagesInFlight[m_CurrentImageIndex] != VK_NULL_HANDLE) 
		{
			vkWaitForFences(m_Device->GetVkDevice(), 1, &m_ImagesInFlight[m_CurrentImageIndex], VK_TRUE, UINT64_MAX);
		}

		m_ImagesInFlight[m_CurrentImageIndex] = m_InFlightFences[m_CurrentFrameIndex];

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		auto currentCmdBuffer = GetCurrentCommandBuffer();

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrameIndex];
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &currentCmdBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];

		vkResetFences(m_Device->GetVkDevice(), 1, &m_InFlightFences[m_CurrentFrameIndex]);
		VOE_CHECK_RESULT(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrameIndex]));
		
		VkSwapchainKHR swapChains[] = { m_Swapchain->GetSwapchain() };

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_CurrentImageIndex;

		auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

		return result;
	}

	void VulkanBase::DestroyCommandBuffers()
	{
		vkFreeCommandBuffers(m_Device->GetVkDevice(), m_Device->GetCommandPool(), static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
	}
}
