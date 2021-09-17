#include "PreCompileHeader.h"
#include "ComputePipeline.h"

#include "VulkanCore/Device.h"

namespace voe {

	ComputePipeline::ComputePipeline(
		Device& device,
		const std::string& compFilepath,
        VkPipelineLayout layout,
        VkPipelineCache cache) : m_Device{ device }
	{
		CreateComputePipeline(compFilepath, layout, cache);
	}

	ComputePipeline::~ComputePipeline()
	{
		vkDestroyShaderModule(m_Device.GetVkDevice(), m_CompShaderModule, nullptr);
		vkDestroyPipeline(m_Device.GetVkDevice(), m_ComputePipeline, nullptr);
	}

    std::vector<char> ComputePipeline::ReadFile(const std::string& filepath)
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

    void ComputePipeline::CreateComputePipeline(const std::string& compFilepath, VkPipelineLayout layout, VkPipelineCache cache)
    {
        auto compCode = ReadFile(compFilepath);

        CreateShaderModule(compCode, &m_CompShaderModule);

        VkPipelineShaderStageCreateInfo shaderStage = {};
        shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStage.module = m_CompShaderModule;
        shaderStage.pName = "main";
        shaderStage.flags = 0;
        shaderStage.pNext = nullptr;
        shaderStage.pSpecializationInfo = nullptr;

        VkComputePipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stage = shaderStage;
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        VOE_CHECK_RESULT(vkCreateComputePipelines(
            m_Device.GetVkDevice(),
            cache,
            1,
            &pipelineCreateInfo,
            nullptr,
            &m_ComputePipeline));
    }

    void ComputePipeline::CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VOE_CHECK_RESULT(vkCreateShaderModule(m_Device.GetVkDevice(), &createInfo, nullptr, shaderModule));
    }

    void ComputePipeline::Bind(VkCommandBuffer commandBuffer)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
    }
}