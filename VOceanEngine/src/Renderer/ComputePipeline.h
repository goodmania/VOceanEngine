#pragma once

namespace voe {

	class Device;

	class VOE_API ComputePipeline
	{
	public:
		ComputePipeline(
			Device& device,
			const std::string& compFilepath,
			VkPipelineLayout layout = nullptr,
			VkPipelineCache cache = nullptr);

		~ComputePipeline();

		ComputePipeline(const ComputePipeline&) = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;

		void Bind(VkCommandBuffer commandBuffer);
		static std::vector<char> ReadFile(const std::string& filepath);

	private:
		void CreateComputePipeline(const std::string& compFilepath, VkPipelineLayout layout, VkPipelineCache cache);
		void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		Device& m_Device;
		VkPipeline m_ComputePipeline;
		VkShaderModule m_CompShaderModule;
	};
}
