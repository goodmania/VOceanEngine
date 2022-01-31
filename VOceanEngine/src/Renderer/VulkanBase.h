#pragma once

#include "Platform/Windows/WindowsWindow.h"
#include "Renderer/Swapchain.h"
#include "Renderer/VulkanOceanRenderer.h"
#include "Renderer/VulkanModelRenderer.h"
#include "Renderer/VulkanImguiRenderer.h"

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

		const std::shared_ptr<Device> GetDevice() const { return m_Device; }

		VulkanRenderer& GetRenderer() const { return *m_Renderer; }
		VulkanModelRenderer& GetModelRenderer() const { return *m_ModelRenderer; }
		VulkanImguiRenderer& GetImguiRenderer() const { return *m_ImguiRenderer; }
		const float GetAspectRatio() const { return m_Swapchain->AspectRatio(); }

		bool IsFrameInProgress() const { return m_IsFrameStarted; }

		VkCommandBuffer BeginFrame();
		void EndFrame();
		void BeginSwapchainRenderPass(VkCommandBuffer commandBuffer);
		void EndSwapchainRenderPass(VkCommandBuffer commandBuffer);

		VkResult SubmitCommandBuffers();
		
	private:
		void InitVulkanDevice();
		void CreateSwapchain();
		void CreateVulkanRenderer();
		void CreateImguiRenderer();
		void CreateModelRenderer();
		void RecreateSwapChain();
		void CreateCommandBuffers();
		void CreateSyncObjects();

		VkSubmitInfo CreateSubmitInfo(
			VkSemaphore& waitSemaphore,
			VkSemaphore& signalSemaphore,
			VkCommandBuffer& commandBuffer,
			VkPipelineStageFlags pipelineStageFlag,
			bool firstDraw);

		void DestroyCommandBuffers();
		
		std::shared_ptr<Window> m_Window;

		std::unique_ptr<Instance>	m_Instance;
		std::unique_ptr<Surface>	m_Surface;
		std::unique_ptr<PhDevice>	m_PhDevice;
		std::shared_ptr<Device>		m_Device;
		
		std::vector<VkCommandBuffer> m_CommandBuffers; // command buffers for rendering

		std::unique_ptr<Swapchain>  m_Swapchain;
		std::unique_ptr<VulkanRenderer> m_Renderer;
		std::unique_ptr<VulkanModelRenderer> m_ModelRenderer;
		std::unique_ptr<VulkanImguiRenderer> m_ImguiRenderer;

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


