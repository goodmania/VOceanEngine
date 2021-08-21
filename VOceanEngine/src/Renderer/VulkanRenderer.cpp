#include "PreCompileHeader.h"
#include "VulkanRenderer.h"

#include "VulkanCore/Device.h"
#include "Renderer/GraphicsPipeline.h"
#include "Renderer/Camera.h"

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
		CreatePipelineLayout();
		CreatePipeline(renderPass);
	}

	VulkanRenderer::~VulkanRenderer() 
	{
		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_PipelineLayout, nullptr);
	}

	void VulkanRenderer::RenderGameObjects(
		VkCommandBuffer commandBuffer,
		std::vector<GameObject>& gameObjects,
		const Camera& camera)
	{

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

		VOE_CHECK_RESULT(vkCreatePipelineLayout(m_Device.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
	}

	void VulkanRenderer::CreatePipeline(VkRenderPass renderPass)
	{
		VOE_CORE_ASSERT(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig = {};
		GraphicsPipeline::DefaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_PipelineLayout;

		m_GraphicsPipeline = std::make_unique<GraphicsPipeline>(
			m_Device.GetVkDevice(),
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig);
	}
}

