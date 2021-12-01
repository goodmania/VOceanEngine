#include "PreCompileHeader.h"
#include "Renderer/Texture.h"

/*
* Vulkan texture loader
*
* Copyright(C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

namespace voe {

	void Texture::UpdateDescriptor()
	{
		m_Descriptor.sampler = m_Sampler;
		m_Descriptor.imageView = m_View;
		m_Descriptor.imageLayout = m_ImageLayout;
	}

	void Texture::Destroy()
	{
		vkDestroyImageView(m_Device.GetVkDevice(), m_View, nullptr);
		vkDestroyImage(m_Device.GetVkDevice(), m_Image, nullptr);
		if (m_Sampler)
		{
			vkDestroySampler(m_Device.GetVkDevice(), m_Sampler, nullptr);
		}
		vkFreeMemory(m_Device.GetVkDevice(), m_DeviceMemory, nullptr);
	}

	/*
	*
	* Creates a 2D texture from a buffer
	*
	* @param buffer Buffer containing texture data to upload
	* @param bufferSize Size of the buffer in machine units
	* @param width Width of the texture to create
	* @param height Height of the texture to create
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) filter Texture filtering for the sampler (defaults to VK_FILTER_LINEAR)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*/

	void Texture2D::FromBuffer(
		void* buffer,
		VkDeviceSize bufferSize,
		VkFormat format,
		uint32_t texWidth,
		uint32_t texHeight,
		Device& device,
		PhDevice& phDevice,
		VkQueue copyQueue,
		VkFilter filter,
		VkImageUsageFlags imageUsageFlags,
		VkImageLayout imageLayout)
	{
		VOE_CORE_ASSERT(buffer);

		m_Device = device;
		m_Width = texWidth;
		m_Height = texHeight;
		m_MipLevels = 1;

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// Create a host-visible staging buffer that contains the raw image data
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		// This buffer is used as a transfer source for the buffer copy
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VOE_CHECK_RESULT(vkCreateBuffer(device.GetVkDevice(), &bufferCreateInfo, nullptr, &stagingBuffer));

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vkGetBufferMemoryRequirements(device.GetVkDevice(), stagingBuffer, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = phDevice.FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VOE_CHECK_RESULT(vkAllocateMemory(device.GetVkDevice(), &memAllocInfo, nullptr, &stagingMemory));
		VOE_CHECK_RESULT(vkBindBufferMemory(device.GetVkDevice(), stagingBuffer, stagingMemory, 0));

		// Copy texture data into staging buffer
		uint8_t* data;
		VOE_CHECK_RESULT(vkMapMemory(device.GetVkDevice(), stagingMemory, 0, memReqs.size, 0, (void**)&data));
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(device.GetVkDevice(), stagingMemory);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = m_Width;
		bufferCopyRegion.imageExtent.height = m_Height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = m_MipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { m_Width, m_Height, 1 };
		imageCreateInfo.usage = imageUsageFlags;

		// Ensure that the TRANSFER_DST bit is set for staging
		if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VOE_CHECK_RESULT(vkCreateImage(device.GetVkDevice(), &imageCreateInfo, nullptr, &m_Image));

		vkGetImageMemoryRequirements(device.GetVkDevice(), m_Image, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;

		memAllocInfo.memoryTypeIndex = phDevice.FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VOE_CHECK_RESULT(vkAllocateMemory(device.GetVkDevice(), &memAllocInfo, nullptr, &m_DeviceMemory));
		VOE_CHECK_RESULT(vkBindImageMemory(device.GetVkDevice(), m_Image, m_DeviceMemory, 0));

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = m_MipLevels;
		subresourceRange.layerCount = 1;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		tools::setImageLayout(
			copyCmd,
			m_Image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		// Change texture image layout to shader read after all mip levels have been copied
		this->m_ImageLayout = imageLayout;
		tools::setImageLayout(
			copyCmd,
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		device.FlushCommandBuffer(copyCmd, copyQueue, true);

		// Clean up staging resources
		vkFreeMemory(device.GetVkDevice(), stagingMemory, nullptr);
		vkDestroyBuffer(device.GetVkDevice(), stagingBuffer, nullptr);

		// Create sampler
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = filter;
		samplerCreateInfo.minFilter = filter;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		VOE_CHECK_RESULT(vkCreateSampler(device.GetVkDevice(), &samplerCreateInfo, nullptr, &m_Sampler));

		// Create image view
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.pNext = NULL;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.image = m_Image;
		VOE_CHECK_RESULT(vkCreateImageView(device.GetVkDevice(), &viewCreateInfo, nullptr, &m_View));

		// Update descriptor image info member that can be used for setting up descriptor sets
		UpdateDescriptor();
	}
}

