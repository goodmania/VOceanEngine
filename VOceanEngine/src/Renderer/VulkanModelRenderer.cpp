#include "PreCompileHeader.h"
#include "VulkanModelRenderer.h"

#include "VulkanCore/Device.h"
#include "Renderer/Descriptor.h"

#include "Renderer/Buffer.h"
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
		glm::vec3 lightDirection = glm::vec3(-1.0f, -1.0f, 1.0f);
		glm::vec3 CameraPos{ 0.0f, 0.0f, 0.0f };
	};

	VulkanModelRenderer::VulkanModelRenderer(Device& device, VkRenderPass renderPass) : m_Device{ device } 
	{
		InitDescriptors();
		CreateGraphicsUbo();
		CreatePipelineCache();
		CreatePipelineLayout();
		CreateDescriptorSets();
		CreatePipeline(renderPass);
	}

	VulkanModelRenderer::~VulkanModelRenderer() 
	{
		delete m_GlobalUboDscInfo;
		delete m_DescriptorAllocator;
		delete m_DescriptorLayoutCache;

		vkDestroyPipelineCache(m_Device.GetVkDevice(), m_PipelineCache, nullptr);
		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_GraphicsPipelineLayout, nullptr);
	}

	void VulkanModelRenderer::RenderGameObjects(FrameInfo frameInfo, std::vector<GameObject>& gameObjects)
	{
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
	}

	void VulkanModelRenderer::InitDescriptors()
	{
		m_DescriptorAllocator	= new DescriptorAllocator(m_Device.GetVkDevice());
		m_DescriptorLayoutCache = new DescriptorLayoutCache(m_Device.GetVkDevice());

		m_GlobalUboDscInfo = new VkDescriptorBufferInfo();
	}

	void VulkanModelRenderer::CreateGraphicsUbo()
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

	void VulkanModelRenderer::UpdateGlobalUboBuffers(FrameInfo& frameInfo)
	{
		// update
		GlobalUbo ubo{};
		ubo.ProjectionView = frameInfo.CameraObj.GetProjection() * frameInfo.CameraObj.GetView();
		ubo.CameraPos = frameInfo.CameraObj.GetCameraPos();
		m_GlobalUboBuffers[frameInfo.FrameIndex]->WriteToBuffer(&ubo);
		m_GlobalUboBuffers[frameInfo.FrameIndex]->Flush();
	}

	void VulkanModelRenderer::OnUpdate(float dt, FrameInfo& frameInfo)
	{
		UpdateGlobalUboBuffers(frameInfo);
	}

	void VulkanModelRenderer::CreateDescriptorSets()
	{
		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindBuffer(0, m_GlobalUboDscInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(m_GraphicsDescriptorSet, m_GraphicsDescriptorSetLayout);
	}

	void VulkanModelRenderer::CreatePipelineLayout()
	{

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

	void VulkanModelRenderer::CreatePipeline(VkRenderPass renderPass)
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

	void VulkanModelRenderer::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreatePipelineCache(m_Device.GetVkDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}
}
