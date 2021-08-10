#include "PreCompileHeader.h"
#include "Swapchain.h"

#include "VulkanCore/VulkanCoreHeader.h"

namespace voe {

	Swapchain::Swapchain(Device* device, PhDevice* phDevice, Surface* surface, VkExtent2D windowExtent, const Swapchain* previous)
	{
		InitSwapchain();
	}

	Swapchain::Swapchain(Device* device, PhDevice* phDevice, Surface* surface, VkExtent2D windowExtent)
	{
		InitSwapchain();
	}

	Swapchain::Swapchain(Device* device, PhDevice* phDevice, Surface* surface)
		: m_Device(device), m_PhDevice(phDevice), m_Surface(surface)
	{
		InitSwapchain();
	}

	Swapchain::~Swapchain()
	{
		for (auto imageView : m_SwapchainImgaeViews)
		{
			vkDestroyImageView(m_Device->GetVkDevice(), imageView, nullptr);
		}
		m_SwapchainImgaeViews.clear();

		if (m_Swapchain != nullptr)
		{
			vkDestroySwapchainKHR(m_Device->GetVkDevice(), m_Swapchain, nullptr);
			m_Swapchain = nullptr;
		}

		for (int i = 0; i < m_DepthImages.size(); i++)
		{
			vkDestroyImageView(m_Device->GetVkDevice(), m_DepthImageViews[i], nullptr);
			vkDestroyImage(m_Device->GetVkDevice(), m_DepthImages[i], nullptr);
			vkFreeMemory(m_Device->GetVkDevice(), m_DepthImageMemories[i], nullptr);
		}
	}

	void Swapchain::InitSwapchain()
	{
		CreateSwapchain();
		CreateImageView();
		CreateDepthResources();
	}

	void Swapchain::CreateSwapchain()
	{
		SwapchainSupportDetails swapchainSupport = m_PhDevice->GetSwapchainSupport();
		VkSurfaceFormatKHR surfaceFormat = m_Surface->ChooseSwapchainSurfaceFormat(swapchainSupport.formats);
		VkPresentModeKHR presentMode = ChooseSwapchainPresentMode(swapchainSupport.presentModes);
		VkExtent2D extent = ChooseSwapchainExtent(swapchainSupport.capabilities);

		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

		if (swapchainSupport.capabilities.maxImageCount > 0 && 
			imageCount > swapchainSupport.capabilities.maxImageCount)
		{
			imageCount = swapchainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_Surface->GetVkSurface();
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = extent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = m_PhDevice->FindQueueFamilies(m_PhDevice->GetVkPhysicalDevice());
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swapchainCreateInfo.preTransform = swapchainSupport.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = m_OldSwapchain == nullptr ? VK_NULL_HANDLE : m_OldSwapchain->m_Swapchain;

		VOE_CHECK_RESULT(vkCreateSwapchainKHR(m_Device->GetVkDevice(), &swapchainCreateInfo, nullptr, &m_Swapchain));

		// we only specified a minimum number of images in the swap chain, so the implementation is
		// allowed to create a swap chain with more. That's why we'll first query the final number of
		// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
		// retrieve the handles.
		vkGetSwapchainImagesKHR(m_Device->GetVkDevice(), m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device->GetVkDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());

		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainDepthFormat = FindDepthFormat();
		m_SwapchainExtent = extent;
	}

	VkPresentModeKHR Swapchain::ChooseSwapchainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				VOE_CORE_INFO("Present mode: Mailbox");
				return availablePresentMode;
			}
		}
		VOE_CORE_INFO("Present mode: V-Sync");
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Swapchain::ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = m_WindowExtent;

			actualExtent.width = std::max(
				capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));

			actualExtent.height = std::max(
				capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}
	
	VkFormat Swapchain::FindDepthFormat()
	{
		return m_PhDevice->FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	void Swapchain::CreateImageView()
	{
		// Get the swap chain buffers containing the image and imageview
		m_Buffers.resize(GetSwapchainImageCount());
		for (uint32_t i = 0; i < GetSwapchainImageCount(); i++)
		{
			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = NULL;
			colorAttachmentView.format = m_SwapchainImageFormat;

			colorAttachmentView.components = {
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			};

			colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorAttachmentView.subresourceRange.baseMipLevel = 0;
			colorAttachmentView.subresourceRange.levelCount = 1;
			colorAttachmentView.subresourceRange.baseArrayLayer = 0;
			colorAttachmentView.subresourceRange.layerCount = 1;
			colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorAttachmentView.flags = 0;

			m_Buffers[i].image = m_SwapchainImages[i];

			colorAttachmentView.image = m_Buffers[i].image;

			VOE_CHECK_RESULT(vkCreateImageView(m_Device->GetVkDevice(), &colorAttachmentView, nullptr, &m_Buffers[i].view));
		}
	}

	void Swapchain::CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();
		m_SwapchainDepthFormat = depthFormat;
		VkExtent2D swapchainExtent = GetSwapchainExtent();

		m_DepthImages.resize(GetSwapchainImageCount());
		m_DepthImageMemories.resize(GetSwapchainImageCount());
		m_DepthImageViews.resize(GetSwapchainImageCount());

		for (int i = 0; i < m_DepthImages.size(); i++)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = m_SwapchainExtent.width;
			imageInfo.extent.height = m_SwapchainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			VOE_CHECK_RESULT(vkCreateImage(m_Device->GetVkDevice(), &imageInfo, nullptr, &m_DepthImages[i]))

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(m_Device->GetVkDevice(), m_DepthImages[i], &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = m_PhDevice->FindMemoryType(
				memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VOE_CHECK_RESULT(vkAllocateMemory(m_Device->GetVkDevice(), &allocInfo, nullptr, &m_DepthImageMemories[i]));
			VOE_CHECK_RESULT(vkBindImageMemory(m_Device->GetVkDevice(), m_DepthImages[i], m_DepthImageMemories[i], 0));

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_DepthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VOE_CHECK_RESULT(vkCreateImageView(m_Device->GetVkDevice(), &viewInfo, nullptr, &m_DepthImageViews[i]));
		}
	}
	
	VkResult Swapchain::AcquireNextImage(uint32_t* imageIndex)
	{
		return VkResult();
	}


}