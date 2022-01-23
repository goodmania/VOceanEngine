#include "PreCompileHeader.h"
#include "VulkanImguiRenderer.h"

#include "VulkanCore/Device.h"
#include "Renderer/Descriptor.h"
#include "Platform/Vulkan/VoeImgui.h"

#include "Renderer/Camera.h"
#include "Renderer/GameObject.h"
#include "Renderer/FrameInfo.h"
#include "Renderer/Swapchain.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>

namespace voe {

	// UI params are set via push constants
	/*struct PushConstantData {
		glm::mat2 Transform{ 1.f };
		glm::vec2 Offset;
		alignas(16) glm::vec3 Color;
	} PushConstBlock;*/

	struct PushConstBlock {
		glm::vec2 Scale;
		glm::vec2 Translate;
	} PushConstBlock;

	struct ImguiUBO {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 lightPos;
	};

	VulkanImguiRenderer::VulkanImguiRenderer(Device& device, PhDevice& phDevice, VkRenderPass renderPass, VkExtent2D windowSize) : m_Device{ device }, m_PhDevice{ phDevice }, m_WindowSize{windowSize}
	{
		InitImgui();
		InitDescriptors();
		CreateImguiUBO();
		CreatePipelineCache();
		CreatePipelineLayout();
		CreatePipeline(renderPass);
	}

	VulkanImguiRenderer::~VulkanImguiRenderer() 
	{
		delete m_ImguiUBODscInfo;
		delete m_DescriptorAllocator;
		delete m_DescriptorLayoutCache;

		// vkDestroyDescriptorSetLayout()は必要なし
		vkDestroyPipelineLayout(m_Device.GetVkDevice(), m_PipelineLayout, nullptr);
		vkDestroyPipelineCache(m_Device.GetVkDevice(), m_PipelineCache, nullptr);
	}

	// todo コメントアウトを消す
	void VulkanImguiRenderer::RenderImgui(FrameInfo frameInfo)
	{
		ImGuiIO& io = ImGui::GetIO();

		m_ImguiPipeline->Bind(frameInfo.CommandBuffer);
		vkCmdBindDescriptorSets(frameInfo.CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

		VkViewport viewport = {};
		viewport.width = io.DisplaySize.x;
		viewport.height = io.DisplaySize.y;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(frameInfo.CommandBuffer, 0, 1, &viewport);

		// UI scale and translate via push constants
		PushConstBlock.Scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		PushConstBlock.Translate = glm::vec2(-1.0f);
		vkCmdPushConstants(frameInfo.CommandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstBlock), &PushConstBlock);

		m_Imgui->DrawFrame(frameInfo.CommandBuffer, frameInfo);

	}

	void VulkanImguiRenderer::InitImgui()
	{
		m_Imgui = std::make_unique<ImGUI>(m_Device, m_PhDevice, static_cast<float>(m_WindowSize.width), static_cast<float>(m_WindowSize.height));
		m_Imgui->InitResources(m_Device.GetGraphicsQueue());
	}

	void VulkanImguiRenderer::InitDescriptors()
	{
		m_DescriptorAllocator	= new DescriptorAllocator(m_Device.GetVkDevice());
		m_DescriptorLayoutCache = new DescriptorLayoutCache(m_Device.GetVkDevice());
		m_ImguiUBODscInfo = new VkDescriptorBufferInfo();
	}

	void VulkanImguiRenderer::CreateImguiUBO()
	{
		m_ImguiUBO.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < m_ImguiUBO.size(); i++)
		{
			m_ImguiUBO[i] = std::make_unique<Buffer>(
				m_Device,
				sizeof(ImguiUBO),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

			m_ImguiUBO[i]->Map();

			m_ImguiUBODscInfo->buffer = m_ImguiUBO[i]->GetBuffer();
			m_ImguiUBODscInfo->range = VK_WHOLE_SIZE;
			m_ImguiUBODscInfo->offset = 0;
		}
	}

	//void VulkanImguiRenderer::UpdateGlobalUboBuffers(FrameInfo& frameInfo)
	//{
	//	// update
	//	ImguiUBO ubo{};
	//	ubo.ProjectionView = frameInfo.CameraObj.GetProjection() * frameInfo.CameraObj.GetView();
	//	ubo.CameraPos = frameInfo.CameraObj.GetCameraPos();
	//	m_GlobalUboBuffers[frameInfo.FrameIndex]->WriteToBuffer(&ubo);
	//	m_GlobalUboBuffers[frameInfo.FrameIndex]->Flush();
	//}


	void VulkanImguiRenderer::OnUpdate(float dt, FrameInfo& frameInfo, VkExtent2D windowSize)
	{
		m_Imgui->UpdateImgui(windowSize);
		// todo modify boolean
		m_Imgui->NewImGuiFrame(frameInfo, false);

		m_Imgui->UpdateBuffers(frameInfo);
	}

	void VulkanImguiRenderer::CreateDescriptorSets()
	{
		DescriptorBuilder::Begin(m_DescriptorLayoutCache, m_DescriptorAllocator)
			.BindImage(0, m_Imgui->GetImguiDscInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build(m_DescriptorSet, m_DescriptorSetLayout);
	}

	void VulkanImguiRenderer::CreatePipelineLayout()
	{
		CreateDescriptorSets();

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstBlock);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		VOE_CHECK_RESULT(vkCreatePipelineLayout(m_Device.GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
	}

	void VulkanImguiRenderer::CreatePipeline(VkRenderPass renderPass)
	{
		assert(m_PipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		ImguiPipelineConfigInfo pipelineConfig = {};
		ImguiPipeline::ImguiPipelineConfig(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_PipelineLayout;

		m_ImguiPipeline = std::make_unique<ImguiPipeline>(
			m_Device,
			"Assets/Shaders/ui.vert.spv",
			"Assets/Shaders/ui.frag.spv",
			pipelineConfig);
	}

	void VulkanImguiRenderer::CreatePipelineCache()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VOE_CHECK_RESULT(vkCreatePipelineCache(m_Device.GetVkDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
	}
}
