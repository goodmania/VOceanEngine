#include "PreCompileHeader.h"
#include "Device.h"

#include "VulkanCore/Tools.h"
#include "VulkanCore/Instance.h"
#include "VulkanCore/PhDevice.h"
#include "VulkanCore/Surface.h"

namespace voe {

	Device::Device(const Instance* instance, PhDevice* phDevice, const Surface* surface)
		: m_Instance(instance), m_PhDevice(phDevice), m_Surface(surface)
	{
        QueueFamilyIndices indices = phDevice->FindQueueFamilies(phDevice->GetVkPhysicalDevice());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

        // Within the same device, queues with higher priority may be allotted more processing time
        // than queues with lower priority.
        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(phDevice->DeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = phDevice->DeviceExtensions.data();

#ifdef VOE_DEBUG
        createInfo.enabledLayerCount = static_cast<uint32_t>(instance->ValidationLayers.size());
        createInfo.ppEnabledLayerNames = instance->ValidationLayers.data();
#endif // VOE_DEBUG

        createInfo.enabledLayerCount = 0;

        VOE_CHECK_RESULT(vkCreateDevice(phDevice->GetVkPhysicalDevice(), &createInfo, nullptr, &m_Device));

        vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.presentFamily,  0, &m_PresentQueue);
	}

	Device::~Device()
	{
        vkDestroyDevice(m_Device, nullptr);
	}
}