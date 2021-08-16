#include "PreCompileHeader.h"
#include "Device.h"

#include "VulkanCore/Tools.h"
#include "VulkanCore/Instance.h"
#include "VulkanCore/Surface.h"

namespace voe {

	Device::Device(const Instance* instance, PhDevice* phDevice, const Surface* surface)
		: m_Instance(instance), m_PhDevice(phDevice), m_Surface(surface)
	{
        CreateDevice();
        CreateCommandPool();
	}

    void Device::CreateDevice()
    {
        QueueFamilyIndices indices = m_PhDevice->FindQueueFamilies(m_PhDevice->GetVkPhysicalDevice());

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

        if (indices.computeFamilyHasValue && indices.computeFamily != indices.graphicsFamily)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.computeFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        else
        {
            indices.computeFamily = indices.graphicsFamily;
        }

        if (indices.transferFamilyHasValue
            && indices.transferFamily != indices.graphicsFamily
            && indices.transferFamily != indices.computeFamily)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices.transferFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        else
        {
            indices.transferFamily = indices.graphicsFamily;
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_PhDevice->DeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = m_PhDevice->DeviceExtensions.data();

#ifdef VOE_DEBUG
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_Instance->ValidationLayers.size());
        createInfo.ppEnabledLayerNames = m_Instance->ValidationLayers.data();
#endif // VOE_DEBUG

        createInfo.enabledLayerCount = 0;

        VOE_CHECK_RESULT(vkCreateDevice(m_PhDevice->GetVkPhysicalDevice(), &createInfo, nullptr, &m_Device));

        vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.presentFamily, 0, &m_PresentQueue);
        vkGetDeviceQueue(m_Device, indices.computeFamily, 0, &m_ComputeQueue);
        vkGetDeviceQueue(m_Device, indices.transferFamily, 0, &m_TransferQueue);

        // for geter function
        m_Indices = indices;
    }

	Device::~Device()
	{
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        vkDestroyDevice(m_Device, nullptr);
	}

    void Device::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VOE_CHECK_RESULT(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = m_PhDevice->FindMemoryType(memRequirements.memoryTypeBits, properties);

        VOE_CHECK_RESULT(vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory));
        vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
    }

    void Device::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_GraphicsQueue);

        vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
    }

    void Device::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
    {

    }

    void Device::CreateCommandPool()
    {
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = GetGraphicsQueueFamily();
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VOE_CHECK_RESULT(vkCreateCommandPool(GetVkDevice(), &cmdPoolInfo, nullptr, &m_CommandPool));
    }
}