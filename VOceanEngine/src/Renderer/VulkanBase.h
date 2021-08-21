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
		static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

		VulkanBase(std::shared_ptr<Window> window);
		~VulkanBase();
		VulkanBase(const VulkanBase&) = delete;
		VulkanBase& operator=(const VulkanBase&) = delete;

		uint32_t GetFrameIndex() const
		{
			VOE_CORE_ASSERT(m_IsFrameStarted && "Cannot get frame index when frame not in progress");
			return m_CurrentFrameIndex;
		}

		VkCommandBuffer GetCurrentCommandBuffer() const
		{
			VOE_CORE_ASSERT(m_IsFrameStarted && "Cannot get command buffer when frame not in progress");
			return m_CommandBuffers[m_CurrentFrameIndex];
		}

		bool IsFrameInProgress() const { return m_IsFrameStarted; }

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapchainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapchainRenderPass(VkCommandBuffer commandBuffer);

		VkResult SubmitCommandBuffers();
		const std::shared_ptr<Device> GetDevice() const { return m_Device; }
		
	private:
		void InitVulkanDevice();
		void CreateSwapchain();
		void RecreateSwapChain();
		void CreateCommandBuffers();
		void CreateSyncObjects();
		void CreatePipelineCache();

		void DestroyCommandBuffers();
		
		std::shared_ptr<Window> m_Window;

		std::unique_ptr<Instance>	m_Instance;
		std::unique_ptr<Surface>	m_Surface;
		std::unique_ptr<PhDevice>	m_PhDevice;
		std::shared_ptr<Device>		m_Device;
		
		std::vector<VkCommandBuffer> m_CommandBuffers; // command buffers for rendering

		std::unique_ptr<Swapchain>  m_Swapchain;
		std::unique_ptr<VulkanRenderer> m_Renderer;

		VkPipelineCache m_PipelineCache;

		// syncs
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		uint32_t m_CurrentImageIndex = 0;
		size_t m_CurrentFrameIndex = 0;
		bool m_IsFrameStarted = false;
	};
}


