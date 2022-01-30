#pragma once
#include "VulkanCore/Device.h"

/*
* Vulkan texture loader
*
* Copyright(C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

namespace voe {

	class Texture
	{	
	public:
		Texture();
		~Texture();
		virtual void UpdateDescriptor();

	protected:
		void Destroy();

		Device*					m_Device;
		VkImage					m_Image;
		VkImageLayout			m_ImageLayout;
		VkDeviceMemory			m_DeviceMemory;
		VkImageView				m_View;
		uint32_t				m_Width, m_Height;
		uint32_t				m_MipLevels;
		uint32_t				m_LayerCount;
		VkDescriptorImageInfo	m_Descriptor;
		VkSampler				m_Sampler;
	};

	class Texture2D : public Texture
	{
	public:
		Texture2D();
		~Texture2D();

		void UpdateDescriptorImageLayout(VkImageLayout newImageLayout);

		VkDescriptorImageInfo* GetDescriptorImageInfo() { return &m_Descriptor; }
		const VkImage GetImage() const { return m_Image; }
		const VkImageLayout GetCurrentImageLayout() const{ return m_ImageLayout; }

		void Texture2D::LoadTextureFromFile(
			const char* filename,
			VkFormat format,
			Device& device,
			PhDevice& phDevice,
			VkQueue copyQueue,
			VkImageUsageFlags imageUsageFlags,
			VkImageLayout imageLayout,
			int textureChannel,
			bool forceLinear);

		void CreateTextureFromBuffer(
			void*				buffer,
			VkDeviceSize		bufferSize,
			VkFormat			format,
			uint32_t			texWidth,
			uint32_t			texHeight,
			Device&				device,
			PhDevice&			phDevice,
			VkQueue				copyQueue,
			VkFilter			filter = VK_FILTER_LINEAR,
			VkImageUsageFlags	imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout		imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};
}
