#include "PreCompileHeader.h"
#include <imgui.h>

#include "VoeImgui.h"
#include "VulkanCore/Device.h"
#include "Renderer/FrameInfo.h"
#include "Renderer/Swapchain.h"

#include "VOceanEngine/Input.h"
#include "VOceanEngine/keyCodes.h"
#include "VOceanEngine/MouseCodes.h"

namespace voe {

	// Options and values to display/toggle from the UI
	struct UISettings {
		bool displayModels = false;
		bool displayLogos = false;
		bool displayBackground = false;
		bool animateLight = false;
		float lightSpeed = 0.25f;
		std::array<float, 50> frameTimes{};
		float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
		float lightTimer = 0.0f;

	} uiSettings;

	ImGUI::ImGUI(Device& device, PhDevice& phDevice, float width, float height) : m_Device(device), m_PhDevice(phDevice)
	{
		ImGui::CreateContext();
		Init(width, height);
	};

	ImGUI::~ImGUI()
	{
		ImGui::DestroyContext();
		// Release all Vulkan resources required for rendering imGui
		vkDestroyImage(m_Device.GetVkDevice(), m_FontImage, nullptr);
		vkDestroyImageView(m_Device.GetVkDevice(), m_FontView, nullptr);
		vkFreeMemory(m_Device.GetVkDevice(), m_FontMemory, nullptr);
		vkDestroySampler(m_Device.GetVkDevice(), m_Sampler, nullptr);
		delete m_FontDescriptor;
	}

	void ImGUI::Init(float width, float height)
	{
		// Color scheme
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		// Dimensions
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(width, height);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
		//Descriptor
		m_FontDescriptor = new VkDescriptorImageInfo();

		// init vertex, index buffer
		m_VertexBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
		m_IndexBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);
	}

	void ImGUI::InitResources(VkQueue copyQueue)
	{
		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		unsigned char* fontData;
		int texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

		// Create target image for copy
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VOE_CHECK_RESULT(vkCreateImage(m_Device.GetVkDevice(), &imageInfo, nullptr, &m_FontImage));

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(m_Device.GetVkDevice(), m_FontImage, &memReqs);

		VkMemoryAllocateInfo memAllocInfo = {};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = m_PhDevice.FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VOE_CHECK_RESULT(vkAllocateMemory(m_Device.GetVkDevice(), &memAllocInfo, nullptr, &m_FontMemory));
		VOE_CHECK_RESULT(vkBindImageMemory(m_Device.GetVkDevice(), m_FontImage, m_FontMemory, 0));

		// Image view
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_FontImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		VOE_CHECK_RESULT(vkCreateImageView(m_Device.GetVkDevice(), &viewInfo, nullptr, &m_FontView));

		// Staging buffers for font data upload
		Buffer stagingBuffer
		{
			m_Device,
			sizeof(char),
			static_cast<uint32_t>(texWidth * texHeight * 4),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)fontData);

		// Copy buffer data to font image
		VkCommandBuffer copyCmd = m_Device.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// Prepare for transfer
		voe::tools::setImageLayout(
			copyCmd,
			m_FontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);

		// Copy
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = texWidth;
		bufferCopyRegion.imageExtent.height = texHeight;
		bufferCopyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer.GetBuffer(),
			m_FontImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion
		);

		// Prepare for shader read
		voe::tools::setImageLayout(
			copyCmd,
			m_FontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		m_Device.FlushCommandBuffer(copyCmd, copyQueue, true);

		// Font texture Sampler
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VOE_CHECK_RESULT(vkCreateSampler(m_Device.GetVkDevice(), &samplerInfo, nullptr, &m_Sampler));

		UpdateDescriptor();
	}

	void ImGUI::UpdateDescriptor()
	{
		m_FontDescriptor->sampler = m_Sampler;
		m_FontDescriptor->imageView = m_FontView;
		m_FontDescriptor->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void ImGUI::UpdateImgui(VkExtent2D windowSize)
	{
		// Update imGui
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(windowSize.width, windowSize.height);
		io.MousePos = ImVec2(Input::GetMouseX(), Input::GetMouseY());
		io.MouseDown[0] = Input::IsMouseButtonPressed(Mouse::ButtonLeft);
		io.MouseDown[1] = Input::IsMouseButtonPressed(Mouse::ButtonRight);
	}

	void ImGUI::NewImGuiFrame(FrameInfo& frameInfo, bool updateFrameGraph)
	{
		ImGui::NewFrame();

		// Init imGui windows and elements
		ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Once);	
		ImVec4 clear_color = ImColor(114, 144, 154);

		ImGui::Begin("Info");
		ImGui::TextUnformatted("VOceanEngine - ImGui");
		ImGui::TextUnformatted(m_PhDevice.GetProperties().deviceName);

		glm::vec3 cameraPos = frameInfo.CameraObj.GetCameraPos();
		glm::vec3 cameraRot = frameInfo.CameraObj.GetCameraRotation();

		ImGui::Text("Camera");
		ImGui::InputFloat3("position", &cameraPos.x, 2);
		ImGui::InputFloat3("rotation", &cameraRot.x, 2);
		ImGui::End();

		ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Once);

		// Instructions	
		ImGui::Begin("Instructions");
		ImGui::Text("Camera Movement	: W, A, S, D");
		ImGui::Text("Camera UP / DOWN	: E, F");
		ImGui::Text("Camera Rotate	: Mouse Right Button + Mouse Movement");
		ImGui::Text("Exit	: Esc");
		ImGui::End();

		// Render to generate draw buffers
		ImGui::Render();
	}

	void ImGUI::UpdateBuffers(FrameInfo& frameInfo)
	{
		static bool firstDraw = true;
		ImDrawData* imDrawData = ImGui::GetDrawData();

		// Note: Alignment is done inside buffer creation
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

		if ((vertexBufferSize == 0) || (indexBufferSize == 0))
			return;

		uint32_t frameIndex = frameInfo.FrameIndex;

		// Update buffers only if vertex or index count has been changed compared to current buffer size
		{
			// Vertex buffer
			if ((m_VertexBuffers[frameIndex] == nullptr) || (m_VertexCount != imDrawData->TotalVtxCount))
			{
				m_VertexBuffers[frameIndex].reset();

				m_VertexBuffers[frameIndex] = std::make_unique<Buffer>(
					m_Device,
					vertexBufferSize,
					imDrawData->TotalVtxCount,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				m_VertexCount = imDrawData->TotalVtxCount;
				m_VertexBuffers[frameIndex]->Map();
			}

			// Index buffer
			if ((m_IndexBuffers[frameIndex] == nullptr) || (m_IndexCount < imDrawData->TotalIdxCount))
			{
				m_IndexBuffers[frameIndex].reset();

				m_IndexBuffers[frameIndex] = std::make_unique<Buffer>(
					m_Device,
					indexBufferSize,
					imDrawData->TotalIdxCount,
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				m_IndexCount = imDrawData->TotalIdxCount;
				m_IndexBuffers[frameIndex]->Map();
			}

			// Upload data
			ImDrawVert* vtxDst = (ImDrawVert*)m_VertexBuffers[frameIndex]->GetMappedMemory();
			ImDrawIdx* idxDst = (ImDrawIdx*)m_IndexBuffers[frameIndex]->GetMappedMemory();

			for (int n = 0; n < imDrawData->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[n];
				memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				vtxDst += cmd_list->VtxBuffer.Size;
				idxDst += cmd_list->IdxBuffer.Size;
			}

			// Flush to make writes visible to GPU
			m_VertexBuffers[frameIndex]->Flush();
			m_IndexBuffers[frameIndex]->Flush();
		}
	}

	void ImGUI::DrawFrame(VkCommandBuffer commandBuffer, FrameInfo& frameInfo)
	{
		// Render commands
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;
		uint32_t frameIndex = frameInfo.FrameIndex;

		if (imDrawData->CmdListsCount > 0) 
		{
			VkBuffer vertexBuffer = m_VertexBuffers[frameIndex]->GetBuffer();
			VkBuffer indexBuffer = m_IndexBuffers[frameIndex]->GetBuffer();

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}
	}
}
