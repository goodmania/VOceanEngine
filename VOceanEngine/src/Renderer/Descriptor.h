/*
The MIT License(MIT)

Copyright(c) 2016 Patrick Marsceill
https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.h

Permission is hereby granted, free of charge, to any person obtaining a copy
of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

namespace voe{

	class DescriptorAllocator
	{
	public:
		struct PoolSizes 
		{
			std::vector<std::pair<VkDescriptorType, float>> Sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		void ResetPools();
		bool Allocate(VkDescriptorSet* descriptorSet, VkDescriptorSetLayout layout);
		void Init(VkDevice& device);
		void DestroyPools();
		const VkDevice GetVkDevice() const { return m_Device; }

	private:
		VkDescriptorPool PickPool();

		VkDescriptorPool m_CurrentPool{ VK_NULL_HANDLE };
		PoolSizes m_DescriptorSizes;
		std::vector<VkDescriptorPool> m_UsedPools;
		std::vector<VkDescriptorPool> m_FreePools;
		VkDevice& m_Device;
	};

	class DescriptorLayoutCache
	{
	public:
		void Init(VkDevice& device);
		void DestroyLayout();
		VkDescriptorSetLayout CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo
		{
			//good idea to turn this into a inlined array
			std::vector<VkDescriptorSetLayoutBinding> Bindings;
			bool operator==(const DescriptorLayoutInfo& other) const;
			size_t Hash() const;
		};

	private:
		struct DescriptorLayoutHash
		{

			std::size_t operator()(const DescriptorLayoutInfo& k) const
			{
				return k.Hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
		VkDevice& m_Device;
	};

	class DescriptorBuilder 
	{
	public:
		static DescriptorBuilder Begin(
			DescriptorLayoutCache*	layoutCache,
			DescriptorAllocator*	allocator);

		DescriptorBuilder& BindBuffer(
			uint32_t				binding,
			VkDescriptorBufferInfo* bufferInfo,
			VkDescriptorType		type,
			VkShaderStageFlags		stageFlags);

		DescriptorBuilder& BindImage(
			uint32_t				binding,
			VkDescriptorImageInfo*	imageInfo,
			VkDescriptorType		type,
			VkShaderStageFlags		stageFlags);

		bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
		bool Build(VkDescriptorSet& set);

	private:
		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorSetLayoutBinding> m_Bindings;

		DescriptorLayoutCache* m_Cache;
		DescriptorAllocator* m_Alloc;
	};
}



