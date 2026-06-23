#include "graph/GraphTypes.h"

namespace mat::demo {

    const char* nodeTypeName(NodeType type) {
        switch (type) {
            case NodeType::RenderConfig:
                return "RenderConfig";
            case NodeType::DrawPass:
                return "DrawPass";
            case NodeType::FullscreenPass:
                return "FullscreenPass";
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
                return "OutputPng";
        }
        return "Unknown";
    }

    NodeType nodeTypeFromName(const std::string& name) {
        if (name == "RenderConfig") return NodeType::RenderConfig;
        if (name == "DrawPass") return NodeType::DrawPass;
        if (name == "FullscreenPass") return NodeType::FullscreenPass;
        if (name == "Material") return NodeType::Material;
        if (name == "Entity") return NodeType::Entity;
        if (name == "Camera") return NodeType::Camera;
        if (name == "Light") return NodeType::Light;
        if (name == "Scene") return NodeType::Scene;
        if (name == "OutputPng") return NodeType::OutputPng;
        return NodeType::DrawPass;
    }

    void applyDefaultPins(GraphNode& node) {
        node.inputs.clear();
        node.outputs.clear();

        switch (node.type) {
            case NodeType::RenderConfig:
                node.outputs.push_back({PinKind::RenderConfig, "render", false});
                break;
            case NodeType::DrawPass:
                node.inputs.push_back({PinKind::Exec, "exec", false});
                node.inputs.push_back({PinKind::Material, "material", true});
                node.outputs.push_back({PinKind::Exec, "exec", false});
                node.outputs.push_back({PinKind::Color, "color", false});
                node.outputs.push_back({PinKind::Depth, "depth", false});
                break;
            case NodeType::FullscreenPass:
                node.inputs.push_back({PinKind::Exec, "exec", false});
                node.inputs.push_back({PinKind::Resource, "in0", true});
                node.inputs.push_back({PinKind::Resource, "in1", true});
                node.inputs.push_back({PinKind::Resource, "in2", true});
                node.outputs.push_back({PinKind::Exec, "exec", false});
                node.outputs.push_back({PinKind::Color, "hdr", false});
                break;
            case NodeType::Material:
                node.outputs.push_back({PinKind::Material, "material", false});
                break;
            case NodeType::Entity:
                node.inputs.push_back({PinKind::Material, "material", false});
                node.outputs.push_back({PinKind::Entity, "entity", false});
                break;
            case NodeType::Camera:
                node.outputs.push_back({PinKind::Camera, "camera", false});
                break;
            case NodeType::Light:
                node.outputs.push_back({PinKind::Light, "light", false});
                break;
            case NodeType::Scene:
                node.inputs.push_back({PinKind::Entity, "entity", true});
                node.inputs.push_back({PinKind::Camera, "camera", false});
                node.inputs.push_back({PinKind::Light, "light", true});
                node.outputs.push_back({PinKind::Scene, "scene", false});
                break;
            case NodeType::OutputPng:
                node.inputs.push_back({PinKind::Exec, "exec", false});
                node.inputs.push_back({PinKind::Color, "color", false});
                break;
        }
    }

}  // namespace mat::demo
