#pragma once

namespace voe {

	class Device;

	struct PipelineConfigInfo
	{
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class VOE_API ImguiPipeline
	{
	public:
		ImguiPipeline(
			Device& device,
			const std::string& vertFilepath,
			const std::string& fragFilepath,
			const PipelineConfigInfo& configInfo);
		~ImguiPipeline();

		ImguiPipeline(const ImguiPipeline&) = delete;
		ImguiPipeline& operator=(const ImguiPipeline&) = delete;

		void Bind(VkCommandBuffer commandBuffer);
		static void ImguiPipelineConfigInfo(PipelineConfigInfo& configInfo);

	private:

		static std::vector<char> ReadFile(const std::string& filepath);

		void CreateImguiPipeline(
			const std::string& vertFilepath,
			const std::string& fragFilepath,
			const PipelineConfigInfo& configInfo);

		void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		VkVertexInputAttributeDescription CreateAttributeDescriptions(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset);

		Device& m_Device;
		VkPipeline m_ImguiPipeline;
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
	};
}
