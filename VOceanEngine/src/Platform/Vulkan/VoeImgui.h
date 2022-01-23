/*
* Vulkan Example - imGui (https://github.com/ocornut/imgui)
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "Renderer/Buffer.h"

// #define ENABLE_VALIDATION false

// ----------------------------------------------------------------------------
// ImGUI class
// ----------------------------------------------------------------------------

namespace voe {
	class FrameInfo;
	class Device;

	class VOE_API ImGUI
	{
	public:
		ImGUI(Device& device, PhDevice& phDevice, float width, float height);
		~ImGUI();

		// Initialize styles, keys, etc.
		void Init(float width, float height);
		
		// Initialize all Vulkan resources used by the ui
		void InitResources(VkQueue copyQueue);
		void UpdateDescriptor();

		VkDescriptorImageInfo* GetImguiDscInfo() const { return m_FontDescriptor; }

		// Starts a new imGui frame and sets up windows and ui elements
		void NewImGuiFrame(FrameInfo& frameInfo, bool updateFrameGraph);
		
		// Update vertex and index buffer containing the imGui elements when required
		void UpdateBuffers(FrameInfo& frameInfo);

		void UpdateImgui(VkExtent2D windowSize);

		// Draw current imGui frame into a command buffer
		void DrawFrame(VkCommandBuffer commandBuffer, FrameInfo& frameInfo);

	private:
		// Vulkan resources for rendering the UI
		VkSampler m_Sampler;
		std::vector<std::shared_ptr<Buffer>> m_VertexBuffers;
		std::vector<std::shared_ptr<Buffer>> m_IndexBuffers;
		int32_t m_VertexCount = 0;
		int32_t m_IndexCount = 0;

		VkDeviceMemory m_FontMemory = VK_NULL_HANDLE;
		VkImage m_FontImage = VK_NULL_HANDLE;
		VkImageView m_FontView = VK_NULL_HANDLE;
		VkDescriptorImageInfo* m_FontDescriptor;

		Device& m_Device;
		PhDevice& m_PhDevice;
	};
}