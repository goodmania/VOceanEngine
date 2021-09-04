#pragma once

#include "Renderer/GraphicsPipeline.h"

namespace voe {

    class Camera;
    class DescriptorLayoutCache;
    class DescriptorAllocator;

    class VOE_API VulkanRenderer
    {

    public:
        VulkanRenderer(Device& device, VkRenderPass renderPass);
        ~VulkanRenderer();

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        void RenderGameObjects(
            VkCommandBuffer commandBuffer,
            std::vector<GameObject>& gameObjects,
            const Camera& camera);

    private:
        void InitDescriptors();
        void CreateDescriptorSet();
        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);

        Device& m_Device;

        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
        VkPipelineLayout m_PipelineLayout;

        // descriptor
        DescriptorAllocator* m_DescriptorAllocator;
        DescriptorLayoutCache* m_DescriptorLayoutCache;
    };
}  // namespace lve