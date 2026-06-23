#include "graph/GraphTemplates.h"

#include "editor/NodeRegistry.h"

#include <imnodes.h>

namespace mat::demo {

    namespace {

        void setProp(GraphNode* node, const char* key, const char* value) {
            if (node != nullptr) {
                node->props[key] = value;
            }
        }

        void connect(GraphDocument& doc, int fromNode, int fromPin, int toNode, int toPin) {
            std::string error;
            doc.tryConnect(NodeRegistry::encodeAttr(fromNode, fromPin, true),
                           NodeRegistry::encodeAttr(toNode, toPin, false), error);
        }

        void placeNode(int nodeId, float x, float y) {
            ImNodes::SetNodeGridSpacePos(nodeId, ImVec2(x, y));
        }

        int addShader(GraphDocument& doc, float x, float y, const char* name, const char* stage,
                      const char* spv) {
            const int id = doc.addNode(NodeType::Shader, x, y);
            if (GraphNode* node = doc.findNode(id)) {
                setProp(node, "name", name);
                setProp(node, "stage", stage);
                setProp(node, "spv", spv);
            }
            return id;
        }

        int addTexture(GraphDocument& doc, float x, float y, const char* name, const char* format,
                       const char* usage) {
            const int id = doc.addNode(NodeType::Texture, x, y);
            if (GraphNode* node = doc.findNode(id)) {
                setProp(node, "name", name);
                setProp(node, "format", format);
                setProp(node, "usage", usage);
            }
            return id;
        }

        int addSubpass(GraphDocument& doc, float x, float y, const char* name, const char* index,
                       const char* purpose) {
            const int id = doc.addNode(NodeType::Subpass, x, y);
            if (GraphNode* node = doc.findNode(id)) {
                setProp(node, "name", name);
                setProp(node, "index", index);
                setProp(node, "purpose", purpose);
            }
            return id;
        }

        int addRenderPass(GraphDocument& doc, float x, float y, const char* name, const char* passIndex) {
            const int id = doc.addNode(NodeType::RenderPass, x, y);
            if (GraphNode* node = doc.findNode(id)) {
                setProp(node, "name", name);
                setProp(node, "passIndex", passIndex);
            }
            return id;
        }

        int addPipeline(GraphDocument& doc, float x, float y, const char* name) {
            const int id = doc.addNode(NodeType::GraphicsPipeline, x, y);
            if (GraphNode* node = doc.findNode(id)) {
                setProp(node, "name", name);
            }
            return id;
        }

        void wirePipeline(GraphDocument& doc, int subpass, int vertShader, int fragShader, int vertex,
                          int pipeline) {
            connect(doc, subpass, 0, pipeline, 0);
            connect(doc, vertShader, 0, pipeline, 1);
            connect(doc, fragShader, 0, pipeline, 2);
            if (vertex > 0) {
                connect(doc, vertex, 0, pipeline, 3);
            }
        }

        void wireRenderPass(GraphDocument& doc, int renderPass, const int* subpasses, int subpassCount,
                            const int* colors, int colorCount, int depthTex) {
            for (int i = 0; i < subpassCount; ++i) {
                connect(doc, subpasses[i], 0, renderPass, 0);
            }
            for (int i = 0; i < colorCount; ++i) {
                connect(doc, colors[i], 0, renderPass, 1);
            }
            if (depthTex > 0) {
                connect(doc, depthTex, 0, renderPass, 2);
            }
        }

        void wireStage(GraphDocument& doc, int stage, int renderPass, int subpass, int pipeline,
                       int scene = -1) {
            connect(doc, renderPass, 0, stage, 1);
            connect(doc, subpass, 0, stage, 2);
            connect(doc, pipeline, 0, stage, 3);
            if (scene > 0) {
                connect(doc, scene, 0, stage, 4);
            }
        }

    }  // namespace

    void GraphTemplates::buildForward(GraphDocument& document) {
        document.clear();

        const int renderConfig = document.addNode(NodeType::RenderConfig, 40.f, 40.f);
        const int colorTex = addTexture(document, 40.f, 200.f, "color0", "RGBA8_UNORM", "ColorAttachment");
        const int depthTex =
            addTexture(document, 40.f, 380.f, "depth", "D32_SFLOAT", "DepthStencilAttachment");

        const int vertShader = addShader(document, 280.f, 200.f, "phong_vert", "Vertex",
                                         "shaders/compiled/forward/phong.vert.spv");
        const int fragShader = addShader(document, 280.f, 340.f, "phong_frag", "Fragment",
                                         "shaders/compiled/forward/phong.frag.spv");
        const int vertex = document.addNode(NodeType::Vertex, 280.f, 480.f);
        const int subpass = addSubpass(document, 520.f, 480.f, "main", "0", "Generic");
        const int pipeline = addPipeline(document, 520.f, 280.f, "phong");
        const int renderPass = addRenderPass(document, 520.f, 80.f, "forward", "1");
        const int camera = document.addNode(NodeType::Camera, 760.f, 480.f);
        const int entity = document.addNode(NodeType::Entity, 760.f, 340.f);
        setProp(document.findNode(entity), "name", "bunny");
        const int scene = document.addNode(NodeType::Scene, 760.f, 200.f);
        const int drawPass = document.addNode(NodeType::DrawPass, 1000.f, 80.f);
        const int output = document.addNode(NodeType::OutputPng, 1240.f, 80.f);

        wirePipeline(document, subpass, vertShader, fragShader, vertex, pipeline);
        wireRenderPass(document, renderPass, &subpass, 1, &colorTex, 1, depthTex);
        wireStage(document, drawPass, renderPass, subpass, pipeline, scene);
        connect(document, vertex, 0, entity, 0);
        connect(document, pipeline, 0, entity, 1);
        connect(document, entity, 0, scene, 0);
        connect(document, camera, 0, scene, 1);
        connect(document, drawPass, 0, output, 0);
        connect(document, drawPass, 1, output, 1);

        placeNode(renderConfig, 40.f, 40.f);
        placeNode(colorTex, 40.f, 200.f);
        placeNode(depthTex, 40.f, 380.f);
        placeNode(vertShader, 280.f, 200.f);
        placeNode(fragShader, 280.f, 340.f);
        placeNode(vertex, 280.f, 480.f);
        placeNode(subpass, 520.f, 480.f);
        placeNode(pipeline, 520.f, 280.f);
        placeNode(renderPass, 520.f, 80.f);
        placeNode(camera, 760.f, 480.f);
        placeNode(entity, 760.f, 340.f);
        placeNode(scene, 760.f, 200.f);
        placeNode(drawPass, 1000.f, 80.f);
        placeNode(output, 1240.f, 80.f);
    }

    void GraphTemplates::buildDeferredMultiPass(GraphDocument& document) {
        document.clear();

        document.addNode(NodeType::RenderConfig, 40.f, 40.f);

        // Pass 1: deferred (GBuffer + Lighting in one VkRenderPass)
        const int albedoTex = addTexture(document, 40.f, 160.f, "albedo", "RGBA8_UNORM", "ColorAttachment");
        const int normalTex = addTexture(document, 40.f, 260.f, "normal", "RGBA16_SNORM", "ColorAttachment");
        const int materialTex =
            addTexture(document, 40.f, 360.f, "material", "RGBA8_UNORM", "ColorAttachment");
        const int depthTex =
            addTexture(document, 40.f, 460.f, "depth", "D32_SFLOAT", "DepthStencilAttachment");

        const int rpDeferred = addRenderPass(document, 280.f, 80.f, "deferred", "1");
        const int spGBuffer = addSubpass(document, 240.f, 560.f, "gbuffer", "0", "GBuffer");
        const int spLighting = addSubpass(document, 360.f, 560.f, "lighting", "1", "Lighting");
        const int subpassesPass1[] = {spGBuffer, spLighting};
        const int colorsPass1[] = {albedoTex, normalTex, materialTex};
        wireRenderPass(document, rpDeferred, subpassesPass1, 2, colorsPass1, 3, depthTex);

        const int gbufVert =
            addShader(document, 520.f, 520.f, "gbuffer_vert", "Vertex",
                      "shaders/compiled/deferred/gbuffer.vert.spv");
        const int gbufFrag =
            addShader(document, 520.f, 620.f, "gbuffer_frag", "Fragment",
                      "shaders/compiled/deferred/gbuffer.frag.spv");
        const int vertex = document.addNode(NodeType::Vertex, 520.f, 720.f);
        const int gbufPipeline = addPipeline(document, 680.f, 600.f, "gbuffer");
        wirePipeline(document, spGBuffer, gbufVert, gbufFrag, vertex, gbufPipeline);

        const int lightVert =
            addShader(document, 840.f, 520.f, "fullscreen_vert", "Vertex",
                      "shaders/compiled/deferred/fullscreen.vert.spv");
        const int lightFrag =
            addShader(document, 840.f, 620.f, "deferred_light_frag", "Fragment",
                      "shaders/compiled/deferred/lighting.frag.spv");
        const int lightPipeline = addPipeline(document, 1000.f, 600.f, "deferred_light");
        wirePipeline(document, spLighting, lightVert, lightFrag, 0, lightPipeline);

        const int camera = document.addNode(NodeType::Camera, 680.f, 820.f);
        const int light = document.addNode(NodeType::Light, 800.f, 820.f);
        const int entity = document.addNode(NodeType::Entity, 920.f, 820.f);
        setProp(document.findNode(entity), "name", "bunny");
        const int scene = document.addNode(NodeType::Scene, 800.f, 920.f);
        connect(document, vertex, 0, entity, 0);
        connect(document, gbufPipeline, 0, entity, 1);
        connect(document, entity, 0, scene, 0);
        connect(document, camera, 0, scene, 1);
        connect(document, light, 0, scene, 2);

        const int stageGBuffer = document.addNode(NodeType::DrawPass, 1240.f, 80.f);
        setProp(document.findNode(stageGBuffer), "name", "Pass1: GBuffer");
        wireStage(document, stageGBuffer, rpDeferred, spGBuffer, gbufPipeline, scene);

        const int stageLighting = document.addNode(NodeType::FullscreenPass, 1480.f, 80.f);
        setProp(document.findNode(stageLighting), "name", "Pass1: Lighting");
        wireStage(document, stageLighting, rpDeferred, spLighting, lightPipeline);
        connect(document, albedoTex, 0, stageLighting, 4);
        connect(document, normalTex, 0, stageLighting, 5);
        connect(document, depthTex, 0, stageLighting, 6);

        // Pass 2: tone mapping
        const int hdrTex = addTexture(document, 40.f, 580.f, "hdr", "RGBA16_SFLOAT", "ColorAttachment");
        const int rpTone = addRenderPass(document, 280.f, 200.f, "tone_mapping", "2");
        const int spTone = addSubpass(document, 280.f, 680.f, "tone", "0", "ToneMapping");
        wireRenderPass(document, rpTone, &spTone, 1, &hdrTex, 1, 0);

        const int toneVert =
            addShader(document, 520.f, 860.f, "tone_vert", "Vertex",
                      "shaders/compiled/post/tone_mapping.vert.spv");
        const int toneFrag =
            addShader(document, 520.f, 960.f, "tone_frag", "Fragment",
                      "shaders/compiled/post/tone_mapping.frag.spv");
        const int tonePipeline = addPipeline(document, 680.f, 900.f, "tone_mapping");
        wirePipeline(document, spTone, toneVert, toneFrag, 0, tonePipeline);

        const int stageTone = document.addNode(NodeType::FullscreenPass, 1720.f, 80.f);
        setProp(document.findNode(stageTone), "name", "Pass2: Tone Mapping");
        wireStage(document, stageTone, rpTone, spTone, tonePipeline);
        connect(document, stageLighting, 1, stageTone, 4);

        // Pass 3: anti-aliasing
        const int ldrTex = addTexture(document, 40.f, 700.f, "ldr", "RGBA8_UNORM", "ColorAttachment");
        const int rpAa = addRenderPass(document, 280.f, 320.f, "anti_aliasing", "3");
        const int spAa = addSubpass(document, 280.f, 800.f, "aa", "0", "AntiAliasing");
        wireRenderPass(document, rpAa, &spAa, 1, &ldrTex, 1, 0);

        const int aaVert = addShader(document, 840.f, 860.f, "aa_vert", "Vertex",
                                     "shaders/compiled/post/aa.vert.spv");
        const int aaFrag = addShader(document, 840.f, 960.f, "aa_frag", "Fragment",
                                     "shaders/compiled/post/aa.frag.spv");
        const int aaPipeline = addPipeline(document, 1000.f, 900.f, "fxaa");
        wirePipeline(document, spAa, aaVert, aaFrag, 0, aaPipeline);

        const int stageAa = document.addNode(NodeType::FullscreenPass, 1960.f, 80.f);
        setProp(document.findNode(stageAa), "name", "Pass3: Anti-Aliasing");
        wireStage(document, stageAa, rpAa, spAa, aaPipeline);
        connect(document, stageTone, 1, stageAa, 4);

        // Pass 4: UI text
        const int uiTex = addTexture(document, 40.f, 820.f, "ui", "RGBA8_UNORM", "ColorAttachment");
        const int rpUi = addRenderPass(document, 280.f, 440.f, "ui", "4");
        const int spText = addSubpass(document, 280.f, 920.f, "text", "0", "UIText");
        wireRenderPass(document, rpUi, &spText, 1, &uiTex, 1, 0);

        const int textVert = addShader(document, 1160.f, 860.f, "text_vert", "Vertex",
                                       "shaders/compiled/ui/text.vert.spv");
        const int textFrag = addShader(document, 1160.f, 960.f, "text_frag", "Fragment",
                                       "shaders/compiled/ui/text.frag.spv");
        const int textPipeline = addPipeline(document, 1320.f, 900.f, "ui_text");
        wirePipeline(document, spText, textVert, textFrag, 0, textPipeline);

        const int stageUi = document.addNode(NodeType::UiPass, 2200.f, 80.f);
        setProp(document.findNode(stageUi), "name", "Pass4: UI Text");
        wireStage(document, stageUi, rpUi, spText, textPipeline);

        const int output = document.addNode(NodeType::OutputPng, 2440.f, 80.f);

        connect(document, stageGBuffer, 0, stageLighting, 0);
        connect(document, stageLighting, 0, stageTone, 0);
        connect(document, stageTone, 0, stageAa, 0);
        connect(document, stageAa, 0, stageUi, 0);
        connect(document, stageUi, 0, output, 0);
        connect(document, stageUi, 1, output, 1);

        placeNode(rpDeferred, 280.f, 80.f);
        placeNode(rpTone, 280.f, 200.f);
        placeNode(rpAa, 280.f, 320.f);
        placeNode(rpUi, 280.f, 440.f);
        placeNode(stageGBuffer, 1240.f, 80.f);
        placeNode(stageLighting, 1480.f, 80.f);
        placeNode(stageTone, 1720.f, 80.f);
        placeNode(stageAa, 1960.f, 80.f);
        placeNode(stageUi, 2200.f, 80.f);
        placeNode(output, 2440.f, 80.f);
    }

}  // namespace mat::demo
