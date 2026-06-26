#include "graph/DefaultDeferredScene.h"

#include <cstdio>

namespace mat::demo {

    namespace {

        constexpr int kFormatPinRgba8 = 1;
        constexpr int kFormatPinRgba16F = 5;
        constexpr int kFormatPinD32 = 8;

        constexpr int kUsagePinColorAttachment = 4;
        constexpr int kUsagePinDepthAttachment = 5;

        constexpr int kAspectPinColor = 0;
        constexpr int kAspectPinDepth = 1;

        constexpr int kLayoutColorAttachment = 2;
        constexpr int kLayoutDepthAttachment = 3;
        constexpr int kLayoutShaderRead = 5;
        constexpr int kLayoutPresent = 9;

        constexpr int kVkFormatRgba8 = 37;
        constexpr int kVkFormatRgba16F = 97;
        constexpr int kVkFormatD32 = 126;

        constexpr int kSceneWidth = 1280;
        constexpr int kSceneHeight = 720;

        struct ImageResource {
            int imageId = -1;
            int imageViewId = -1;
        };

        class DeferredSceneBuilder {
        public:
            explicit DeferredSceneBuilder(GraphDocument& document) : _document(document) {}

            GraphNode* addNode(NodeType type, float worldX, float worldY) {
                const int nodeId = _document.addNode(type, worldX, worldY);
                return _document.findNode(nodeId);
            }

            void link(int fromNodeId, int fromPinIndex, int toNodeId, int toPinIndex) {
                _document.addLink(fromNodeId, fromPinIndex, toNodeId, toPinIndex);
            }

            void linkOutput(int fromNodeId, int toNodeId, int toPinIndex) { link(fromNodeId, 0, toNodeId, toPinIndex); }

            ImageResource addImage2D(float x, float y, int formatPin, int usagePin, int aspectPin, int formatField) {
                GraphNode* formatNode = addNode(NodeType::VkFormat, x, y);
                GraphNode* usageNode = addNode(NodeType::VkImageUsage, x, y + 70.f);
                GraphNode* imageNode = addNode(NodeType::VkImage, x + 340.f, y);
                GraphNode* aspectNode = addNode(NodeType::VkImageAspect, x + 340.f, y + 70.f);
                GraphNode* imageViewNode = addNode(NodeType::VkImageView, x + 680.f, y);

                imageNode->imageWidth = kSceneWidth;
                imageNode->imageHeight = kSceneHeight;
                imageViewNode->imageViewFormat = formatField;
                imageViewNode->imageViewViewType = 1;

                link(formatNode->id, formatPin, imageNode->id, 0);
                link(usageNode->id, usagePin, imageNode->id, 1);
                linkOutput(imageNode->id, imageViewNode->id, 0);
                link(aspectNode->id, aspectPin, imageViewNode->id, 1);

                return {imageNode->id, imageViewNode->id};
            }

            GraphNode* addAttachmentDescription(float x, float y, int vkFormat, int finalLayout) {
                GraphNode* node = addNode(NodeType::VkAttachmentDescription, x, y);
                node->attachmentDescriptionFormat = vkFormat;
                node->attachmentDescriptionLoadOp = 1;
                node->attachmentDescriptionStoreOp = 0;
                node->attachmentDescriptionInitialLayout = 0;
                node->attachmentDescriptionFinalLayout = finalLayout;
                return node;
            }

            GraphNode* addAttachmentReference(float x, float y, int attachmentIndex, int layout) {
                GraphNode* node = addNode(NodeType::VkAttachmentReference, x, y);
                node->attachmentReferenceAttachment = attachmentIndex;
                node->attachmentReferenceLayout = layout;
                return node;
            }

            GraphNode* addShaderModule(float x, float y, const char* path) {
                GraphNode* node = addNode(NodeType::VkShaderModule, x, y);
                std::snprintf(node->shaderModulePath, sizeof(node->shaderModulePath), "%s", path);
                return node;
            }

            GraphNode* addShaderStage(float x, float y, int stage, GraphNode* module) {
                GraphNode* node = addNode(NodeType::VkPipelineShaderStage, x, y);
                node->shaderStage = stage;
                std::snprintf(node->shaderStageEntryName, sizeof(node->shaderStageEntryName), "main");
                linkOutput(module->id, node->id, 0);
                return node;
            }

            GraphNode* addColorBlendAttachment(float x, float y) {
                GraphNode* node = addNode(NodeType::VkPipelineColorBlendAttachmentState, x, y);
                node->colorBlendAttachmentBlendEnable = false;
                return node;
            }

            struct PipelineBuildResult {
                int pipelineId = -1;
            };

            PipelineBuildResult buildGraphicsPipeline(float x, float y, int renderPassId, int renderPassIndex,
                                                    const char* vertPath, const char* fragPath,
                                                    int colorBlendAttachmentCount, bool withVertexInput,
                                                    int pipelineLayoutId) {
                const float dx = -360.f;
                float row = y;

                GraphNode* vertModule = addShaderModule(x + dx, row, vertPath);
                GraphNode* fragModule = addShaderModule(x + dx, row + 90.f, fragPath);
                GraphNode* vertStage = addShaderStage(x + dx + 280.f, row, 0, vertModule);
                GraphNode* fragStage = addShaderStage(x + dx + 280.f, row + 90.f, 4, fragModule);

                GraphNode* vertexInput = nullptr;
                if (withVertexInput) {
                    vertexInput = addNode(NodeType::VkPipelineVertexInputState, x + dx, row + 190.f);
                    GraphNode* vertex = addNode(NodeType::Vertex, x + dx - 320.f, row + 190.f);
                    linkOutput(vertex->id, vertexInput->id, 0);
                    link(1, vertex->id, vertexInput->id, 1);
                } else {
                    vertexInput = addNode(NodeType::VkPipelineVertexInputState, x + dx, row + 190.f);
                }

                GraphNode* inputAssembly = addNode(NodeType::VkPipelineInputAssemblyState, x + dx, row + 280.f);
                inputAssembly->inputAssemblyTopology = withVertexInput ? 5 : 3;

                GraphNode* viewportState = addNode(NodeType::VkPipelineViewportState, x + dx, row + 370.f);
                GraphNode* rasterState = addNode(NodeType::VkPipelineRasterizationState, x + dx, row + 460.f);
                rasterState->rasterizerCullMode = withVertexInput ? 2 : 0;

                GraphNode* multisampleState = addNode(NodeType::VkPipelineMultisampleState, x + dx, row + 550.f);
                GraphNode* depthStencilState = addNode(NodeType::VkPipelineDepthStencilState, x + dx, row + 640.f);
                depthStencilState->depthStencilDepthTestEnable = withVertexInput;
                depthStencilState->depthStencilDepthWriteEnable = withVertexInput;

                GraphNode* colorBlendState = addNode(NodeType::VkPipelineColorBlendState, x + dx, row + 730.f);
                for (int index = 0; index < colorBlendAttachmentCount; ++index) {
                    GraphNode* colorWriteMask = addNode(NodeType::VkColorWriteMask, x + dx - 280.f, row + 730.f + index * 70.f);
                    GraphNode* blendAttachment = addColorBlendAttachment(x + dx, row + 730.f + index * 70.f);
                    linkOutput(colorWriteMask->id, blendAttachment->id, 0);
                    linkOutput(blendAttachment->id, colorBlendState->id, 0);
                }

                GraphNode* viewportDynamic = addNode(NodeType::VkDynamicState, x + dx - 280.f, row + 820.f);
                GraphNode* scissorDynamic = addNode(NodeType::VkDynamicState, x + dx - 280.f, row + 890.f);
                GraphNode* dynamicState = addNode(NodeType::VkPipelineDynamicState, x + dx, row + 855.f);
                link(viewportDynamic->id, 0, dynamicState->id, 0);
                link(scissorDynamic->id, 1, dynamicState->id, 0);

                GraphNode* pipeline = addNode(NodeType::VkPipeline, x, y);
                pipeline->renderPassIndex = renderPassIndex;

                linkOutput(vertStage->id, pipeline->id, 0);
                linkOutput(fragStage->id, pipeline->id, 0);
                linkOutput(vertexInput->id, pipeline->id, 1);
                linkOutput(inputAssembly->id, pipeline->id, 2);
                linkOutput(viewportState->id, pipeline->id, 3);
                linkOutput(rasterState->id, pipeline->id, 4);
                linkOutput(multisampleState->id, pipeline->id, 5);
                linkOutput(depthStencilState->id, pipeline->id, 6);
                linkOutput(colorBlendState->id, pipeline->id, 7);
                linkOutput(dynamicState->id, pipeline->id, 8);
                if (pipelineLayoutId >= 0) {
                    linkOutput(pipelineLayoutId, pipeline->id, 9);
                }
                linkOutput(renderPassId, pipeline->id, 10);

                return {pipeline->id};
            }

            GraphNode* addVector2(float x, float y, float valueX, float valueY) {
                GraphNode* node = addNode(NodeType::Vector2, x, y);
                node->vector2[0] = valueX;
                node->vector2[1] = valueY;
                return node;
            }

            GraphNode* addVector4(float x, float y, float xVal, float yVal, float zVal, float wVal) {
                GraphNode* node = addNode(NodeType::Vector4, x, y);
                node->vector4[0] = xVal;
                node->vector4[1] = yVal;
                node->vector4[2] = zVal;
                node->vector4[3] = wVal;
                return node;
            }

            GraphNode* addClearValueBundle(float x, float y, int slotCount) {
                GraphNode* clearValue = addNode(NodeType::VkClearValue, x, y);
                for (int index = 1; index < slotCount; ++index) {
                    _document.addClearValueInputSlot(clearValue->id);
                }
                return clearValue;
            }

            GraphNode* addRenderDraw(float x, float y, int renderPassId, int framebufferId, int pipelineId,
                                     GraphNode* clearValue, GraphNode* offset, GraphNode* extent) {
                GraphNode* renderDraw = addNode(NodeType::VkRenderDraw, x, y);
                linkOutput(renderPassId, renderDraw->id, 0);
                linkOutput(framebufferId, renderDraw->id, 1);
                linkOutput(offset->id, renderDraw->id, 2);
                linkOutput(extent->id, renderDraw->id, 3);
                linkOutput(clearValue->id, renderDraw->id, 4);
                linkOutput(pipelineId, renderDraw->id, 5);
                return renderDraw;
            }

            struct RenderPassBuildResult {
                int renderPassId = -1;
            };

            RenderPassBuildResult buildGBufferRenderPass(float x, float y) {
                GraphNode* renderPass = addNode(NodeType::VkRenderPass, x, y);
                for (int index = 0; index < 3; ++index) {
                    _document.addRenderPassAttachmentSlot(renderPass->id);
                }

                GraphNode* albedoAttachment =
                    addAttachmentDescription(x - 420.f, y - 260.f, kVkFormatRgba8, kLayoutColorAttachment);
                GraphNode* normalAttachment =
                    addAttachmentDescription(x - 420.f, y - 120.f, kVkFormatRgba16F, kLayoutColorAttachment);
                GraphNode* materialAttachment =
                    addAttachmentDescription(x - 420.f, y + 20.f, kVkFormatRgba8, kLayoutColorAttachment);
                GraphNode* depthAttachment =
                    addAttachmentDescription(x - 420.f, y + 160.f, kVkFormatD32, kLayoutDepthAttachment);

                linkOutput(albedoAttachment->id, renderPass->id, 0);
                linkOutput(normalAttachment->id, renderPass->id, 1);
                linkOutput(materialAttachment->id, renderPass->id, 2);
                linkOutput(depthAttachment->id, renderPass->id, 3);

                GraphNode* subpass = addNode(NodeType::VkSubpassDescription, x - 420.f, y + 320.f);
                GraphNode* dependency = addNode(NodeType::VkSubpassDependency, x - 420.f, y + 470.f);
                linkOutput(subpass->id, renderPass->id, 4);
                linkOutput(dependency->id, renderPass->id, 5);

                GraphNode* albedoRef = addAttachmentReference(x - 760.f, y + 260.f, 0, kLayoutColorAttachment);
                GraphNode* normalRef = addAttachmentReference(x - 760.f, y + 340.f, 1, kLayoutColorAttachment);
                GraphNode* materialRef = addAttachmentReference(x - 760.f, y + 420.f, 2, kLayoutColorAttachment);
                GraphNode* depthRef = addAttachmentReference(x - 760.f, y + 520.f, 3, kLayoutDepthAttachment);
                linkOutput(albedoRef->id, subpass->id, 0);
                linkOutput(normalRef->id, subpass->id, 0);
                linkOutput(materialRef->id, subpass->id, 0);
                linkOutput(depthRef->id, subpass->id, 1);

                return {renderPass->id};
            }

            RenderPassBuildResult buildLightingRenderPass(float x, float y) {
                GraphNode* renderPass = addNode(NodeType::VkRenderPass, x, y);
                GraphNode* colorAttachment =
                    addAttachmentDescription(x - 420.f, y, kVkFormatRgba8, kLayoutPresent);
                linkOutput(colorAttachment->id, renderPass->id, 0);

                GraphNode* subpass = addNode(NodeType::VkSubpassDescription, x - 420.f, y + 160.f);
                GraphNode* dependency = addNode(NodeType::VkSubpassDependency, x - 420.f, y + 310.f);
                dependency->subpassDependencySrcSubpass = 0;
                dependency->subpassDependencyDstSubpass = 0;
                linkOutput(subpass->id, renderPass->id, 1);
                linkOutput(dependency->id, renderPass->id, 2);

                GraphNode* colorRef = addAttachmentReference(x - 760.f, y + 160.f, 0, kLayoutColorAttachment);
                linkOutput(colorRef->id, subpass->id, 0);
                return {renderPass->id};
            }

            int buildLightingPipelineLayout(float x, float y, int descriptorSetLayoutId) {
                GraphNode* pipelineLayout = addNode(NodeType::VkPipelineLayout, x, y);
                linkOutput(descriptorSetLayoutId, pipelineLayout->id, 0);
                return pipelineLayout->id;
            }

            int buildGBufferDescriptorSetLayout(float x, float y) {
                GraphNode* setLayout = addNode(NodeType::VkDescriptorSetLayout, x, y);

                GraphNode* albedoBinding = addNode(NodeType::VkDescriptorSetLayoutBinding, x - 320.f, y - 80.f);
                albedoBinding->descriptorSetLayoutBindingBinding = 0;
                albedoBinding->descriptorSetLayoutBindingDescriptorType = 1;
                albedoBinding->descriptorSetLayoutBindingStageFlags = 4;

                GraphNode* normalBinding = addNode(NodeType::VkDescriptorSetLayoutBinding, x - 320.f, y + 20.f);
                normalBinding->descriptorSetLayoutBindingBinding = 1;
                normalBinding->descriptorSetLayoutBindingDescriptorType = 1;
                normalBinding->descriptorSetLayoutBindingStageFlags = 4;

                GraphNode* materialBinding = addNode(NodeType::VkDescriptorSetLayoutBinding, x - 320.f, y + 120.f);
                materialBinding->descriptorSetLayoutBindingBinding = 2;
                materialBinding->descriptorSetLayoutBindingDescriptorType = 1;
                materialBinding->descriptorSetLayoutBindingStageFlags = 4;

                linkOutput(albedoBinding->id, setLayout->id, 0);
                linkOutput(normalBinding->id, setLayout->id, 0);
                linkOutput(materialBinding->id, setLayout->id, 0);
                return setLayout->id;
            }

            void linkDescriptorImage(float x, float y, int imageViewId, int samplerId) {
                GraphNode* descriptorImage = addNode(NodeType::VkDescriptorImage, x, y);
                descriptorImage->descriptorImageLayout = kLayoutShaderRead;
                linkOutput(imageViewId, descriptorImage->id, 0);
                linkOutput(samplerId, descriptorImage->id, 1);
            }

            GraphDocument& _document;
        };

    }  // namespace

    void loadDefaultDeferredScene(GraphDocument& document, GridViewState& view) {
        DeferredSceneBuilder builder(document);

        // --- G-Buffer pass ---
        const auto gBufferPass = builder.buildGBufferRenderPass(400.f, 200.f);

        ImageResource albedoImage = builder.addImage2D(980.f, -80.f, kFormatPinRgba8, kUsagePinColorAttachment,
                                                       kAspectPinColor, kVkFormatRgba8);
        ImageResource normalImage = builder.addImage2D(980.f, 120.f, kFormatPinRgba16F, kUsagePinColorAttachment,
                                                       kAspectPinColor, kVkFormatRgba16F);
        ImageResource materialImage = builder.addImage2D(980.f, 320.f, kFormatPinRgba8, kUsagePinColorAttachment,
                                                         kAspectPinColor, kVkFormatRgba8);
        ImageResource depthImage = builder.addImage2D(980.f, 520.f, kFormatPinD32, kUsagePinDepthAttachment,
                                                        kAspectPinDepth, kVkFormatD32);

        GraphNode* gBufferFramebuffer = builder.addNode(NodeType::VkFramebuffer, 1420.f, 220.f);
        gBufferFramebuffer->framebufferWidth = kSceneWidth;
        gBufferFramebuffer->framebufferHeight = kSceneHeight;
        builder.linkOutput(gBufferPass.renderPassId, gBufferFramebuffer->id, 0);
        builder.linkOutput(albedoImage.imageViewId, gBufferFramebuffer->id, 1);
        builder.linkOutput(normalImage.imageViewId, gBufferFramebuffer->id, 1);
        builder.linkOutput(materialImage.imageViewId, gBufferFramebuffer->id, 1);
        builder.linkOutput(depthImage.imageViewId, gBufferFramebuffer->id, 1);

        const auto gBufferPipeline = builder.buildGraphicsPipeline(
            1880.f, 220.f, gBufferPass.renderPassId, 0, "shaders/deferred/gbuffer.vert.spv",
            "shaders/deferred/gbuffer.frag.spv", 3, true, -1);

        GraphNode* gBufferClear = builder.addClearValueBundle(1420.f, 620.f, 4);
        builder.linkOutput(builder.addVector4(1100.f, 620.f, 0.f, 0.f, 0.f, 1.f)->id, gBufferClear->id, 0);
        builder.linkOutput(builder.addVector4(1100.f, 700.f, 0.5f, 0.5f, 1.f, 0.f)->id, gBufferClear->id, 1);
        builder.linkOutput(builder.addVector4(1100.f, 780.f, 0.8f, 0.2f, 0.f, 1.f)->id, gBufferClear->id, 2);
        builder.linkOutput(builder.addVector4(1100.f, 860.f, 1.f, 0.f, 0.f, 0.f)->id, gBufferClear->id, 3);

        GraphNode* drawOffset = builder.addVector2(1420.f, 760.f, 0.f, 0.f);
        GraphNode* drawExtent = builder.addVector2(1420.f, 840.f, static_cast<float>(kSceneWidth),
                                                   static_cast<float>(kSceneHeight));
        builder.addRenderDraw(1880.f, 620.f, gBufferPass.renderPassId, gBufferFramebuffer->id, gBufferPipeline.pipelineId,
                              gBufferClear, drawOffset, drawExtent);

        // --- Lighting pass ---
        const auto lightingPass = builder.buildLightingRenderPass(2480.f, 220.f);

        ImageResource lightingTarget = builder.addImage2D(2860.f, 120.f, kFormatPinRgba8, kUsagePinColorAttachment,
                                                          kAspectPinColor, kVkFormatRgba8);
        GraphNode* lightingFramebuffer = builder.addNode(NodeType::VkFramebuffer, 3240.f, 220.f);
        lightingFramebuffer->framebufferWidth = kSceneWidth;
        lightingFramebuffer->framebufferHeight = kSceneHeight;
        builder.linkOutput(lightingPass.renderPassId, lightingFramebuffer->id, 0);
        builder.linkOutput(lightingTarget.imageViewId, lightingFramebuffer->id, 1);

        GraphNode* linearSampler = builder.addNode(NodeType::VkSampler, 2480.f, 520.f);
        linearSampler->samplerMagFilter = 1;
        linearSampler->samplerMinFilter = 1;
        linearSampler->samplerAddressModeU = 2;
        linearSampler->samplerAddressModeV = 2;
        linearSampler->samplerAddressModeW = 2;

        const int gBufferDescriptorLayoutId = builder.buildGBufferDescriptorSetLayout(2480.f, 680.f);
        builder.linkDescriptorImage(2140.f, 520.f, albedoImage.imageViewId, linearSampler->id);
        builder.linkDescriptorImage(2140.f, 620.f, normalImage.imageViewId, linearSampler->id);
        builder.linkDescriptorImage(2140.f, 720.f, materialImage.imageViewId, linearSampler->id);

        const int lightingPipelineLayoutId =
            builder.buildLightingPipelineLayout(2860.f, 680.f, gBufferDescriptorLayoutId);
        const auto lightingPipeline = builder.buildGraphicsPipeline(
            3700.f, 220.f, lightingPass.renderPassId, 0, "shaders/deferred/lighting.vert.spv",
            "shaders/deferred/lighting.frag.spv", 1, false, lightingPipelineLayoutId);

        GraphNode* lightingClear = builder.addClearValueBundle(3240.f, 620.f, 1);
        builder.linkOutput(builder.addVector4(3020.f, 620.f, 0.f, 0.f, 0.f, 1.f)->id, lightingClear->id, 0);

        GraphNode* lightingOffset = builder.addVector2(3240.f, 760.f, 0.f, 0.f);
        GraphNode* lightingExtent = builder.addVector2(3240.f, 840.f, static_cast<float>(kSceneWidth),
                                                       static_cast<float>(kSceneHeight));
        builder.addRenderDraw(3700.f, 620.f, lightingPass.renderPassId, lightingFramebuffer->id,
                              lightingPipeline.pipelineId, lightingClear, lightingOffset, lightingExtent);

        view.zoom = 0.42f;
        view.pan = ImVec2(-900.f, -120.f);
    }

}  // namespace mat::demo
