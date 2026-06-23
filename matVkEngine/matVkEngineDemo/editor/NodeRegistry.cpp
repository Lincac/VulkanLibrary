#include "editor/NodeRegistry.h"

#include <imgui.h>

#include <string>

namespace mat::demo {

    namespace {
        constexpr int kPinsPerNode = 32;
        constexpr float kPropFieldWidth = 128.f;
    }

    int NodeRegistry::encodeAttr(int nodeId, int pinIndex, bool isOutput) {
        return nodeId * (kPinsPerNode * 2) + pinIndex * 2 + (isOutput ? 1 : 0);
    }

    int NodeRegistry::encodeStaticAttr(int nodeId) {
        return nodeId * (kPinsPerNode * 2) + kPinsPerNode * 2 - 1;
    }

    const char* NodeRegistry::displayName(NodeType type) {
        switch (type) {
            case NodeType::RenderConfig:
                return "Render Config";
            case NodeType::RenderPass:
                return "Render Pass";
            case NodeType::Subpass:
                return "Subpass";
            case NodeType::GraphicsPipeline:
                return "Graphics Pipeline";
            case NodeType::Texture:
                return "Texture";
            case NodeType::Shader:
                return "Shader";
            case NodeType::Vertex:
                return "Vertex Buffer";
            case NodeType::DrawPass:
                return "Draw Pass";
            case NodeType::FullscreenPass:
                return "Fullscreen Pass";
            case NodeType::UiPass:
                return "UI Pass";
            case NodeType::Material:
                return "Material";
            case NodeType::Entity:
                return "Entity";
            case NodeType::Camera:
                return "Camera";
            case NodeType::Light:
                return "Light";
            case NodeType::Scene:
                return "Scene";
            case NodeType::OutputPng:
                return "Output PNG";
        }
        return "Node";
    }

    namespace {

        char* propBuffer(GraphNode& node, const char* key, char* buffer, size_t size) {
            const auto iter = node.props.find(key);
            if (iter != node.props.end()) {
                strncpy_s(buffer, size, iter->second.c_str(), _TRUNCATE);
            } else {
                buffer[0] = '\0';
            }
            return buffer;
        }

        void saveProp(GraphNode& node, const char* key, const char* buffer) {
            node.props[key] = buffer;
        }

        void drawTextProp(GraphNode& node, const char* label, const char* key) {
            char buffer[256]{};
            propBuffer(node, key, buffer, sizeof(buffer));

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine(72.f);
            ImGui::PushItemWidth(kPropFieldWidth);
            const std::string inputId = std::string("##") + key;
            if (ImGui::InputText(inputId.c_str(), buffer, sizeof(buffer))) {
                saveProp(node, key, buffer);
            }
            ImGui::PopItemWidth();
        }

        void drawComboProp(GraphNode& node, const char* label, const char* key,
                           const char* const* items, int itemCount) {
            int current = 0;
            const auto iter = node.props.find(key);
            if (iter != node.props.end()) {
                for (int i = 0; i < itemCount; ++i) {
                    if (iter->second == items[i]) {
                        current = i;
                        break;
                    }
                }
            } else if (itemCount > 0) {
                node.props[key] = items[0];
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine(72.f);
            ImGui::PushItemWidth(kPropFieldWidth);
            const std::string comboId = std::string("##") + key;
            if (ImGui::Combo(comboId.c_str(), &current, items, itemCount)) {
                node.props[key] = items[current];
            }
            ImGui::PopItemWidth();
        }

        template <size_t N>
        void drawComboProp(GraphNode& node, const char* label, const char* key,
                           const char* const (&items)[N]) {
            drawComboProp(node, label, key, items, static_cast<int>(N));
        }

    }  // namespace

    void NodeRegistry::drawNodeBody(GraphNode& node) {
        static const char* kLoadOps[] = {"Clear", "Load", "DontCare"};
        static const char* kStoreOps[] = {"Store", "DontCare"};
        static const char* kTopologies[] = {"PointList", "LineList", "TriangleList", "TriangleStrip"};
        static const char* kCullModes[] = {"None", "Front", "Back"};
        static const char* kPolygonModes[] = {"Fill", "Line", "Point"};
        static const char* kTextureUsages[] = {"ColorAttachment", "DepthStencilAttachment", "Sampled", "Storage"};
        static const char* kShaderStages[] = {"Vertex", "Fragment", "Geometry", "Compute"};
        static const char* kSubpassPurposes[] = {"Generic",     "GBuffer",      "Lighting",
                                                 "ToneMapping", "AntiAliasing", "UIText"};

        switch (node.type) {
            case NodeType::RenderConfig:
                drawTextProp(node, "width", "width");
                drawTextProp(node, "height", "height");
                drawTextProp(node, "output", "outputPath");
                break;
            case NodeType::RenderPass:
                drawTextProp(node, "name", "name");
                drawTextProp(node, "pass#", "passIndex");
                drawComboProp(node, "loadOp", "loadOp", kLoadOps);
                drawComboProp(node, "storeOp", "storeOp", kStoreOps);
                drawTextProp(node, "clear", "clearColor");
                break;
            case NodeType::Subpass:
                drawTextProp(node, "name", "name");
                drawTextProp(node, "index", "index");
                drawComboProp(node, "purpose", "purpose", kSubpassPurposes);
                break;
            case NodeType::GraphicsPipeline:
                drawTextProp(node, "name", "name");
                drawComboProp(node, "topology", "topology", kTopologies);
                drawComboProp(node, "cull", "cullMode", kCullModes);
                drawComboProp(node, "polygon", "polygonMode", kPolygonModes);
                break;
            case NodeType::Texture:
                drawTextProp(node, "name", "name");
                drawTextProp(node, "format", "format");
                drawTextProp(node, "width", "width");
                drawTextProp(node, "height", "height");
                drawComboProp(node, "usage", "usage", kTextureUsages);
                drawTextProp(node, "file", "filePath");
                break;
            case NodeType::Shader:
                drawTextProp(node, "name", "name");
                drawComboProp(node, "stage", "stage", kShaderStages);
                drawTextProp(node, "spv", "spv");
                break;
            case NodeType::Vertex:
                drawTextProp(node, "mesh", "meshPath");
                drawTextProp(node, "layout", "layout");
                break;
            case NodeType::DrawPass:
            case NodeType::FullscreenPass:
            case NodeType::UiPass:
            case NodeType::Scene:
                drawTextProp(node, "name", "name");
                break;
            case NodeType::Material:
                drawTextProp(node, "name", "name");
                drawTextProp(node, "vert", "vertSpv");
                drawTextProp(node, "frag", "fragSpv");
                break;
            case NodeType::Entity:
                drawTextProp(node, "name", "name");
                break;
            case NodeType::Camera:
                drawTextProp(node, "eye", "eye");
                drawTextProp(node, "target", "target");
                drawTextProp(node, "up", "up");
                drawTextProp(node, "fov", "fov");
                drawTextProp(node, "near", "near");
                drawTextProp(node, "far", "far");
                break;
            case NodeType::Light:
                drawTextProp(node, "dir", "direction");
                drawTextProp(node, "intensity", "intensity");
                break;
            case NodeType::OutputPng:
                drawTextProp(node, "output", "outputPath");
                break;
        }
    }

}  // namespace mat::demo
