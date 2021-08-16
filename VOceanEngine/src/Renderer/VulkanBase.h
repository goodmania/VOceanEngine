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
		VulkanBase(std::shared_ptr<Window> window);
		~VulkanBase();

	private:
		void InitVulkanDevice();
		void CreateSwapchain();
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
	};
}


