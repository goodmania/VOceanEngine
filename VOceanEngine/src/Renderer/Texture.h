//#pragma once
//
///*
//* Vulkan texture loader
//*
//* Copyright(C) by Sascha Willems - www.saschawillems.de
//*
//* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
//*/
//
//namespace voe {
//
//	class Device;
//
//	class Texture
//	{	
//	protected:
//		void UpdateDescriptor();
//		void Destroy();
//
//		Device&					m_Device;
//		VkImage					m_Image;
//		VkImageLayout			m_ImageLayout;
//		VkDeviceMemory			m_DeviceMemory;
//		VkImageView				m_View;
//		uint32_t				m_Width, m_Height;
//		uint32_t				m_MipLevels;
//		uint32_t				m_LayerCount;
//		VkDescriptorImageInfo	m_Descriptor;
//		VkSampler				m_Sampler;
//	};
//
//	class Texture2D : public Texture
//	{
//	public:
//		void LoadFromFile(
//			std::string			filename,
//			VkFormat			format,
//			Device&				device,
//			VkQueue				copyQueue,
//			VkImageUsageFlags	imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
//			VkImageLayout		imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//			bool				forceLinear = false);
//
//		void FromBuffer(
//			void*				buffer,
//			VkDeviceSize		bufferSize,
//			VkFormat			format,
//			uint32_t			texWidth,
//			uint32_t			texHeight,
//			Device&				device,
//			VkQueue				copyQueue,
//			VkFilter			filter = VK_FILTER_LINEAR,
//			VkImageUsageFlags	imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
//			VkImageLayout		imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
//	};
//}
