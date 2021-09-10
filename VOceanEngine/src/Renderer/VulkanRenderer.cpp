#include "PreCompileHeader.h"
#include "VulkanRenderer.h"

#include "VulkanCore/Device.h"
#include "Renderer/Descriptor.h"
#include "Renderer/HeightMap/HeightMap.h"

#include "Renderer/Camera.h"
#include "Renderer/GameObject.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace voe {

	struct PushConstantData 
	{
		glm::mat4 Transform{ 1.f };
		glm::mat4 NormalMatrix{ 1.f };
	};

	VulkanRenderer::VulkanRenderer(Device& device, VkRenderPass renderPass) : m_Device{ device } 
	{
		InitDescriptors();
		SetupFFTOceanComputePipeline();
		CreatePipelineLayout();
		CreatePipeline(renderPass);
	}

	VulkanRenderer::~VulkanRenderer() 
	{
		delete m_DescriptorAllocator;
		delete m_DescriptorLayoutCache;

		vkDestroySemaphore(m_Device.GetVkDevice(), m_Semaphores.Ready, nullptr);
		vkDestroySemaphore(m_Device.GetVkDevice(), m_Semaphores.Complete, nullptr);
		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_ComputePipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Device.GetVkDevice(),m_DescriptorSetLayout, nullptr);
		vkDestroyCommandPool(m_Device.GetVkDevice(), m_ComputeCommandPool, nullptr);

		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_GraphicsPipelineLayout, nullptr);
	}

	void VulkanRenderer::RenderGameObjects(
		VkCommandBuffer commandBuffer,
		std::vector<GameObject>& gameObjects,
		const Camera& camera)
	{
		m_GraphicsPipeline->Bind(commandBuffer);

		auto projectionView = camera.GetProjectionMatrix() * camera.GetViewMatrix();

		for (auto& obj : gameObjects)
		{
			PushConstantData push{};
			auto modelMatrix = obj.m_Transform.Mat4();
			push.Transform = projectionView * modelMatrix;
			push.NormalMatrix = obj.m_Transform.NormalMatrix();

			vkCmdPushConstants(
				commandBuffer,
				m_GraphicsPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstantData),
				&push);

			obj.m_Model->Bind(commandBuffer);
			obj.m_Model->Draw(commandBuffer);
		}
	}

	void VulkanRenderer::InitOceanH0Param()
	{
		m_OceanH0 = std::make_unique<HeightMap>(m_Device, m_Device.GetComputeQueue());
		m_OceanH0->CreateHeightMap(m_OceanGridSize);
	}

	void VulkanRenderer::InitDescriptors()
	{
		m_DescriptorAllocator	= new DescriptorAllocator(m_Device.GetVkDevice());
		m_DescriptorLayoutCache = new DescriptorLayoutCache(m_Device.GetVkDevice());
	}

	bool VulkanRenderer::IsComputeQueueSpecialized() const
	{
		return m_Device.GetGraphicsQueueFamily() != m_Device.GetComputeQueueFamily();
	}

	void VulkanRenderer::OnUpdate(float dt)
	{
		m_OceanH0->UpdateUniformBuffers(dt);
	}

	void VulkanRenderer::SetupFFTOceanComputePipeline()
	{
		auto device = m_Device.GetVkDevice();
		m_SpecializedComputeQueue = IsComputeQueueSpecialized();

		// create and build ocean descriptorsets
		InitOceanH0Param();
		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindBuffer(0, m_OceanH0->GetStorageBuffers().InputBufferDscInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(1, m_OceanH0->GetStorageBuffers().OutputBufferDscInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(2, m_OceanH0->GetUBOBuffers().UniformBufferDscInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.Build(m_DescriptorSets[0], m_DescriptorSetLayout);

		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindBuffer(0, m_OceanH0->GetStorageBuffers().OutputBufferDscInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(1, m_OceanH0->GetStorageBuffers().InputBufferDscInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(2, m_OceanH0->GetUBOBuffers().UniformBufferDscInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.Build(m_DescriptorSets[1]);

		// create pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
		VOE_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout));

		// create compute pipeline
		m_ComputePipeline = std::make_unique<ComputePipeline>(
			m_Device,
			"Assets/Shaders/spectrum.spv",
			m_ComputePipelineLayout);

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_Device.GetComputeQueueFamily();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VOE_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &m_ComputeCommandPool));

		// Create a command buffer for compute operations
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_ComputeCommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = m_ComputeCommandBuffers.size();
		VOE_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &m_ComputeCommandBuffers[0]));

		// Semaphores for graphics / compute synchronization
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_Semaphores.Ready));
		VOE_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_Semaphores.Complete));

		// Build a single command buffer containing the compute dispatch commands
		BuildComputeCommandBuffer();
	}

	void VulkanRenderer::SetupUniformBuffers()
	{
		m_OceanH0->CreateUniformBuffers();
	}

	void VulkanRenderer::BuildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		for (uint32_t i = 0; i < 2; i++) {

			VOE_CHECK_RESULT(vkBeginCommandBuffer(m_ComputeCommandBuffers[i], &cmdBufInfo));

			// Acquire the storage buffers from the graphics queue
			AddGraphicsToComputeBarriers(m_ComputeCommandBuffers[i]);
			m_ComputePipeline->Bind(m_ComputeCommandBuffers[i]);

			uint32_t calculateNormals = 0;
			//vkCmdPushConstants(m_ComputeCommandBuffers[i], m_ComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &calculateNormals);

			// Dispatch the compute job
			const uint32_t iterations = 64;
			for (uint32_t j = 0; j < iterations; j++) 
			{
				m_ReadSet = 1 - m_ReadSet;
				vkCmdBindDescriptorSets(
					m_ComputeCommandBuffers[i],
					VK_PIPELINE_BIND_POINT_COMPUTE,
					m_ComputePipelineLayout,
					0,
					1,
					&m_DescriptorSets[m_ReadSet],
					0,
					0);

				if (j == iterations - 1) 
				{
					calculateNormals = 1;
					// vkCmdPushConstants(compute.commandBuffers[i], compute.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &calculateNormals);
				}

				vkCmdDispatch(m_ComputeCommandBuffers[i], m_OceanGridSize / 10, m_OceanGridSize / 10, 1);

				// Don't add a barrier on the last iteration of the loop, since we'll have an explicit release to the graphics queue
				if (j != iterations - 1) 
				{
					AddComputeToComputeBarriers(m_ComputeCommandBuffers[i]);
				}
			}

			// release the storage buffers back to the graphics queue
			AddComputeToGraphicsBarriers(m_ComputeCommandBuffers[i]);
			vkEndCommandBuffer(m_ComputeCommandBuffers[i]);
		}
	}

	void VulkanRenderer::AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer)
	{
		if (m_SpecializedComputeQueue) 
		{
			VkBufferMemoryBarrier bufferBarrier = {};
			bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferBarrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			bufferBarrier.srcQueueFamilyIndex = m_Device.GetGraphicsQueueFamily();
			bufferBarrier.dstQueueFamilyIndex = m_Device.GetComputeQueueFamily();
			bufferBarrier.size = VK_WHOLE_SIZE;

			std::vector<VkBufferMemoryBarrier> bufferBarriers;
			bufferBarrier.buffer = m_OceanH0->GetStorageBuffers().InputBuffer;
			bufferBarriers.push_back(bufferBarrier);
			bufferBarrier.buffer = m_OceanH0->GetStorageBuffers().OutputBuffer;
			bufferBarriers.push_back(bufferBarrier);

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_FLAGS_NONE,
				0, 
				nullptr,
				static_cast<uint32_t>(bufferBarriers.size()),
				bufferBarriers.data(),
				0,
				nullptr);
		}
	}

	void VulkanRenderer::AddComputeToComputeBarriers(VkCommandBuffer commandBuffer)
	{
		VkBufferMemoryBarrier bufferBarrier = {};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		bufferBarrier.srcQueueFamilyIndex = m_Device.GetComputeQueueFamily();
		bufferBarrier.dstQueueFamilyIndex = m_Device.GetComputeQueueFamily();
		bufferBarrier.size = VK_WHOLE_SIZE;

		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		bufferBarrier.buffer = m_OceanH0->GetStorageBuffers().InputBuffer;
		bufferBarriers.push_back(bufferBarrier);
		bufferBarrier.buffer = m_OceanH0->GetStorageBuffers().OutputBuffer;
		bufferBarriers.push_back(bufferBarrier);

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_FLAGS_NONE,
			0,
			nullptr,
			static_cast<uint32_t>(bufferBarriers.size()),
			bufferBarriers.data(),
			0,
			nullptr);
	}

	void VulkanRenderer::AddComputeToGraphicsBarriers(VkCommandBuffer commandBuffer)
	{
		if (m_SpecializedComputeQueue) 
		{
			VkBufferMemoryBarrier bufferBarrier = {};
			bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.srcQueueFamilyIndex = m_Device.GetComputeQueueFamily();
			bufferBarrier.dstQueueFamilyIndex = m_Device.GetGraphicsQueueFamily();
			bufferBarrier.size = VK_WHOLE_SIZE;

			std::vector<VkBufferMemoryBarrier> bufferBarriers;
			bufferBarrier.buffer = m_OceanH0->GetStorageBuffers().InputBuffer;
			bufferBarriers.push_back(bufferBarrier);
			bufferBarrier.buffer = m_OceanH0->GetStorageBuffers().OutputBuffer;
			bufferBarriers.push_back(bufferBarrier);

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_FLAGS_NONE,
				0,
				nullptr,
				static_cast<uint32_t>(bufferBarriers.size()),
				bufferBarriers.data(),
				0,
				nullptr);
		}
	}

	void VulkanRenderer::CreatePipelineLayout()
	{
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VOE_CHECK_RESULT(vkCreatePipelineLayout(m_Device.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_GraphicsPipelineLayout));
	}

	void VulkanRenderer::CreatePipeline(VkRenderPass renderPass)
	{
		VOE_CORE_ASSERT(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig = {};
		GraphicsPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_GraphicsPipelineLayout;

		// auto device = const_cast<Device&>(m_Device);
		m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(
			m_Device,
			"Assets/Shaders/testVert.spv",
			"Assets/Shaders/testFrag.spv",
			pipelineConfig);
	}
}

