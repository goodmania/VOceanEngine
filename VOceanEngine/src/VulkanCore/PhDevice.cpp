#include "PreCompileHeader.h"
#include "PhDevice.h"

#include "VulkanCore/Surface.h"

namespace voe {

	PhDevice::PhDevice(const Instance* instance, const Surface* surface) : m_Instance(instance), m_Surface(surface)
	{
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(instance->GetVkInstance(), &physicalDeviceCount, nullptr);

		if (physicalDeviceCount == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance->GetVkInstance(), &physicalDeviceCount, physicalDevices.data());

		m_PhysicalDevice = ChoosePhysicalDevice(physicalDevices);

		if (!m_PhysicalDevice)
			throw std::runtime_error("Vulkan runtime error, failed to find a suitable GPU");

		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_Properties);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_EnabledFeatures);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);
		if (m_Instance->EnableMultiSampling())
		{
			m_MsaaSamples = GetMaxUsableSampleCount();
		}

#if defined(VOE_DEBUG)
		VOE_CORE_INFO("\n Physical Device: {}  \n DeviceID: {}", m_Properties.deviceName, m_Properties.deviceID);
		//VOE_CORE_INFO("\n Physical Device: {}", m_Properties.limits.minUniformBufferOffsetAlignment);
#endif
	}

	VkPhysicalDevice PhDevice::ChoosePhysicalDevice(const std::vector<VkPhysicalDevice>& devices)
	{
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				return device;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	bool PhDevice::IsDeviceSuitable(const VkPhysicalDevice& device)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device);

		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false;

		if (extensionsSupported)
		{
			SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device);
			swapChainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices PhDevice::FindQueueFamilies(const VkPhysicalDevice& device)
	{
		QueueFamilyIndices indices;
		uint32_t queueFamilyCount = 0;

		// the number of queue families is returned in pQueueFamilyCount.
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

		// retrieve details about the queue families and queues supported by a device
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
				indices.graphicsFamilyHasValue = true;
			}

			VkBool32 presentSupport = false;

			// determine whether a queue family of a physical device supports presentation to a given surface(glfw surface)
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface->GetVkSurface(), &presentSupport);

			if (queueFamily.queueCount > 0 && presentSupport)
			{
				indices.presentFamily = i;
				indices.presentFamilyHasValue = true;
			}

			// Check for compute support.
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				indices.computeFamily = i;
				indices.computeFamilyHasValue = true;
			}

			// Check for transfer support.
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				indices.transferFamily = i;
				indices.transferFamilyHasValue = true;
			}

			if (indices.IsComplete())
			{
				break;
			}
			i++;
		}
		return indices;
	}

	bool PhDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice& device)
	{
		uint32_t extensionCount = 0;

		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> properties(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, properties.data());

		std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

		for (const auto& extension : properties)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	SwapchainSupportDetails PhDevice::QuerySwapchainSupport(VkPhysicalDevice device)
	{
		SwapchainSupportDetails details;

		// query the basic capabilities of a surface,
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface->GetVkSurface(), &details.capabilities);

		// query the supported swapchain format-color space pairs for a surface
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface->GetVkSurface(), &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface->GetVkSurface(), &formatCount, details.formats.data());
		}

		//query the supported presentation modes for a surface
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface->GetVkSurface(), &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				m_Surface->GetVkSurface(),
				&presentModeCount,
				details.presentModes.data());
		}
		return details;
	}

	VkFormat PhDevice::FindSupportedFormat(
		const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	uint32_t PhDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	uint32_t PhDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkBool32& foundMemoryType)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & 1) == 1)
			{
				if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					if (foundMemoryType)
					{
						foundMemoryType = true;
					}
					return i;
				}
			}
			typeFilter >>= 1;
		}

		if (foundMemoryType)
		{
			foundMemoryType = false;
			return 0;
		}
		else
		{
			throw std::runtime_error("Could not find a matching memory type");
		}
	}

	VkSampleCountFlagBits PhDevice::GetMaxUsableSampleCount() const
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

		auto counts = std::min(
			physicalDeviceProperties.limits.framebufferColorSampleCounts,
			physicalDeviceProperties.limits.framebufferDepthSampleCounts);

		for (const auto& sampleFlag : STAGE_FLAG_BITS)
		{
			if (counts & sampleFlag)
				return sampleFlag;
		}
		return VK_SAMPLE_COUNT_1_BIT;
	}
}