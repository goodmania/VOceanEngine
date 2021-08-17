#pragma once

namespace voe {

    class VulkanRenderSystem
    {
        class GraphicsPipeline;

    public:
        VulkanRenderSystem(Device& device, VkRenderPass renderPass);
        ~VulkanRenderSystem();

        VulkanRenderSystem(const VulkanRenderSystem&) = delete;
        VulkanRenderSystem& operator=(const VulkanRenderSystem&) = delete;

        void RenderGameObjects(
            VkCommandBuffer commandBuffer,
            std::vector<GameObject>& gameObjects,
            const Camera& camera);

    private:
        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);

        Device& m_Device;

        std::unique_ptr<GraphicsPipeline> m_GPipeline;
        VkPipelineLayout m_PipelineLayout;
    };
}  // namespace lve