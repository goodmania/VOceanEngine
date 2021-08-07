#pragma once

#include "VulkanCore/Instance.h"

namespace voe {

	static const std::vector<VkSampleCountFlagBits> STAGE_FLAG_BITS = {
	VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_16_BIT,
	VK_SAMPLE_COUNT_8_BIT,	VK_SAMPLE_COUNT_4_BIT,	VK_SAMPLE_COUNT_2_BIT
	};

	class VOE_API PhDevice 
	{
	public:
		explicit PhDevice(const Instance* instance);

		const VkPhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
		const VkPhysicalDeviceFeatures& GetFeatures() const { return m_EnabledFeatures; }
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }
		const VkSampleCountFlagBits& GetMsaaSamples() const { return m_MsaaSamples; }

	private:
		VkPhysicalDevice ChoosePhysicalDevice(const std::vector<VkPhysicalDevice>& devices);
		static uint32_t IsDeviceSuitable(const VkPhysicalDevice& device);
		VkSampleCountFlagBits GetMaxUsableSampleCount() const;

		static void LogVulkanDevice(const VkPhysicalDeviceProperties& physicalDeviceProperties, const std::vector<VkExtensionProperties>& extensionProperties);

		const Instance* m_Instance;
		VkInstance GetNativeInstance() const { return m_Instance->GetVoeInstance(); }

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_Properties;
		VkPhysicalDeviceFeatures m_EnabledFeatures;
		VkPhysicalDeviceMemoryProperties m_MemoryProperties = {};
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
	};
}