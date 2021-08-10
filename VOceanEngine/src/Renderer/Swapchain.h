#pragma once

#include "Platform/Windows/WindowsWindow.h"

namespace voe {

	class Device;
	class PhDevice;
	class Surface;

	struct SwapChainBuffer 
	{
		VkImage image;
		VkImageView view;
	};

	class VOE_API Swapchain
	{
	public:
		Swapchain(Device* device, PhDevice* phDevice, Surface* surface, 
			VkExtent2D windowExtent, const Swapchain* previous = nullptr);

		Swapchain(Device* device, PhDevice* phDevice, Surface* surface, VkExtent2D windowExtent);
		Swapchain(Device* device, PhDevice* phDevice, Surface* surface);
		~Swapchain();

		VkResult AcquireNextImage(uint32_t* imageIndex);

		const size_t GetSwapchainImageCount() const { return m_SwapchainImages.size(); }
		const VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }

	private:
		void InitSwapchain();
		void CreateSwapchain();
		void CreateImageView();
		void CreateDepthResources();

		// choose swapchian format, present mode, and extent
		VkPresentModeKHR ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkFormat FindDepthFormat();

		// swapchain
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		std::shared_ptr<Swapchain> m_OldSwapchain;

		// Vulkancore pointers
		Device* m_Device;
		PhDevice* m_PhDevice;
		Surface* m_Surface;

		VkFence m_FenceImage = VK_NULL_HANDLE;
		uint32_t m_ActiveImageIndex;

		VkExtent2D m_WindowExtent;
		VkPresentModeKHR m_PresentMode;

		uint32_t m_ImageCount = 0;
		VkSurfaceTransformFlagsKHR m_PreTransform;
		VkCompositeAlphaFlagBitsKHR m_CompositeAlpha;

		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImgaeViews;
		std::vector<VkImage> m_DepthImages;
		std::vector<VkImageView> m_DepthImageViews;
		std::vector<VkDeviceMemory> m_DepthImageMemories;

		VkFormat m_SwapchainImageFormat;
		VkFormat m_SwapchainDepthFormat;
		VkExtent2D m_SwapchainExtent;
		std::vector<SwapChainBuffer> m_Buffers;
	};
}



