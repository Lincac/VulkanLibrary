#pragma once

#include "graph/GraphTypes.h"

#include <cstdint>
#include <vector>

namespace mat::demo {

    struct VertexAttribute {
        char name[kMaxVertexAttributeNameLen] = "attr";
        int channelCount = 1;
        float values[kMaxVertexAttributeChannels]{};
    };

    struct GraphNode {
        int id = 0;
        NodeType type = NodeType::VkPipeline;
        float worldX = 0.f;
        float worldY = 0.f;
        int renderPassIndex = 0;
        int inputAssemblyTopology = 5;
        bool inputAssemblyprimitiveRestart = false;
        int viewportCount = 1;
        int scissorCount = 1;
        bool rasterizerDepthClampEnable = false;
        bool rasterizerDiscard = false;
        int rasterizerPolygonMode = 0;
        float rasterizerLineWidth = 1.0f;
        int rasterizerCullMode = 2;
        int rasterizerFrontFace = 0;
        bool rasterizerDepthBiasEnable = false;
        bool multisampleSampleShadingEnable = false;
        int multisampleRasterizationSamples = 0;
        bool depthStencilDepthTestEnable = true;
        bool depthStencilDepthWriteEnable = true;
        int depthStencilDepthCompareOp = 1;
        bool depthStencilDepthBoundsTestEnable = false;
        bool depthStencilStencilTestEnable = false;
        bool colorBlendLogicOpEnable = false;
        int colorBlendLogicOp = 3;
        float colorBlendConstantR = 0.f;
        float colorBlendConstantG = 0.f;
        float colorBlendConstantB = 0.f;
        float colorBlendConstantA = 0.f;
        bool colorBlendAttachmentBlendEnable = false;
        int descriptorSetLayoutBindingBinding = 1;
        int descriptorSetLayoutBindingDescriptorCount = 1;
        int descriptorSetLayoutBindingDescriptorType = 1;
        int descriptorSetLayoutBindingStageFlags = 4;
        int shaderStage = 0;
        char shaderStageEntryName[kMaxShaderStageEntryNameLen] = "main";
        char shaderModulePath[kMaxShaderModulePathLen]{};
        std::vector<VertexAttribute> vertexAttributes;
        int renderPassAttachmentSlotCount = 1;
        int attachmentDescriptionFormat = 0;
        int attachmentDescriptionSamples = 0;
        int attachmentDescriptionLoadOp = 1;
        int attachmentDescriptionStoreOp = 0;
        int attachmentDescriptionStencilLoadOp = 2;
        int attachmentDescriptionStencilStoreOp = 1;
        int attachmentDescriptionInitialLayout = 0;
        int attachmentDescriptionFinalLayout = 9;
        int subpassDescriptionPipelineBindPoint = 0;
        int subpassDependencySrcSubpass = -1;
        int subpassDependencyDstSubpass = 0;
        int subpassDependencySrcStageMask = 0x00020400;
        int subpassDependencySrcAccessMask = 0x00000010;
        int subpassDependencyDstStageMask = 0x00010400;
        int subpassDependencyDstAccessMask = 0x00000050;
        int attachmentReferenceAttachment = 0;
        int attachmentReferenceLayout = 2;
        int framebufferWidth = 800;
        int framebufferHeight = 600;
        int framebufferLayers = 1;
        int imageWidth = 800;
        int imageHeight = 600;
        int imageTiling = 0;
        char imagePath[kMaxImagePathLen]{};
        int imageViewViewType = 1;
        int imageViewFormat = 0;
        int imageViewBaseMipLevel = 0;
        int imageViewLevelCount = 1;
        int imageViewBaseArrayLayer = 0;
        int imageViewLayerCount = 1;
        int samplerMagFilter = 1;
        int samplerMinFilter = 1;
        int samplerAddressModeU = 0;
        int samplerAddressModeV = 0;
        int samplerAddressModeW = 0;
        bool samplerAnisotropyEnable = true;
        int samplerBorderColor = 0;
        bool samplerUnnormalizedCoordinates = false;
        bool samplerCompareEnable = false;
        int samplerCompareOp = 7;
        int samplerMipmapMode = 1;
        int writeDescriptorSetDstSet = 0;
        int writeDescriptorSetDstBinding = 0;
        int writeDescriptorSetDstArrayElement = 0;
        int writeDescriptorSetDescriptorType = 6;
        int writeDescriptorSetDescriptorCount = 1;
        int bufferSharingMode = 0;
        int descriptorBufferOffset = 0;
        int descriptorImageLayout = 5;
        int structInputSlotCount = 0;
        int renderDrawPipelineSlotCount = 0;
        int clearValueInputSlotCount = 0;
        float matrix4x4[kMatrix4x4ElementCount]{};
        float matrix3x3[kMatrix3x3ElementCount]{};
        float vector4[kVector4ElementCount]{};
        float vector3[kVector3ElementCount]{};
        float vector2[kVector2ElementCount]{};
        float scalarFloat = 0.f;
        int scalarInt = 0;
    };

    struct GraphLink {
        int id = 0;
        int fromNodeId = 0;
        int fromPinIndex = 0;
        int toNodeId = 0;
        int toPinIndex = 0;
    };

    class GraphDocument {
    public:
        int addNode(NodeType type, float worldX, float worldY);
        int addLink(int fromNodeId, int fromPinIndex, int toNodeId, int toPinIndex);

        GraphNode* findNode(int nodeId);
        const GraphNode* findNode(int nodeId) const;
        bool setNodePosition(int nodeId, float worldX, float worldY);
        bool removeNode(int nodeId);
        bool addRenderPassAttachmentSlot(int nodeId);
        bool removeRenderPassAttachmentSlot(int nodeId, int slotIndex);
        bool addStructInputSlot(int nodeId);
        bool removeStructInputSlot(int nodeId, int slotIndex);
        bool addRenderDrawPipelineSlot(int nodeId);
        bool removeRenderDrawPipelineSlot(int nodeId, int slotIndex);
        bool addClearValueInputSlot(int nodeId);
        bool removeClearValueInputSlot(int nodeId, int slotIndex);

        const std::vector<GraphNode>& nodes() const { return _nodes; }
        const std::vector<GraphLink>& links() const { return _links; }

    private:
        void removeLinksToInput(int toNodeId, int toPinIndex);
        void removeLinksFromOutput(int fromNodeId, int fromPinIndex);

        std::vector<GraphNode> _nodes;
        std::vector<GraphLink> _links;
        int _nextNodeId = 1;
        int _nextLinkId = 1;
    };

    int vertexNodeBodyRowCount(const GraphNode& node);
    ImVec2 vertexNodeWorldSize(const GraphNode& node);
    ImVec2 nodeWorldSize(const GraphNode& node);
    void initDefaultVertexAttributes(GraphNode& node);
    void initIdentityMatrix4x4(GraphNode& node);
    void initIdentityMatrix3x3(GraphNode& node);

    int structNodeBodyRowCount(const GraphNode& node);
    int renderDrawNodeBodyRowCount(const GraphNode& node);
    int clearValueNodeBodyRowCount(const GraphNode& node);
    int graphNodeInputPinCount(const GraphNode& node);
    int graphNodeInputPinBodyRow(const GraphNode& node, int pinIndex);
    bool graphNodeInputPinAllowsMultipleLinks(const GraphNode& node, int pinIndex);
    bool graphNodeGetInputPin(const GraphNode& node, int pinIndex, NodeInputPinInfo& out);
    bool graphNodeInputPinAcceptsSource(const GraphNode& inputNode, int inputPinIndex, NodeType sourceType,
                                        int sourcePinIndex);
    bool graphNodeInputPinAcceptsSource(const GraphDocument& document, const GraphNode& inputNode, int inputPinIndex,
                                        NodeType sourceType, int sourcePinIndex);
    int graphNodeInputPinIndexForType(const GraphNode& node, NodeType slotType, int slotSourcePinIndex = -1);
    int graphNodeInputPinIndexForType(const GraphDocument& document, const GraphNode& node, NodeType slotType,
                                      int slotSourcePinIndex = -1);
    int renderPassNodeBodyRowCount(const GraphNode& node);

}  // namespace mat::demo
