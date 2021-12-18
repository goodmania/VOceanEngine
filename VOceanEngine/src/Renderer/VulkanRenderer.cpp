#include "PreCompileHeader.h"
#include "VulkanRenderer.h"

#include "VulkanCore/Device.h"
#include "Renderer/Descriptor.h"
#include "Renderer/HeightMap/HeightMap.h"

#include "Renderer/Camera.h"
#include "Renderer/GameObject.h"
#include "Renderer/FrameInfo.h"
#include "Renderer/Swapchain.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace voe {

	struct PushConstantData 
	{
		glm::mat4 ModelMatrix{ 1.f };
		glm::mat4 NormalMatrix{ 1.f };
	};

	struct GlobalUbo               
	{
		glm::mat4 ProjectionView{ 1.f };
		glm::vec3 lightDirection = glm::vec3(-1.0f, -1.0f, -1.0f);
		//alignas(16) glm::vec3 SeaBaseColor = glm::vec3(0.0f, 0.1f, 0.6f);
		alignas(16) glm::vec3 SeaBaseColor = glm::vec3(0.01f, 0.13f, 0.15f);
		float BaseColorStrength{ 1.5f };
		glm::vec3 SeaShallowColor = glm::vec3(75.f / 256.f, 89.f / 256.f, 35.f / 256.f);
		//glm::vec3 SeaShallowColor = glm::vec3(0.1f, 0.3f, 0.3f);
		float ColorHightOffset{ 0.05f };
		glm::vec3 CameraPos{ 0.0f, 0.0f, 0.0f };
	};

	VulkanRenderer::VulkanRenderer(Device& device, VkRenderPass renderPass) : m_Device{ device } 
	{
		InitDescriptors();
		CreateGraphicsUbo();
		CreatePipelineCache();
		SetupFFTOceanComputePipelines();
		//SetupImageTransitionCommand();
		CreatePipelineLayout();
		CreatePipeline(renderPass);
	}

	VulkanRenderer::~VulkanRenderer() 
	{
		delete m_GlobalUboDscInfo;
		delete m_DescriptorAllocator;
		delete m_DescriptorLayoutCache;

		vkDestroySemaphore(m_Device.GetVkDevice(), m_ComputeSemaphores.Ready, nullptr);
		vkDestroySemaphore(m_Device.GetVkDevice(), m_ComputeSemaphores.Complete, nullptr);

		vkDestroySemaphore(m_Device.GetVkDevice(), m_ImageTransitionSemaphores.Ready, nullptr);
		vkDestroySemaphore(m_Device.GetVkDevice(), m_ImageTransitionSemaphores.Complete, nullptr);

		for (uint32_t i = 0; i < m_DescriptorSetLayouts.size(); i++)
		{
			vkDestroyDescriptorSetLayout(m_Device.GetVkDevice(), m_DescriptorSetLayouts[i], nullptr);
		}
		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_ComputePipelineLayout, nullptr);
		vkDestroyCommandPool(m_Device.GetVkDevice(), m_ComputeCommandPool, nullptr);
		vkDestroyCommandPool(m_Device.GetVkDevice(), m_ImageTransitionCommandPool, nullptr);

		vkDestroyPipelineCache(m_Device.GetVkDevice(), m_PipelineCache, nullptr);
		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_GraphicsPipelineLayout, nullptr);
	}

	void VulkanRenderer::RenderGameObjects(FrameInfo frameInfo, std::vector<GameObject>& gameObjects)
	{
		// Acquire barrier
		AddComputeToGraphicsBarriers(frameInfo.CommandBuffer, frameInfo.FrameIndex);

		m_GraphicsPipeline->Bind(frameInfo.CommandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_GraphicsPipelineLayout,
			0,
			1,
			&m_GraphicsDescriptorSet,
			0,
			0);

		for (auto& obj : gameObjects)
		{
			PushConstantData push = {};
			push.ModelMatrix = obj.m_Transform.Mat4();
			push.NormalMatrix = obj.m_Transform.NormalMatrix();

			vkCmdPushConstants(
				frameInfo.CommandBuffer,
				m_GraphicsPipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(PushConstantData),
				&push);

			obj.m_Model->Bind(frameInfo.CommandBuffer);
			obj.m_Model->Draw(frameInfo.CommandBuffer);
		}

		// Acquire barrier
		AddGraphicsToComputeBarriers(frameInfo.CommandBuffer, frameInfo.FrameIndex);
	}

	void VulkanRenderer::InitOceanHeightMap()
	{
		m_OceanHeightMap = std::make_unique<HeightMap>(m_Device, m_Device.GetComputeQueue());
		m_OceanHeightMap->CreateHeightMap(m_GroupSize);
	}

	void VulkanRenderer::InitDescriptors()
	{
		m_DescriptorAllocator	= new DescriptorAllocator(m_Device.GetVkDevice());
		m_DescriptorLayoutCache = new DescriptorLayoutCache(m_Device.GetVkDevice());

		m_GlobalUboDscInfo = new VkDescriptorBufferInfo();
	}

	void VulkanRenderer::CreateGraphicsUbo()
	{
		m_GlobalUboBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < m_GlobalUboBuffers.size(); i++)
		{
			m_GlobalUboBuffers[i] = std::make_unique<Buffer>(
				m_Device,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			m_GlobalUboBuffers[i]->Map();

			m_GlobalUboDscInfo->buffer = m_GlobalUboBuffers[i]->GetBuffer();
			m_GlobalUboDscInfo->range = VK_WHOLE_SIZE;
			m_GlobalUboDscInfo->offset = 0;
		}
	}

	void VulkanRenderer::UpdateGlobalUboBuffers(FrameInfo& frameInfo)
	{
		// update
		GlobalUbo ubo{};
		ubo.ProjectionView = frameInfo.CameraObj.GetProjection() * frameInfo.CameraObj.GetView();
		ubo.CameraPos = frameInfo.CameraObj.GetCameraPos();
		m_GlobalUboBuffers[frameInfo.FrameIndex]->WriteToBuffer(&ubo);
		m_GlobalUboBuffers[frameInfo.FrameIndex]->Flush();
	}

	bool VulkanRenderer::IsComputeQueueSpecialized() const
	{
		return m_Device.GetGraphicsQueueFamily() != m_Device.GetComputeQueueFamily();
	}

	void VulkanRenderer::OnUpdate(float dt, FrameInfo& frameInfo)
	{
		m_OceanHeightMap->UpdateComputeUniformBuffers(dt, frameInfo.FrameIndex);
		UpdateGlobalUboBuffers(frameInfo);
	}

	void VulkanRenderer::CreateDescriptorSets()
	{
		m_DescriptorSets.resize(11);
		m_DescriptorSetLayouts.resize(11);
		uint32_t descriptorIndex = 0;

		// m_DescriptorSets[0]
		// for spectrum.comp (Calculate Phillips spectrum, partial derivative, and displacement in x,y direction)
		// and oceanNormal.comp
		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindBuffer(0, m_OceanHeightMap->GetH0BufferDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(1, m_OceanHeightMap->GetHtBufferDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(2, m_OceanHeightMap->GetHt_dmyBufferDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindBuffer(3, m_OceanHeightMap->GetUniformBufferDscInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindImage(4, m_OceanHeightMap->GetOceanNormalTextureDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.BindImage(5, m_OceanHeightMap->GetOceanBubbleTextureDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
			.Build(m_DescriptorSets[descriptorIndex], m_DescriptorSetLayouts[descriptorIndex]);

		++descriptorIndex;

		// for FFT calculations. Specify individual elements of the Ocean structure.
		for (uint32_t index = 0; index < HeightMap::m_OceanElementCount; index++)
		{
			// m_DescriptorSets - H_y [1], H_dx [3], H_dz [5], dx [7], dy [9]
			assert(descriptorIndex < m_DescriptorSets.size());
			DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
				.BindBuffer(0, m_OceanHeightMap->GetHtBufferDscInfos(index), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.BindBuffer(1, m_OceanHeightMap->GetHt_dmyBufferDscInfos(index), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.BindBuffer(2, m_OceanHeightMap->GetUniformBufferDscInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.Build(m_DescriptorSets[descriptorIndex], m_DescriptorSetLayouts[descriptorIndex]);

			++descriptorIndex;

			// m_DescriptorSets - Hdmy_y [2], H_dmydx [4], H_dmydz [6], dmydx [8], dmydy [10]
			assert(descriptorIndex < m_DescriptorSets.size());
			DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
				.BindBuffer(0, m_OceanHeightMap->GetHt_dmyBufferDscInfos(index), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.BindBuffer(1, m_OceanHeightMap->GetHtBufferDscInfos(index), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.BindBuffer(2, m_OceanHeightMap->GetUniformBufferDscInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
				.Build(m_DescriptorSets[descriptorIndex], m_DescriptorSetLayouts[descriptorIndex]);

			++descriptorIndex;		
		}
	}

	void VulkanRenderer::SetupFFTOceanComputePipelines()
	{
		auto device = m_Device.GetVkDevice();
		m_SpecializedComputeQueue = IsComputeQueueSpecialized();

		InitOceanHeightMap();

		// create and build ocean descriptorsets
		CreateDescriptorSets();
		
		// create pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutCreateInfo.pSetLayouts = m_DescriptorSetLayouts.data();
		VOE_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_ComputePipelineLayout));
		
		// create compute pipeline
		m_ComputePipeline = std::make_unique<ComputePipeline>(
			m_Device,
			"Assets/Shaders/spectrum.spv",
			m_ComputePipelineLayout,
			m_PipelineCache);

		m_FFTComputePipeline = std::make_unique<ComputePipeline>(
			m_Device,
			"Assets/Shaders/FFT.spv",
			m_ComputePipelineLayout,
			m_PipelineCache);

		m_ComputeNormalPipeline = std::make_unique<ComputePipeline>(
			m_Device,
			"Assets/Shaders/oceanNormal.spv",
			m_ComputePipelineLayout,
			m_PipelineCache);

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
		VOE_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ComputeSemaphores.Ready));
		VOE_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ComputeSemaphores.Complete));

		// Build a single command buffer containing the compute dispatch commands
		BuildComputeCommandBuffer();
	}

	void VulkanRenderer::SetupImageTransitionCommand()
	{
		auto device = m_Device.GetVkDevice();

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_Device.GetComputeQueueFamily();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VOE_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &m_ImageTransitionCommandPool));

		// Create a command buffer for compute operations
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_ImageTransitionCommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = m_ImageTransitionCommandBuffers.size();
		VOE_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &m_ImageTransitionCommandBuffers[0]));

		// Semaphores for graphics / compute synchronization
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ImageTransitionSemaphores.Ready));
		VOE_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_ImageTransitionSemaphores.Complete));

		BuildImageTransitionCommand();
	}

	void VulkanRenderer::BuildImageTransitionCommand()
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		m_OceanHeightMap->GetOceanBubbleTexture(0).UpdateDescriptorImageLayout(VK_IMAGE_LAYOUT_GENERAL);
		m_OceanHeightMap->GetOceanBubbleTexture(1).UpdateDescriptorImageLayout(VK_IMAGE_LAYOUT_GENERAL);

		for (uint32_t index = 0; index < 2; index++)
		{
			VOE_CHECK_RESULT(vkBeginCommandBuffer(m_ImageTransitionCommandBuffers[index], &cmdBufInfo));

			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.srcAccessMask = 0;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = m_OceanHeightMap->GetOceanBubbleImage(index);
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			vkCmdPipelineBarrier(
				m_ImageTransitionCommandBuffers[index],
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			VOE_CHECK_RESULT(vkEndCommandBuffer(m_ImageTransitionCommandBuffers[index]));
		}
	}

	void VulkanRenderer::BuildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		for (uint32_t index = 0; index < 2; index++)
		{
			VOE_CHECK_RESULT(vkBeginCommandBuffer(m_ComputeCommandBuffers[index], &cmdBufInfo));

			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = m_Device.GetComputeQueueFamily();
			imageMemoryBarrier.dstQueueFamilyIndex = m_Device.GetComputeQueueFamily();
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = m_OceanHeightMap->GetOceanBubbleImage(index);
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			VkImageMemoryBarrier imageNormalMemoryBarrier{};
			imageNormalMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageNormalMemoryBarrier.srcQueueFamilyIndex = m_Device.GetComputeQueueFamily();
			imageNormalMemoryBarrier.dstQueueFamilyIndex = m_Device.GetComputeQueueFamily();
			imageNormalMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageNormalMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageNormalMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageNormalMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageNormalMemoryBarrier.image = m_OceanHeightMap->GetOceanNormalImage(index);
			imageNormalMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			AddGraphicsToComputeBarriers(m_ComputeCommandBuffers[index], index);

			// 1: Calculate philips spectrum
			m_ComputePipeline->Bind(m_ComputeCommandBuffers[index]);
			vkCmdBindDescriptorSets(m_ComputeCommandBuffers[index], VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &m_DescriptorSets[0], 0, 0);
			vkCmdDispatch(m_ComputeCommandBuffers[index], 1, m_GroupSize, 1);
			AddComputeToComputeBarriers(m_ComputeCommandBuffers[index], m_OceanHeightMap->GetH0Buffer(index), m_OceanHeightMap->GetHtBuffer(index));

			m_FFTComputePipeline->Bind(m_ComputeCommandBuffers[index]);
			
			// Calculate wave heights(H_y), displacements(dx, dz), and partial derivatives(H_x,H_z) with FFT
			for (uint32_t descriptorIndex = 1; descriptorIndex < m_DescriptorSets.size();)
			{
				// 2-1: Calculate FFT in horizontal direction
				vkCmdBindDescriptorSets(m_ComputeCommandBuffers[index], VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 1, 1, &m_DescriptorSets[descriptorIndex], 0, 0);
				vkCmdDispatch(m_ComputeCommandBuffers[index], m_GroupSize, 1, 1);
				++descriptorIndex;

				AddComputeToComputeBarriers(m_ComputeCommandBuffers[index], m_OceanHeightMap->GetHtBuffer(index), m_OceanHeightMap->GetHt_dmyBuffer(index));

				// 2-2: Calculate FFT in vertical direction
				vkCmdBindDescriptorSets(m_ComputeCommandBuffers[index], VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 1, 1, &m_DescriptorSets[descriptorIndex], 0, 0);
				vkCmdDispatch(m_ComputeCommandBuffers[index], m_GroupSize, 1, 1);
				++descriptorIndex;

				AddComputeToComputeBarriers(m_ComputeCommandBuffers[index], m_OceanHeightMap->GetHtBuffer(index), m_OceanHeightMap->GetHt_dmyBuffer(index));
			}
		
			vkCmdPipelineBarrier(
				m_ComputeCommandBuffers[index],
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			vkCmdPipelineBarrier(
				m_ComputeCommandBuffers[index],
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageNormalMemoryBarrier);

			// 3: Calculate NormalMap
			m_ComputeNormalPipeline->Bind(m_ComputeCommandBuffers[index]);
			vkCmdBindDescriptorSets(m_ComputeCommandBuffers[index], VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipelineLayout, 0, 1, &m_DescriptorSets[0], 0, 0);
			vkCmdDispatch(m_ComputeCommandBuffers[index], 1, m_GroupSize, 1);

			AddComputeToGraphicsBarriers(m_ComputeCommandBuffers[index], index);

			VOE_CHECK_RESULT(vkEndCommandBuffer(m_ComputeCommandBuffers[index]));
		}
	}

	void VulkanRenderer::AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer, uint32_t index)
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
			bufferBarrier.buffer = m_OceanHeightMap->GetHtBuffer(index);
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

	void VulkanRenderer::AddComputeToComputeBarriers(VkCommandBuffer commandBuffer, VkBuffer InputBuffer, VkBuffer OutputBuffer)
	{
		VkBufferMemoryBarrier bufferBarrier = {};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		bufferBarrier.srcQueueFamilyIndex = m_Device.GetComputeQueueFamily();
		bufferBarrier.dstQueueFamilyIndex = m_Device.GetComputeQueueFamily();
		bufferBarrier.size = VK_WHOLE_SIZE;

		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		bufferBarrier.buffer = InputBuffer;
		bufferBarriers.push_back(bufferBarrier);
		bufferBarrier.buffer = OutputBuffer;
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

	void VulkanRenderer::AddComputeToGraphicsBarriers(VkCommandBuffer commandBuffer, uint32_t index)
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
			bufferBarrier.buffer = m_OceanHeightMap->GetHtBuffer(index);
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
		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindBuffer(0, m_OceanHeightMap->GetHtBufferDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.BindBuffer(1, m_OceanHeightMap->GetUniformBufferDscInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.BindBuffer(2, m_GlobalUboDscInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.BindImage(3, m_OceanHeightMap->GetOceanNormalTextureDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.BindImage(4, m_OceanHeightMap->GetOceanBubbleTextureDscInfo(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(m_GraphicsDescriptorSet, m_GraphicsDescriptorSetLayout);

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_GraphicsDescriptorSetLayout;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VOE_CHECK_RESULT(vkCreatePipelineLayout(m_Device.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_GraphicsPipelineLayout));
	}

	void VulkanRenderer::CreatePipeline(VkRenderPass renderPass)
	{
		assert(m_GraphicsPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig = {};
		GraphicsPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_GraphicsPipelineLayout;

		m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(
			m_Device,
			"Assets/Shaders/testVert.spv",
			"Assets/Shaders/testFrag.spv",
			pipelineConfig);
	}

	void VulkanRenderer::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreatePipelineCache(m_Device.GetVkDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}
}
