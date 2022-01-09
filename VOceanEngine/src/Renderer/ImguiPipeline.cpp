#include "PreCompileHeader.h"

#include <imgui.h>
#include "ImguiPipeline.h"

#include "VulkanCore/Device.h"
#include "Renderer/Model.h"

namespace voe {

	ImguiPipeline::ImguiPipeline(Device& device, const std::string& vertFilepath, const std::string& fragFilepath, const ImguiPipelineConfigInfo& configInfo) : m_Device{ device }
	{
		CreateImguiPipeline(vertFilepath, fragFilepath, configInfo);
	}

	ImguiPipeline::~ImguiPipeline()
	{
		vkDestroyShaderModule(m_Device.GetVkDevice(), m_VertShaderModule, nullptr);
		vkDestroyShaderModule(m_Device.GetVkDevice(), m_FragShaderModule, nullptr);
		vkDestroyPipeline(m_Device.GetVkDevice(), m_ImguiPipeline, nullptr);
	}

    void ImguiPipeline::Bind(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ImguiPipeline);
    }

    void ImguiPipeline::ImguiPipelineConfig(ImguiPipelineConfigInfo& configInfo)
	{
        configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        configInfo.viewportInfo.viewportCount = 1;
        configInfo.viewportInfo.pViewports = nullptr;
        configInfo.viewportInfo.scissorCount = 1;
        configInfo.viewportInfo.pScissors = nullptr;

        configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
        configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        configInfo.rasterizationInfo.lineWidth = 1.0f;
        configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
        configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
        configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
        configInfo.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
        configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

        configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
        configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        configInfo.multisampleInfo.minSampleShading = 1.0f;           // Optional
        configInfo.multisampleInfo.pSampleMask = nullptr;             // Optional
        configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
        configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

        configInfo.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        configInfo.colorBlendAttachment.blendEnable = VK_TRUE;
        configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
        configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
        configInfo.colorBlendInfo.attachmentCount = 1;
        configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
        configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
        configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
        configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
        configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

        configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configInfo.depthStencilInfo.depthTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
        configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
        configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
        configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
        configInfo.depthStencilInfo.front = {};  // Optional
        configInfo.depthStencilInfo.back = {};   // Optional

        configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
        configInfo.dynamicStateInfo.dynamicStateCount =
            static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
        configInfo.dynamicStateInfo.flags = 0;
	}

    std::vector<char> ImguiPipeline::ReadFile(const std::string& filepath)
    {
        std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file: " + filepath);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    void ImguiPipeline::CreateImguiPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const ImguiPipelineConfigInfo& configInfo)
	{
        assert(
            configInfo.pipelineLayout != VK_NULL_HANDLE &&
            "Cannot create graphics pipeline: no pipelineLayout provided in configInfo");
        assert(
            configInfo.renderPass != VK_NULL_HANDLE &&
            "Cannot create graphics pipeline: no renderPass provided in configInfo");

        auto vertCode = ReadFile(vertFilepath);
        auto fragCode = ReadFile(fragFilepath);

        CreateShaderModule(vertCode, &m_VertShaderModule);
        CreateShaderModule(fragCode, &m_FragShaderModule);

        VkPipelineShaderStageCreateInfo shaderStages[2];
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = m_VertShaderModule;
        shaderStages[0].pName = "main";
        shaderStages[0].flags = 0;
        shaderStages[0].pNext = nullptr;
        shaderStages[0].pSpecializationInfo = nullptr;

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = m_FragShaderModule;
        shaderStages[1].pName = "main";
        shaderStages[1].flags = 0;
        shaderStages[1].pNext = nullptr;
        shaderStages[1].pSpecializationInfo = nullptr;

        // Vertex bindings an attributes based on ImGui vertex definition
        VkVertexInputBindingDescription vInputBindDescription = {};
        vInputBindDescription.binding = 0;
        vInputBindDescription.stride = sizeof(ImDrawVert);
        vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = 
        {
            CreateAttributeDescriptions(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)),	// Location 0: Position
            CreateAttributeDescriptions(0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)),	// Location 1: UV
            CreateAttributeDescriptions(0, 2, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)),	// Location 2: Color
        };

        std::vector<VkVertexInputBindingDescription> vertexInputBindings;
        vertexInputBindings.push_back(vInputBindDescription);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
        vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributes.data();
        vertexInputInfo.pVertexBindingDescriptions = vertexInputBindings.data();

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
        pipelineInfo.pViewportState = &configInfo.viewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
        pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
        pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
        pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
        pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

        pipelineInfo.layout = configInfo.pipelineLayout;
        pipelineInfo.renderPass = configInfo.renderPass;
        pipelineInfo.subpass = configInfo.subpass;

        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VOE_CHECK_RESULT(vkCreateGraphicsPipelines(
            m_Device.GetVkDevice(),
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_ImguiPipeline));
	}

	void ImguiPipeline::CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
	{
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VOE_CHECK_RESULT(vkCreateShaderModule(m_Device.GetVkDevice(), &createInfo, nullptr, shaderModule));
	}

    VkVertexInputAttributeDescription ImguiPipeline::CreateAttributeDescriptions(
        uint32_t binding,
        uint32_t location,
        VkFormat format,
        uint32_t offset)
    {
        VkVertexInputAttributeDescription vInputAttribDescription{};
        vInputAttribDescription.location = location;
        vInputAttribDescription.binding = binding;
        vInputAttribDescription.format = format;
        vInputAttribDescription.offset = offset;
        return vInputAttribDescription;
    }
}
