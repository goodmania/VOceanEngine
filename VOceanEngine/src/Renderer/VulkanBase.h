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
			return m_CurrentFrame;
		}

		VkCommandBuffer GetCurrentCommandBuffer() const
		{
			VOE_CORE_ASSERT(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return m_CommandBuffers[m_CurrentFrame];
		}

		bool IsFrameInProgress() const { return m_IsFrameStarted; }

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);

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
		std::unique_ptr<Device>		m_Device;
		
		std::vector<VkCommandBuffer> m_CommandBuffers; // command buffers for rendering

		std::unique_ptr<Swapchain>  m_Swapchain;
		std::unique_ptr<VulkanRenderer> m_Renderer;

		VkPipelineCache m_PipelineCache;

		// lve syncs
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		// sascha syncs
		struct {
			VkSemaphore presentComplete;
			VkSemaphore renderComplete;
		} m_Semaphores;
		// Contains the command buffer and semaphore to be presented to the queue.
		VkSubmitInfo m_SubmitInfo;
		/** @brief Pipeline stages used to wait at for graphics queue submissions */
		VkPipelineStageFlags m_SubmitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		std::vector<VkFence> m_WaitFences;

		size_t m_CurrentFrame = 0;
		bool m_IsFrameStarted = false;
	};
}


