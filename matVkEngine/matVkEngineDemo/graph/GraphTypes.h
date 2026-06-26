#pragma once

#include <cstdint>
#include <imgui.h>
namespace mat::demo {

    enum class NodeType {
        VkPipeline,
        VkPipelineShaderStage,
        VkShaderModule,
        Vertex,
        VkPipelineVertexInputState,
        VkPipelineInputAssemblyState,
        VkPipelineViewportState,
        VkPipelineRasterizationState,
        VkPipelineMultisampleState,
        VkPipelineDepthStencilState,
        VkPipelineColorBlendState,
        VkPipelineColorBlendAttachmentState,
        VkColorWriteMask,
        VkDynamicState,
        VkPipelineDynamicState,
        VkDescriptorSetLayoutBinding,
        VkDescriptorSetLayout,
        VkBuffer,
        VkWriteDescriptorSet,
        VkDescriptorBuffer,
        VkDescriptorImage,
        VkPipelineLayout,
        VkRenderPass,
        VkAttachmentDescription,
        VkSubpassDescription,
        VkSubpassDependency,
        VkAttachmentReference,
        VkFormat,
        VkImageUsage,
        VkImageAspect,
        VkBufferUsage,
        VkMemoryProperty,
        VkImage,
        VkImageView,
        VkFramebuffer,
        VkSampler,
        Matrix4x4,
        Matrix3x3,
        Vector4,
        Vector3,
        Vector2,
        Float,
        Int,
        Struct,
    };

    constexpr float kNodeWidth = 280.f;
    constexpr float kNodeHeaderHeight = 30.f;
    constexpr float kNodePinRowHeight = 22.f;
    constexpr float kNodeEmptyBodyHeight = 24.f;
    constexpr int kVkPipelineInputPinCount = 11;
    constexpr int kVkRenderPassAttachmentHeaderRowCount = 1;
    constexpr int kVkRenderPassAddAttachmentRowCount = 1;
    constexpr int kVkRenderPassFixedInputPinCount = 2;
    constexpr int kVkAttachmentDescriptionParamCount = 8;
    constexpr int kVkSubpassDescriptionParamCount = 1;
    constexpr int kVkSubpassDescriptionInputPinCount = 2;
    constexpr int kVkSubpassDependencyParamCount = 6;
    constexpr int kVkAttachmentReferenceParamCount = 2;
    constexpr int kVkFormatOutputPinCount = 10;
    constexpr int kVkImageUsageOutputPinCount = 8;
    constexpr int kVkImageAspectOutputPinCount = 4;
    constexpr int kVkBufferUsageOutputPinCount = 9;
    constexpr int kVkMemoryPropertyOutputPinCount = 6;
    constexpr int kVkImagePrefixParamCount = 3;
    constexpr int kVkImageInputPinCount = 2;
    constexpr int kVkImageSuffixParamCount = 2;
    constexpr int kVkImageTilingOptionCount = 2;
    constexpr int kVkImageViewPrefixParamCount = 1;
    constexpr int kVkImageViewInputPinCount = 2;
    constexpr int kVkImageViewSuffixParamCount = 6;
    constexpr int kVkImageViewTypeOptionCount = 7;
    constexpr int kVkFramebufferParamCount = 4;
    constexpr int kVkFramebufferInputPinCount = 2;
    constexpr int kVkSamplerParamCount = 12;
    constexpr int kVkFilterOptionCount = 2;
    constexpr int kVkSamplerAddressModeOptionCount = 5;
    constexpr int kVkBorderColorOptionCount = 6;
    constexpr int kVkSamplerMipmapModeOptionCount = 2;
    constexpr int kVkAttachmentLoadOpOptionCount = 3;
    constexpr int kVkAttachmentStoreOpOptionCount = 2;
    constexpr int kVkImageLayoutOptionCount = 10;
    constexpr int kVkPipelineBindPointOptionCount = 2;
    constexpr int kVkPipelineIndexRowCount = 1;
    constexpr int kVkPipelineShaderStagePrefixParamCount = 2;
    constexpr int kVkPipelineShaderStageInputPinCount = 1;
    constexpr int kVkPipelineShaderStageSuffixParamCount = 1;
    constexpr int kVkShaderModuleParamCount = 1;
    constexpr int kVkPipelineVertexInputStateParamCount = 1;
    constexpr int kVkPipelineVertexInputStateInputPinCount = 2;
    constexpr int kVertexAddItemRowCount = 1;
    constexpr int kVertexOutputPinCount = 2;
    constexpr int kMaxVertexAttributeNameLen = 32;
    constexpr int kMaxVertexAttributeChannels = 4;
    constexpr int kMinVertexAttributeChannels = 1;
    constexpr int kMaxShaderModulePathLen = 260;
    constexpr int kMaxImagePathLen = 260;
    constexpr int kMaxShaderStageEntryNameLen = 64;
    constexpr int kVkPipelineInputAssemblyStateParamCount = 3;
    constexpr int kVkPipelineViewportStateParamCount = 3;
    constexpr int kVkPipelineRasterizationStateParamCount = 8;
    constexpr int kVkPipelineMultisampleStateParamCount = 3;
    constexpr int kVkPipelineDepthStencilStateParamCount = 6;
    constexpr int kVkPipelineColorBlendStateParamCount = 7;
    constexpr int kVkPipelineColorBlendStateInputPinCount = 1;
    constexpr int kVkPipelineColorBlendAttachmentStateParamCount = 1;
    constexpr int kVkPipelineColorBlendAttachmentStateInputPinCount = 1;
    constexpr int kVkColorWriteMaskOutputPinCount = 4;
    constexpr int kVkDynamicStateOutputPinCount = 5;
    constexpr int kVkPipelineDynamicStateParamCount = 1;
    constexpr int kVkPipelineDynamicStateInputPinCount = 1;
    constexpr int kVkPipelineLayoutParamCount = 1;
    constexpr int kVkPipelineLayoutInputPinCount = 1;
    constexpr int kVkDescriptorSetLayoutParamCount = 1;
    constexpr int kVkDescriptorSetLayoutInputPinCount = 1;
    constexpr int kVkDescriptorSetLayoutBindingParamCount = 5;
    constexpr int kVkBufferPrefixParamCount = 1;
    constexpr int kVkBufferInputPinCount = 3;
    constexpr int kVkBufferSuffixParamCount = 1;
    constexpr int kVkBufferSharingModeOptionCount = 2;
    constexpr int kVkWriteDescriptorSetParamCount = 6;
    constexpr int kVkWriteDescriptorSetInputPinCount = 2;
    constexpr int kVkDescriptorBufferParamCount = 1;
    constexpr int kVkDescriptorBufferInputPinCount = 1;
    constexpr int kVkDescriptorImageParamCount = 1;
    constexpr int kVkDescriptorImageInputPinCount = 2;
    constexpr int kVkDescriptorTypeOptionCount = 11;
    constexpr int kStructAddInputRowCount = 1;
    constexpr int kMatrix4x4RowCount = 4;
    constexpr int kMatrix3x3RowCount = 3;
    constexpr int kScalarValueNodeBodyRowCount = 1;
    constexpr int kMatrix4x4ElementCount = 16;
    constexpr int kMatrix3x3ElementCount = 9;
    constexpr int kVector4ElementCount = 4;
    constexpr int kVector3ElementCount = 3;
    constexpr int kVector2ElementCount = 2;
    constexpr int kVkShaderStageFlagOptionCount = 7;
    constexpr int kVkPrimitiveTopologyOptionCount = 10;
    constexpr int kVkPolygonModeOptionCount = 3;
    constexpr int kVkCullModeOptionCount = 4;
    constexpr int kVkFrontFaceOptionCount = 2;
    constexpr int kVkSampleCountOptionCount = 7;
    constexpr int kVkCompareOpOptionCount = 8;
    constexpr int kVkLogicOpOptionCount = 16;

    extern const char kVkPipelineVertexInputStateSType[];
    extern const char kVkPipelineInputAssemblyStateSType[];
    extern const char kVkPipelineShaderStageSType[];
    extern const char kVkPipelineViewportStateSType[];
    extern const char kVkPipelineRasterizationStateSType[];
    extern const char kVkPipelineMultisampleStateSType[];
    extern const char kVkPipelineDepthStencilStateSType[];
    extern const char kVkPipelineColorBlendStateSType[];
    extern const char kVkPipelineDynamicStateSType[];
    extern const char kVkPipelineLayoutSType[];
    extern const char kVkDescriptorSetLayoutSType[];
    extern const char kVkDescriptorSetLayoutBindingNullSampler[];
    extern const char kVkWriteDescriptorSetSType[];
    extern const char kVkFramebufferSType[];
    extern const char kVkImageSType[];
    extern const char kVkImageViewSType[];
    extern const char kVkSamplerSType[];
    extern const char kVkBufferSType[];

    struct NodeInputPinInfo {
        const char* label = "";
        NodeType slotType = NodeType::VkPipeline;
        int slotSourcePinIndex = -1;
    };

    struct NodeInputPinDef {
        const char* label;
        NodeType slotType;
        int slotSourcePinIndex = -1;
    };

    const char* nodeTypeName(NodeType type);
    ImVec2 nodeWorldSize(NodeType type);
    bool nodeHasOutputPin(NodeType type);
    bool nodeHasInputPins(NodeType type);
    bool nodeInputPinAllowsMultipleLinks(NodeType type, int pinIndex);
    int nodeInputPinCount(NodeType type);
    int nodeInputPinBodyRow(NodeType type, int pinIndex);
    const NodeInputPinDef* nodeInputPin(NodeType type, int index);
    bool nodeInputPinAcceptsSource(NodeType inputNodeType, int inputPinIndex, NodeType sourceType,
                                   int sourcePinIndex);
    int nodeInputPinIndexForType(NodeType nodeType, NodeType slotType, int slotSourcePinIndex = -1);
    int nodeOutputPinCount(NodeType type);
    const char* nodeOutputPinLabel(NodeType type, int pinIndex);
    NodeType pinLinkTargetNodeTypeForSource(NodeType sourceType);
    bool isStructValueNodeType(NodeType type);
    bool isScalarValueNodeType(NodeType type);
    int valueNodeBodyRowCount(NodeType type);
    const char* vkPrimitiveTopologyOptionName(int index);
    const char* vkPolygonModeOptionName(int index);
    const char* vkCullModeOptionName(int index);
    const char* vkFrontFaceOptionName(int index);
    const char* vkBool32OptionName(bool value);
    const char* vkSampleCountOptionName(int index);
    const char* vkCompareOpOptionName(int index);
    const char* vkLogicOpOptionName(int index);
    const char* vkDescriptorTypeOptionName(int index);
    const char* vkShaderStageFlagOptionName(int index);
    const char* vkAttachmentLoadOpOptionName(int index);
    const char* vkAttachmentStoreOpOptionName(int index);
    const char* vkImageLayoutOptionName(int index);
    const char* vkPipelineBindPointOptionName(int index);
    const char* vkImageViewTypeOptionName(int index);
    const char* vkImageTilingOptionName(int index);
    const char* vkBufferSharingModeOptionName(int index);
    const char* vkFilterOptionName(int index);
    const char* vkSamplerAddressModeOptionName(int index);
    const char* vkBorderColorOptionName(int index);
    const char* vkSamplerMipmapModeOptionName(int index);
    int vkFormatOutputPinValue(int pinIndex);
    uint32_t vkImageUsageOutputPinValue(int pinIndex);
    uint32_t vkImageAspectOutputPinValue(int pinIndex);
    uint32_t vkBufferUsageOutputPinValue(int pinIndex);
    uint32_t vkMemoryPropertyOutputPinValue(int pinIndex);

}  // namespace mat::demo
