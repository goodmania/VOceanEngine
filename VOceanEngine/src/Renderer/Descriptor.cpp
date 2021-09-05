/*
The MIT License(MIT)

Copyright(c) 2016 Patrick Marsceill
https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.cpp

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

#include "PreCompileHeader.h"
#include "Renderer/Descriptor.h"

namespace voe {

	VkDescriptorPool CreateDescriptorPool(
		VkDevice device,
		const DescriptorAllocator::PoolSizes& poolSizes,
		int count,
		VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(poolSizes.Sizes.size());

		for (auto sz : poolSizes.Sizes) 
		{
			sizes.push_back({ sz.first, uint32_t(sz.second * count) });
		}

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = flags;
		poolInfo.maxSets = count;
		poolInfo.poolSizeCount = (uint32_t)sizes.size();
		poolInfo.pPoolSizes = sizes.data();

		VkDescriptorPool descriptorPool;
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

		return descriptorPool;
	}

	// -- Descriptor Allocator class methods --

	DescriptorAllocator::DescriptorAllocator(const VkDevice& device) : m_Device(device)
	{

	}

	DescriptorAllocator::~DescriptorAllocator()
	{
		DestroyPools();
	}

	void DescriptorAllocator::DestroyPools()
	{
		for (auto freePool : m_FreePools)
		{
			vkDestroyDescriptorPool(m_Device, freePool, nullptr);
		}
		for (auto userPool : m_UsedPools)
		{
			vkDestroyDescriptorPool(m_Device, userPool, nullptr);
		}
	}

	void DescriptorAllocator::ResetPools()
	{
		for (auto pool : m_UsedPools)
		{
			vkResetDescriptorPool(m_Device, pool, 0);
		}

		m_FreePools = m_UsedPools;
		m_UsedPools.clear();
		m_CurrentPool = VK_NULL_HANDLE;
	}

	bool DescriptorAllocator::Allocate(VkDescriptorSet* descriptorSet, VkDescriptorSetLayout layout)
	{
		if (m_CurrentPool == VK_NULL_HANDLE)
		{
			m_CurrentPool = PickPool();
			m_UsedPools.push_back(m_CurrentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorPool = m_CurrentPool;
		allocInfo.descriptorSetCount = 1;

		VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, descriptorSet);

		if (result == VK_SUCCESS) 
			return true;

		if (result == (VK_ERROR_FRAGMENTED_POOL || VK_ERROR_OUT_OF_POOL_MEMORY))
		{
			//allocate a new pool and retry
			m_CurrentPool = PickPool();
			m_UsedPools.push_back(m_CurrentPool);

			result = vkAllocateDescriptorSets(m_Device, &allocInfo, descriptorSet);

			//if it still fails then we have big issues
			if (result == VK_SUCCESS)
				return true;
		}
		return false;
	}

	VkDescriptorPool DescriptorAllocator::PickPool()
	{
		if (m_FreePools.size() > 0)
		{
			VkDescriptorPool pool = m_FreePools.back();
			m_FreePools.pop_back();
			return pool;
		}
		else 
		{
			return CreateDescriptorPool(m_Device, m_DescriptorSizes, 1000, 0);
		}
	}

	// -- Descriptor LayoutCache class methods --

	DescriptorLayoutCache::DescriptorLayoutCache(const VkDevice& device) : m_Device(device)
	{

	}

	DescriptorLayoutCache::~DescriptorLayoutCache()
	{
		DestroyLayout();
	}

	VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(VkDescriptorSetLayoutCreateInfo* info)
	{
		DescriptorLayoutInfo layoutInfo = {};
		layoutInfo.Bindings.reserve(info->bindingCount);
		bool isSorted = true;
		int32_t lastBinding = -1;
		for (uint32_t i = 0; i < info->bindingCount; i++) 
		{
			layoutInfo.Bindings.push_back(info->pBindings[i]);

			//check that the bindings are in strict increasing order
			if (static_cast<int32_t>(info->pBindings[i].binding) > lastBinding)
			{
				lastBinding = info->pBindings[i].binding;
			}
			else 
			{
				isSorted = false;
			}
		}
		if (!isSorted)
		{
			std::sort(
				layoutInfo.Bindings.begin(),
				layoutInfo.Bindings.end(),
				[](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b) 
				{
					return a.binding < b.binding;
				});
		}

		auto it = layoutCache.find(layoutInfo);
		if (it != layoutCache.end())
		{
			return (*it).second;
		}
		else 
		{
			VkDescriptorSetLayout layout;
			vkCreateDescriptorSetLayout(m_Device, info, nullptr, &layout);

			//layoutCache.emplace()
			//add to cache
			layoutCache[layoutInfo] = layout;
			return layout;
		}
	}

	void DescriptorLayoutCache::DestroyLayout()
	{
		//delete every descriptor layout held
		for (auto pair : layoutCache)
		{
			vkDestroyDescriptorSetLayout(m_Device, pair.second, nullptr);
		}
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
	{
		if (other.Bindings.size() != Bindings.size())
		{
			return false;
		}
		else
		{
			//compare each of the bindings is the same. Bindings are sorted so they will match
			for (int i = 0; i < Bindings.size(); i++) 
			{
				if (other.Bindings[i].binding != Bindings[i].binding)
				{
					return false;
				}
				if (other.Bindings[i].descriptorType != Bindings[i].descriptorType)
				{
					return false;
				}
				if (other.Bindings[i].descriptorCount != Bindings[i].descriptorCount)
				{
					return false;
				}
				if (other.Bindings[i].stageFlags != Bindings[i].stageFlags)
				{
					return false;
				}
			}
			return true;
		}
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
	{
		using std::size_t;
		using std::hash;

		size_t result = hash<size_t>()(Bindings.size());

		for (const VkDescriptorSetLayoutBinding& b : Bindings)
		{
			//pack the binding data into a single int64. Not fully correct but its ok
			size_t binding_hash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;

			//shuffle the packed binding data and xor it with the main hash
			result ^= hash<size_t>()(binding_hash);
		}

		return result;
	}

	// -- Descriptor Builder class methods --

	DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
	{
		DescriptorBuilder builder;

		builder.m_Cache = layoutCache;
		builder.m_Alloc = allocator;
		return builder;
	}

	DescriptorBuilder& DescriptorBuilder::BindBuffer(
		uint32_t binding,
		VkDescriptorBufferInfo* bufferInfo,
		VkDescriptorType type, 
		VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding = {};

		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;
		newBinding.binding = binding;

		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite = {};

		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;
		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pBufferInfo = bufferInfo;
		newWrite.dstBinding = binding;

		m_Writes.push_back(newWrite);
		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::BindImage(
		uint32_t binding,
		VkDescriptorImageInfo* imageInfo,
		VkDescriptorType type, 
		VkShaderStageFlags stageFlags)
	{
		VkDescriptorSetLayoutBinding newBinding = {};

		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = stageFlags;
		newBinding.binding = binding;

		m_Bindings.push_back(newBinding);

		VkWriteDescriptorSet newWrite = {};

		newWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		newWrite.pNext = nullptr;
		newWrite.descriptorCount = 1;
		newWrite.descriptorType = type;
		newWrite.pImageInfo = imageInfo;
		newWrite.dstBinding = binding;

		m_Writes.push_back(newWrite);
		return *this;
	}

	bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
	{
		//build layout first
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;
		layoutInfo.pBindings = m_Bindings.data();
		layoutInfo.bindingCount = static_cast<uint32_t>(m_Bindings.size());

		layout = m_Cache->CreateDescriptorLayout(&layoutInfo);

		//allocate descriptor
		bool success = m_Alloc->Allocate(&set, layout);
		if (!success) { return false; };

		//write descriptor
		for (VkWriteDescriptorSet& w : m_Writes) 
		{
			w.dstSet = set;
		}

		vkUpdateDescriptorSets(
			m_Alloc->GetVkDevice(),
			static_cast<uint32_t>(m_Writes.size()),
			m_Writes.data(),
			0,
			nullptr);

		return true;
	}

	bool DescriptorBuilder::Build(VkDescriptorSet& set)
	{
		VkDescriptorSetLayout layout;
		return Build(set, layout);
	}
}

