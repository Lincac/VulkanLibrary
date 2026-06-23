#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mat::demo {

    enum class NodeType {
        RenderConfig,
        DrawPass,
        FullscreenPass,
        Material,
        Entity,
        Camera,
        Light,
        Scene,
        OutputPng,
    };

    enum class PinKind {
        Exec,
        Color,
        Depth,
        Resource,
        Material,
        Entity,
        Camera,
        Light,
        Scene,
        RenderConfig,
    };

    struct PinId {
        int nodeId = 0;
        int pinIndex = 0;
        bool isOutput = false;

        bool operator==(const PinId& other) const {
            return nodeId == other.nodeId && pinIndex == other.pinIndex && isOutput == other.isOutput;
        }
    };

    struct PinDesc {
        PinKind kind = PinKind::Exec;
        std::string label;
        bool allowMultiple = false;
    };

    struct GraphNode {
        int id = 0;
        NodeType type = NodeType::DrawPass;
        float posX = 0.f;
        float posY = 0.f;
        std::unordered_map<std::string, std::string> props;
        std::vector<PinDesc> inputs;
        std::vector<PinDesc> outputs;
    };

    struct GraphLink {
        int id = 0;
        int startAttr = 0;
        int endAttr = 0;
    };

    const char* nodeTypeName(NodeType type);
    NodeType nodeTypeFromName(const std::string& name);

    void applyDefaultPins(GraphNode& node);

}  // namespace mat::demo

namespace std {
    template <>
    struct hash<mat::demo::PinId> {
        size_t operator()(const mat::demo::PinId& pin) const {
            return hash<int>()(pin.nodeId) ^ (hash<int>()(pin.pinIndex) << 1) ^
                   (hash<bool>()(pin.isOutput) << 2);
        }
    };
}  // namespace std
