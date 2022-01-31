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

	struct ModelUbo
	{
		glm::mat4 ProjectionView{ 1.f };
		glm::vec4 AmbientLightColor{ 1.f, 1.f, 1.f, .02f };  // w is intensity
		glm::vec3 LightPosition{ -1.f };
		alignas(16) glm::vec4 LightColor{ 1.f };  // w is light intensity
	};

	VulkanModelRenderer::VulkanModelRenderer(Device& device, VkRenderPass renderPass) : m_Device{ device } 
	{
		InitDescriptors();
		CreateModelUbo();
		CreatePipelineCache();
		CreateDescriptorSets();
		CreatePipelineLayout();
		CreatePipeline(renderPass);
	}

	VulkanModelRenderer::~VulkanModelRenderer() 
	{
		delete m_ModelUboDscInfo;
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

		m_ModelUboDscInfo = new VkDescriptorBufferInfo();
	}

	void VulkanModelRenderer::CreateModelUbo()
	{
		m_ModelUboBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < m_ModelUboBuffers.size(); i++)
		{
			m_ModelUboBuffers[i] = std::make_unique<Buffer>(
				m_Device,
				sizeof(ModelUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			m_ModelUboBuffers[i]->Map();

			m_ModelUboDscInfo->buffer = m_ModelUboBuffers[i]->GetBuffer();
			m_ModelUboDscInfo->range = VK_WHOLE_SIZE;
			m_ModelUboDscInfo->offset = 0;
		}
	}

	void VulkanModelRenderer::UpdateModelUboBuffers(FrameInfo& frameInfo)
	{
		// update
		ModelUbo ubo{};
		ubo.ProjectionView = frameInfo.CameraObj.GetProjection() * frameInfo.CameraObj.GetView();
		m_ModelUboBuffers[frameInfo.FrameIndex]->WriteToBuffer(&ubo);
		m_ModelUboBuffers[frameInfo.FrameIndex]->Flush();
	}

	void VulkanModelRenderer::OnUpdate(float dt, FrameInfo& frameInfo)
	{
		UpdateModelUboBuffers(frameInfo);
	}

	void VulkanModelRenderer::CreateDescriptorSets()
	{
		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindBuffer(0, m_ModelUboDscInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
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
			"Assets/Shaders/modelVert.spv",
			"Assets/Shaders/modelFrag.spv",
			pipelineConfig);
	}

	void VulkanModelRenderer::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreatePipelineCache(m_Device.GetVkDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}
}
