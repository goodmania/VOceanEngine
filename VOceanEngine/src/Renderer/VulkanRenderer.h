#pragma once

#include "Renderer/GraphicsPipeline.h"
#include "Renderer/ComputePipeline.h"

namespace voe {

    class Camera;
    class DescriptorLayoutCache;
    class DescriptorAllocator;
    class HeightMap;
    class FrameInfo;

    class VOE_API VulkanRenderer
    {
    public:
        struct Semaphores
        {
            VkSemaphore Ready{ 0L };
            VkSemaphore Complete{ 0L };
        };

    public:
        VulkanRenderer(Device& device, VkRenderPass renderPass);
        ~VulkanRenderer();
        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        void RenderGameObjects(FrameInfo frameInfo, std::vector<GameObject>& gameObjects);

        bool IsComputeQueueSpecialized() const;
        void OnUpdate(float dt, int frameIndex);
        void BuildComputeCommandBuffer();

        Semaphores GetComputeSemaphores() { return m_ComputeSemaphores; }
        std::array<VkCommandBuffer, 2> GetComputeCommandBuffer() { return m_ComputeCommandBuffers; }
        const uint32_t GetOceanMeshSize() { return m_GroupSize; }

    private:
        void InitOceanHeightMap();
        void InitDescriptors();
        void SetupFFTOceanComputePipelines();

        void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer);
        void AddComputeToComputeBarriers(VkCommandBuffer commandBuffer, VkBuffer InputBuffer, VkBuffer OutputBuffer);
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
        std::unique_ptr<HeightMap> m_OceanHeightMap;

        // 512->256
        const uint32_t m_GroupSize = 256;
 
        std::array<VkDescriptorSet, 3> m_DescriptorSets;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        VkDescriptorSetLayout m_GraphicsDescriptorSetLayout;

        Semaphores m_ComputeSemaphores;

        // maybe the following variables should be moved to a Device class ?
        VkCommandPool m_ComputeCommandPool;
        std::array<VkCommandBuffer, 2> m_ComputeCommandBuffers;
    };
}  