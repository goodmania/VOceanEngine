#pragma once

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/ComputePipeline.h"

namespace voe {

    class Camera;
    class DescriptorLayoutCache;
    class DescriptorAllocator;
    class HeightMap;

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

        bool IsComputeQueueSpecialized() const;
        void OnUpdate(float dt);
        void BuildComputeCommandBuffer();

    private:
        void InitOceanH0Param();
        void InitDescriptors();
        void SetupFFTOceanComputePipelines();
        void SetupUniformBuffers();

        void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer);
        void AddComputeToComputeBarriers(VkCommandBuffer commandBuffer);
        void AddComputeToGraphicsBarriers(VkCommandBuffer commandBuffer);

        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineCache();

        Device& m_Device;
        bool m_SpecializedComputeQueue = false;

        // pipelines
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
        VkPipelineLayout m_GraphicsPipelineLayout;

        std::unique_ptr<ComputePipeline> m_ComputePipeline;
        std::unique_ptr<ComputePipeline> m_FFTComputePipeline;
        VkPipelineLayout m_ComputePipelineLayout;

        VkPipelineCache m_PipelineCache;

        // descriptor helpers
        DescriptorAllocator* m_DescriptorAllocator;
        DescriptorLayoutCache* m_DescriptorLayoutCache;

        // ocean params
        std::unique_ptr<HeightMap> m_OceanH0;

        // 512->256
        const uint32_t m_OceanThreadsSize = 256;
        uint32_t m_ReadSet = 0;
        std::array<VkDescriptorSet, 2> m_DescriptorSets;
        VkDescriptorSetLayout m_DescriptorSetLayout;

        struct Semaphores
        {
            VkSemaphore Ready{ 0L };
            VkSemaphore Complete{ 0L };
        } m_Semaphores;

        // maybe the following variables should be moved to a Device class ?
        VkCommandPool m_ComputeCommandPool;
        VkCommandBuffer m_ComputeCommandBuffer;
    };
}  