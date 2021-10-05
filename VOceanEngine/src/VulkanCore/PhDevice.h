#pragma once

#include "VulkanCore/Instance.h"

namespace voe {

	class Surface;

	static const std::vector<VkSampleCountFlagBits> STAGE_FLAG_BITS = {
	VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_16_BIT,
	VK_SAMPLE_COUNT_8_BIT,	VK_SAMPLE_COUNT_4_BIT,	VK_SAMPLE_COUNT_2_BIT
	};

	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily;
		uint32_t presentFamily;
		uint32_t computeFamily;
		uint32_t transferFamily;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;
		bool computeFamilyHasValue = false;
		bool transferFamilyHasValue = false;
		bool IsComplete() { return graphicsFamilyHasValue && presentFamilyHasValue && computeFamilyHasValue && transferFamily; }
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VOE_API PhDevice 
	{
	public:
		const std::vector<const char*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		PhDevice(const Instance* instance, const Surface* surface);

		const VkPhysicalDevice& GetVkPhysicalDevice() const { return m_PhysicalDevice; }
		const VkPhysicalDeviceProperties& GetProperties() const { return m_Properties; }
		const VkPhysicalDeviceFeatures& GetFeatures() const { return m_EnabledFeatures; }
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_MemoryProperties; }
		const SwapchainSupportDetails GetSwapchainSupport() { return QuerySwapchainSupport(m_PhysicalDevice); }
		const VkSampleCountFlagBits& GetMsaaSamples() const { return m_MsaaSamples; }
		QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device);
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkBool32& foundMemoryType);

	private:
		VkPhysicalDevice ChoosePhysicalDevice(const std::vector<VkPhysicalDevice>& devices);
		bool IsDeviceSuitable(const VkPhysicalDevice& device);
		bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device);
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
		VkSampleCountFlagBits GetMaxUsableSampleCount() const;

		const Instance* m_Instance;
		const Surface* m_Surface;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_Properties;
		VkPhysicalDeviceFeatures m_EnabledFeatures;
		VkPhysicalDeviceMemoryProperties m_MemoryProperties = {};
		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
	};
}