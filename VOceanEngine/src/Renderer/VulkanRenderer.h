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
        void OnUpdate(float dt, FrameInfo& frameInfo);
        void BuildComputeCommandBuffer();

        Semaphores GetComputeSemaphores() { return m_ComputeSemaphores; }
        std::array<VkCommandBuffer, 2> GetComputeCommandBuffer() { return m_ComputeCommandBuffers; }
        const uint32_t GetGridSize() { return m_GroupSize; }
        const uint32_t GetOceanSize() { return m_GroupSize * 5 / 2; }

    private:
        void InitOceanHeightMap();
        void InitDescriptors();
        void CreateDescriptorSets();
        void CreateGraphicsUbo();
        void UpdateGlobalUboBuffers(FrameInfo& frameInfo);
        void SetupFFTOceanComputePipelines();

        void AddGraphicsToComputeBarriers(VkCommandBuffer commandBuffer, uint32_t index);
        void AddComputeToComputeBarriers(VkCommandBuffer commandBuffer, VkBuffer InputBuffer, VkBuffer OutputBuffer);
        void AddComputeToGraphicsBarriers(VkCommandBuffer commandBuffer, uint32_t index);

        void CreatePipelineLayout();
        void CreatePipeline(VkRenderPass renderPass);
        void CreatePipelineCache();

        Device& m_Device;
        bool m_SpecializedComputeQueue = false;

        // 512->256
        const uint32_t m_GroupSize = 256;

        // ocean params
        std::unique_ptr<HeightMap> m_OceanHeightMap;

        // pipelines
        std::unique_ptr<ComputePipeline> m_ComputePipeline;
        std::unique_ptr<ComputePipeline> m_FFTComputePipeline;
        std::unique_ptr<ComputePipeline> m_ComputeNormalPipeline;
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
        VkPipelineCache m_PipelineCache;

        // descriptor helpers
        DescriptorAllocator* m_DescriptorAllocator;
        DescriptorLayoutCache* m_DescriptorLayoutCache;

        // pipeline, descriptor layout
        VkPipelineLayout m_ComputePipelineLayout;
        std::vector<VkDescriptorSet> m_DescriptorSets;
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

        VkPipelineLayout m_GraphicsPipelineLayout;
        VkDescriptorSet  m_GraphicsDescriptorSet;
        VkDescriptorSetLayout m_GraphicsDescriptorSetLayout;
        
        std::vector<std::shared_ptr<Buffer>> m_GlobalUboBuffers;
        VkDescriptorBufferInfo* m_GlobalUboDscInfo = VK_NULL_HANDLE;

        Semaphores m_ComputeSemaphores;

        // maybe the following variables should be moved to a Device class ?
        VkCommandPool m_ComputeCommandPool;
        std::array<VkCommandBuffer, 2> m_ComputeCommandBuffers;
    };
}  