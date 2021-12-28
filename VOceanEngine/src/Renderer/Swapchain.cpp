#include "PreCompileHeader.h"
#include "VulkanCore/VulkanCoreHeader.h"

#include "Swapchain.h"

namespace voe {

	Swapchain::Swapchain(Device* device, PhDevice* phDevice, Surface* surface, VkExtent2D windowExtent, std::shared_ptr<Swapchain> previous)
		: m_Device(device), m_PhDevice(phDevice), m_Surface(surface), m_WindowExtent(windowExtent), m_OldSwapchain(previous)
	{
		InitSwapchain();
		m_OldSwapchain = nullptr;
	}

	Swapchain::Swapchain(Device* device, PhDevice* phDevice, Surface* surface, VkExtent2D windowExtent)
		: m_Device(device), m_PhDevice(phDevice), m_Surface(surface), m_WindowExtent(windowExtent)
	{
		InitSwapchain();
	}

	Swapchain::~Swapchain()
	{
		for (auto imageView : m_SwapchainImageViews)
		{
			vkDestroyImageView(m_Device->GetVkDevice(), imageView, nullptr);
		}

		m_SwapchainImageViews.clear();

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

		for (auto framebuffer : m_Framebuffers) 
		{
			vkDestroyFramebuffer(m_Device->GetVkDevice(), framebuffer, nullptr);
		}

		vkDestroyRenderPass(m_Device->GetVkDevice(), m_RenderPass, nullptr);
	}

	void Swapchain::InitSwapchain()
	{
		CreateSwapchain();
		CreateImageView();
		CreateDepthResources();
		CreateRenderPass();
		CreateFramebuffers();
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
		std::array<uint32_t, 2> queueFamilyIndices = { indices.graphicsFamily, indices.presentFamily };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
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
		m_SwapchainImageViews.resize(GetSwapchainImageCount());
		for (uint32_t i = 0; i < GetSwapchainImageCount(); i++)
		{
			VkImageViewCreateInfo colorAttachmentView = {};
			colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorAttachmentView.pNext = nullptr;
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
			colorAttachmentView.image = m_SwapchainImages[i];

			VOE_CHECK_RESULT(vkCreateImageView(m_Device->GetVkDevice(), &colorAttachmentView, nullptr, &m_SwapchainImageViews[i]));
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

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = m_PhDevice->FindMemoryType(
				memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VOE_CHECK_RESULT(vkAllocateMemory(m_Device->GetVkDevice(), &allocInfo, nullptr, &m_DepthImageMemories[i]));
			VOE_CHECK_RESULT(vkBindImageMemory(m_Device->GetVkDevice(), m_DepthImages[i], m_DepthImageMemories[i], 0));

			VkImageViewCreateInfo viewInfo = {};
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

	void Swapchain::CreateRenderPass()
	{
		auto sampleCount = m_PhDevice->GetMsaaSamples();

		if (sampleCount != VK_SAMPLE_COUNT_1_BIT)
		{
			std::array<VkAttachmentDescription, 4> attachments = {};

			// Multisampled attachment that we render to
			attachments[0].format = m_SwapchainImageFormat;
			attachments[0].samples = sampleCount;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// This is the frame buffer attachment to where the multisampled image
			// will be resolved to and which will be presented to the swapchain
			attachments[1].format = m_SwapchainImageFormat;
			attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			// Multisampled depth attachment we render to
			attachments[2].format = m_SwapchainImageFormat;
			attachments[2].samples = sampleCount;
			attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Depth resolve attachment
			attachments[3].format = m_SwapchainDepthFormat;
			attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 2;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Resolve attachment reference for the color attachment
			VkAttachmentReference resolveReference = {};
			resolveReference.attachment = 1;
			resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorReference;
			// Pass our resolve attachments to the sub pass
			subpass.pResolveAttachments = &resolveReference;
			subpass.pDepthStencilAttachment = &depthReference;

			std::array<VkSubpassDependency, 2> dependencies = {};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassCI = {};
			renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCI.pAttachments = attachments.data();
			renderPassCI.subpassCount = 1;
			renderPassCI.pSubpasses = &subpass;
			renderPassCI.dependencyCount = 2;
			renderPassCI.pDependencies = dependencies.data();
			VOE_CHECK_RESULT(vkCreateRenderPass(m_Device->GetVkDevice(), &renderPassCI, nullptr, &m_RenderPass));
		}
		else
		{
			std::array<VkAttachmentDescription, 2> attachments = {};
			// Color attachment
			attachments[0].format = m_SwapchainImageFormat;
			attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			// Depth attachment
			attachments[1].format = m_SwapchainDepthFormat;
			attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpassDescription = {};
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &colorReference;
			subpassDescription.pDepthStencilAttachment = &depthReference;
			subpassDescription.inputAttachmentCount = 0;
			subpassDescription.pInputAttachments = nullptr;
			subpassDescription.preserveAttachmentCount = 0;
			subpassDescription.pPreserveAttachments = nullptr;
			subpassDescription.pResolveAttachments = nullptr;

			// Subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies = {};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassCI = {};
			renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCI.pAttachments = attachments.data();
			renderPassCI.subpassCount = 1;
			renderPassCI.pSubpasses = &subpassDescription;
			renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCI.pDependencies = dependencies.data();

			VOE_CHECK_RESULT(vkCreateRenderPass(m_Device->GetVkDevice(), &renderPassCI, nullptr, &m_RenderPass));
		}
	}

	void Swapchain::CreateFramebuffers()
	{
		m_Framebuffers.resize(GetSwapchainImageCount());
		for (size_t i = 0; i < GetSwapchainImageCount(); i++) 
		{
			std::array<VkImageView, 2> attachments = { m_SwapchainImageViews[i], m_DepthImageViews[i] };

			VkExtent2D swapChainExtent = GetSwapchainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VOE_CHECK_RESULT(
				vkCreateFramebuffer(
					m_Device->GetVkDevice(),
					&framebufferInfo,
					nullptr,
					&m_Framebuffers[i]));
		}
	}
	
	VkResult Swapchain::AcquireNextImage(const VkSemaphore& presentCompleteSemaphore, VkFence fence, uint32_t* imageIndex)
	{
		if (fence != VK_NULL_HANDLE)
			VOE_CHECK_RESULT(vkWaitForFences(m_Device->GetVkDevice(), 1, &fence, VK_TRUE, std::numeric_limits<uint64_t>::max()));

		auto acquireResult = vkAcquireNextImageKHR(
			m_Device->GetVkDevice(),
			m_Swapchain,
			std::numeric_limits<uint64_t>::max(),
			presentCompleteSemaphore,
			VK_NULL_HANDLE,
			imageIndex);

		return acquireResult;
	}
}