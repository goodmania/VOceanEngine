#pragma once
#include "Renderer/ImguiPipeline.h"

namespace voe {

    class Camera;
    class DescriptorLayoutCache;
    class DescriptorAllocator;
    class PhDevice;
    class FrameInfo;
    class ImGUI;
    class ImGuiPipeline;

    class VOE_API VulkanImguiRenderer
    {
    public:
        VulkanImguiRenderer(Device& device, PhDevice& phDevice, VkRenderPass renderPass, VkExtent2D windowSize);
        ~VulkanImguiRenderer();
        VulkanImguiRenderer(const VulkanImguiRenderer&) = delete;
        VulkanImguiRenderer& operator=(const VulkanImguiRenderer&) = delete;

        void RenderImgui(FrameInfo frameInfo/*, std::vector<GameObject>& gameObjects*/);
        void OnUpdate(float dt, FrameInfo& frameInfo, VkExtent2D windowSize);

    private:
        void InitImgui();
        void InitDescriptors();
        void CreateImguiUBO();
        void CreateDescriptorSets();

        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineCache();

        Device& m_Device;
        PhDevice& m_PhDevice;
        VkExtent2D m_WindowSize;

        // imgui
        std::unique_ptr<ImGUI> m_Imgui;

        // pipelines
        std::unique_ptr<ImguiPipeline> m_ImguiPipeline;
        VkPipelineCache m_PipelineCache;
        VkPipelineLayout m_PipelineLayout;

        // descriptor helpers
        DescriptorAllocator* m_DescriptorAllocator;
        DescriptorLayoutCache* m_DescriptorLayoutCache;

        // descriptor
        VkDescriptorSet m_DescriptorSet;
        VkDescriptorSetLayout m_DescriptorSetLayout;

        // UBO
        std::vector<std::shared_ptr<Buffer>> m_ImguiUBO;
        VkDescriptorBufferInfo* m_ImguiUBODscInfo;
    };
}  