#pragma once

#include "Renderer/GraphicsPipeline.h"

namespace voe {

    class Camera;
    class DescriptorLayoutCache;
    class DescriptorAllocator;
    class HeightMap;
    class FrameInfo;

    class VOE_API VulkanModelRenderer
    {
    public:
        struct Semaphores
        {
            VkSemaphore Ready{ 0L };
            VkSemaphore Complete{ 0L };
        };

    public:
        VulkanModelRenderer(Device& device, VkRenderPass renderPass);
        ~VulkanModelRenderer();
        VulkanModelRenderer(const VulkanModelRenderer&) = delete;
        VulkanModelRenderer& operator=(const VulkanModelRenderer&) = delete;

        void RenderGameObjects(FrameInfo frameInfo, std::vector<GameObject>& gameObjects);

        void OnUpdate(float dt, FrameInfo& frameInfo);

    private:
        void InitDescriptors();
        void CreateDescriptorSets();
        void CreateGraphicsUbo();
        void UpdateGlobalUboBuffers(FrameInfo& frameInfo);


        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineCache();

        Device& m_Device;

        // pipelines
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
        VkPipelineCache m_PipelineCache;

        // descriptor helpers
        DescriptorAllocator* m_DescriptorAllocator;
        DescriptorLayoutCache* m_DescriptorLayoutCache;

        // pipeline, descriptor layout
        std::vector<VkDescriptorSet> m_DescriptorSets;
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

        VkPipelineLayout m_GraphicsPipelineLayout;
        VkDescriptorSet  m_GraphicsDescriptorSet;
        VkDescriptorSetLayout m_GraphicsDescriptorSetLayout;
        
        std::vector<std::shared_ptr<Buffer>> m_GlobalUboBuffers;
        VkDescriptorBufferInfo* m_GlobalUboDscInfo = VK_NULL_HANDLE;

        Semaphores m_ImageTransitionSemaphores;

        // Command buffer for image transitions
        VkCommandPool m_ImageTransitionCommandPool;
        std::array<VkCommandBuffer, 2> m_ImageTransitionCommandBuffers;
    };
}  