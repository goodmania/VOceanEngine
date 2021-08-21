#pragma once

#include "Renderer/GraphicsPipeline.h"

namespace voe {

    class VulkanRenderer
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
        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);

        Device& m_Device;

        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
        VkPipelineLayout m_PipelineLayout;
    };
}  // namespace lve