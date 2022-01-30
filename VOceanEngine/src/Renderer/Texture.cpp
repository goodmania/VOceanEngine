#include "PreCompileHeader.h"
#include "Renderer/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/*
* Vulkan texture loader. Refered to Sascha Willems examples.  
*
* Copyright(C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

namespace voe {

	Texture::Texture()
	{

	}

	Texture::~Texture()
	{

	}
	void Texture::UpdateDescriptor()
	{
		m_Descriptor.sampler = m_Sampler;
		m_Descriptor.imageView = m_View;
		m_Descriptor.imageLayout = m_ImageLayout;
	}

	void Texture::Destroy()
	{
		vkDestroyImageView(m_Device->GetVkDevice(), m_View, nullptr);
		vkDestroyImage(m_Device->GetVkDevice(), m_Image, nullptr);
		if (m_Sampler)
		{
			vkDestroySampler(m_Device->GetVkDevice(), m_Sampler, nullptr);
		}
		vkFreeMemory(m_Device->GetVkDevice(), m_DeviceMemory, nullptr);
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

	Texture2D::Texture2D()
	{

	}

	Texture2D::~Texture2D()
	{
		Destroy();
	}

	void Texture2D::UpdateDescriptorImageLayout(VkImageLayout newImageLayout)
	{
		m_Descriptor.imageLayout = newImageLayout;
	}

	/**
	* Load a 2D texture including all mip levels
	*
	* @param filename File to load (supports .ktx)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
	*
	*/
	void Texture2D::LoadTextureFromFile(
		const char* filename,
		VkFormat format,
		Device& device,
		PhDevice& phDevice,
		VkQueue copyQueue,
		VkImageUsageFlags imageUsageFlags,
		VkImageLayout imageLayout,
		int textureChannel,
		bool forceLinear)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, textureChannel);

		if (!pixels)
		{
			throw std::runtime_error("failed to load texture image!");
		}

		m_Device = &device;
		m_Width = texWidth;
		m_Height = texHeight;
		m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		VkDeviceSize imageSize = texWidth * texHeight * textureChannel;

		// Get device properties for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(phDevice.GetVkPhysicalDevice(), format, &formatProperties);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 useStaging = !forceLinear;

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs = {};

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		if (useStaging)
		{
			// Create a host-visible staging buffer that contains the raw image data
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = imageSize;
			// This buffer is used as a transfer source for the buffer copy
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
			memcpy(data, pixels, imageSize);
			vkUnmapMemory(device.GetVkDevice(), stagingMemory);

			// Setup buffer copy regions for each mip level
			VkDeviceSize offset;
			std::vector<VkBufferImageCopy> bufferCopyRegions;

			for (uint32_t i = 0; i < m_MipLevels; i++)
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = std::max(1u, m_Width >> i);
				bufferCopyRegion.imageExtent.height = std::max(1u, m_Height >> i);
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);

				offset += (bufferCopyRegion.imageExtent.width) * (bufferCopyRegion.imageExtent.height) * textureChannel;
			}

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
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
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
		}
		else
		{
			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			// Check if this support is supported for linear tiling
			assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.extent = { m_Width, m_Height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// Load mip map level 0 to linear tiling image
			VOE_CHECK_RESULT(vkCreateImage(device.GetVkDevice(), &imageCreateInfo, nullptr, &mappableImage));

			// Get memory requirements for this image 
			// like size and alignment
			vkGetImageMemoryRequirements(device.GetVkDevice(), mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			memAllocInfo.allocationSize = memReqs.size;

			// Get memory type that can be mapped to host memory
			memAllocInfo.memoryTypeIndex = phDevice.FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// Allocate host memory
			VOE_CHECK_RESULT(vkAllocateMemory(device.GetVkDevice(), &memAllocInfo, nullptr, &mappableMemory));

			// Bind allocated image for use
			VOE_CHECK_RESULT(vkBindImageMemory(device.GetVkDevice(), mappableImage, mappableMemory, 0));

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subRes.mipLevel = 0;

			VkSubresourceLayout subResLayout;
			void* data;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(device.GetVkDevice(), mappableImage, &subRes, &subResLayout);

			// Map image memory
			VOE_CHECK_RESULT(vkMapMemory(device.GetVkDevice(), mappableMemory, 0, memReqs.size, 0, &data));

			// Copy image data into memory
			memcpy(data, pixels, memReqs.size);

			vkUnmapMemory(device.GetVkDevice(), mappableMemory);

			// Linear tiled images don't need to be staged
			// and can be directly used as textures
			m_Image = mappableImage;
			m_DeviceMemory = mappableMemory;
			this->m_ImageLayout = imageLayout;

			// Setup image memory barrier
			tools::setImageLayout(copyCmd, m_Image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

			device.FlushCommandBuffer(copyCmd, copyQueue, true);
		}

		stbi_image_free(pixels);

		// Create a default sampler
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;

		// Max level-of-detail should match mip level count
		samplerCreateInfo.maxLod = (useStaging) ? (float)m_MipLevels : 0.0f;

		// Only enable anisotropic filtering if enabled on the device
		samplerCreateInfo.maxAnisotropy = phDevice.GetFeatures().samplerAnisotropy ? phDevice.GetProperties().limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.anisotropyEnable = phDevice.GetFeatures().samplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VOE_CHECK_RESULT(vkCreateSampler(device.GetVkDevice(), &samplerCreateInfo, nullptr, &m_Sampler));

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		viewCreateInfo.subresourceRange.levelCount = (useStaging) ? m_MipLevels : 1;
		viewCreateInfo.image = m_Image;
		VOE_CHECK_RESULT(vkCreateImageView(device.GetVkDevice(), &viewCreateInfo, nullptr, &m_View));

		// Update descriptor image info member that can be used for setting up descriptor sets
		UpdateDescriptor();
	}

	void Texture2D::CreateTextureFromBuffer(
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
		m_Device = &device;
		m_Width = texWidth;
		m_Height = texHeight;
		m_MipLevels = 1;

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs = {};

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

