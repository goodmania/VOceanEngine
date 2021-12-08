#pragma once

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#define VOE_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << voe::tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

#define VOE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

	const std::string getAssetPath();

	namespace voe
	{
		namespace tools
		{
			/** @brief Returns an error code as a string */
			std::string errorString(VkResult errorCode);

			// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
			void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

			// Uses a fixed sub resource layout with first mip level and layer
			void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageAspectFlags aspectMask,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

			/** @brief Checks if a file exists */
			bool fileExists(const std::string& filename);

			// Display error message and exit on fatal error
			void exitFatal(const std::string& message, int32_t exitCode);
			void exitFatal(const std::string& message, VkResult resultCode);

			// Load a SPIR-V shader (binary)
			VkShaderModule loadShader(const char* fileName, VkDevice device);
		}
	}


