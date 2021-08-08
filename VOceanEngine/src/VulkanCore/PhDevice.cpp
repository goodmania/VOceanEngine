#include "PreCompileHeader.h"
#include "PhDevice.h"

#include "VulkanCore/Surface.h"

namespace voe {
	
	PhDevice::PhDevice(const Instance* instance, const Surface* surface) : m_Instance(instance), m_Surface(surface)
	{
		uint32_t physicalDeviceCount;
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
		m_MsaaSamples = GetMaxUsableSampleCount();

#if defined(VOE_DEBUG)
		VOE_CORE_INFO("Selected Physical Device: ", m_Properties.deviceID, " ", std::quoted(m_Properties.deviceName), '\n');
#endif
	}

	VkPhysicalDevice PhDevice::ChoosePhysicalDevice(const std::vector<VkPhysicalDevice>& devices)
	{
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				break;
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
