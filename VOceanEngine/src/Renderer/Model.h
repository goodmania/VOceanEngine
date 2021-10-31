#pragma once

namespace voe 
{
    class Device;
    class Buffer;

    class VOE_API Model 
    {
    public:
        struct Vertex 
        {
            glm::vec4 position{};
            glm::vec4 color{};
            glm::vec4 normal{};
            glm::vec2 uv{};

            static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

            bool operator==(const Vertex& other) const 
            {
                return position == other.position && color == other.color && normal == other.normal &&
                    uv == other.uv;
            }
        };

        struct Builder 
        {
            std::vector<Vertex> vertices{};
            std::vector<uint32_t> indices{};

            void LoadModel(const std::string& filepath);
            void CreateXZPlaneModel(uint32_t w, uint32_t h);
        };

        Model(Device& device, const Model::Builder& builder);
        ~Model();

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        static std::unique_ptr<Model> CreateModelFromFile(Device& device, const std::string& filepath);
        static std::unique_ptr<Model> CreateXZPlaneModelFromProcedural(Device& device, uint32_t w, uint32_t h);
        
        void Bind(VkCommandBuffer commandBuffer);
        void Draw(VkCommandBuffer commandBuffer);

    private:
        void CreateVertexBuffers(const std::vector<Vertex>& vertices);
        void CreateIndexBuffers(const std::vector<uint32_t>& indices);

        Device& m_Device;

        std::unique_ptr<Buffer> m_VertexBuffer;
        uint32_t m_VertexCount;

        std::unique_ptr<Buffer> m_IndexBuffer;
        uint32_t m_IndexCount;
        bool m_HasIndexBuffer = false;
    };
}  