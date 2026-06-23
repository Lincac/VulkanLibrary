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
            case NodeType::DrawPass:
                return "Draw Pass";
            case NodeType::FullscreenPass:
                return "Fullscreen Pass";
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

    }  // namespace

    void NodeRegistry::drawNodeBody(GraphNode& node) {
        switch (node.type) {
            case NodeType::RenderConfig:
                drawTextProp(node, "width", "width");
                drawTextProp(node, "height", "height");
                drawTextProp(node, "output", "outputPath");
                break;
            case NodeType::DrawPass:
            case NodeType::FullscreenPass:
            case NodeType::Scene:
                drawTextProp(node, "name", "name");
                break;
            case NodeType::Material:
                drawTextProp(node, "name", "name");
                drawTextProp(node, "vert", "vertSpv");
                drawTextProp(node, "frag", "fragSpv");
                break;
            case NodeType::Entity:
                drawTextProp(node, "mesh", "meshPath");
                break;
            case NodeType::Camera:
                drawTextProp(node, "eye", "eye");
                drawTextProp(node, "target", "target");
                drawTextProp(node, "fov", "fov");
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
