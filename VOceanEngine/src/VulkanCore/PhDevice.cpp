#include "PreCompileHeader.h"
#include "PhDevice.h"

namespace voe {
	
	PhDevice::PhDevice(const Instance* instance) : m_Instance(instance)
	{
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(GetNativeInstance(), &physicalDeviceCount, nullptr);

		if (physicalDeviceCount == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(GetNativeInstance(), &physicalDeviceCount, physicalDevices.data());

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

	uint32_t PhDevice::IsDeviceSuitable(const VkPhysicalDevice& device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;

		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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

	void PhDevice::LogVulkanDevice(
		const VkPhysicalDeviceProperties& physicalDeviceProperties,
		const std::vector<VkExtensionProperties>& extensionProperties) 
	{
		std::stringstream ss;
		switch (static_cast<int32_t>(physicalDeviceProperties.deviceType)) 
		{
		case 1:
			ss << "Integrated";
			break;
		case 2:
			ss << "Discrete";
			break;
		case 3:
			ss << "Virtual";
			break;
		case 4:
			ss << "CPU";
			break;
		default:
			ss << "Other " << physicalDeviceProperties.deviceType;
		}

		ss << " Physical Device: " << physicalDeviceProperties.deviceID;
		switch (physicalDeviceProperties.vendorID) 
		{
		case 0x8086:
			ss << " \"Intel\"";
			break;
		case 0x10DE:
			ss << " \"Nvidia\"";
			break;
		case 0x1002:
			ss << " \"AMD\"";
			break;
		default:
			ss << " \"" << physicalDeviceProperties.vendorID << '\"';
		}

		ss << " " << std::quoted(physicalDeviceProperties.deviceName) << '\n';

		uint32_t supportedVersion[3] = 
		{
			VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
			VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
			VK_VERSION_PATCH(physicalDeviceProperties.apiVersion)
		};
		ss << "API Version: " << supportedVersion[0] << "." << supportedVersion[1] << "." << supportedVersion[2] << '\n';

		ss << "Extensions: ";
		for (const auto& extension : extensionProperties)
			ss << extension.extensionName << ", ";

		ss << "\n\n";
		VOE_CORE_INFO(ss.str());
	}
}
