#include "graph/GraphDocument.h"

#include "graph/GraphValidator.h"

#include <algorithm>

namespace mat::demo {

    int GraphDocument::encodeAttr(int nodeId, int pinIndex, bool isOutput) {
        return nodeId * (kPinsPerNode * 2) + pinIndex * 2 + (isOutput ? 1 : 0);
    }

    int GraphDocument::attrFromPin(const PinId& pin) const {
        return encodeAttr(pin.nodeId, pin.pinIndex, pin.isOutput);
    }

    std::optional<PinId> GraphDocument::pinFromAttr(int attrId) const {
        if (attrId <= 0) {
            return std::nullopt;
        }

        const int slot = attrId % (kPinsPerNode * 2);
        PinId pin{};
        pin.isOutput = (slot % 2) == 1;
        pin.pinIndex = slot / 2;
        pin.nodeId = attrId / (kPinsPerNode * 2);
        return pin;
    }

    int GraphDocument::addNode(NodeType type, float posX, float posY) {
        GraphNode node{};
        node.id = _nextNodeId++;
        node.type = type;
        node.posX = posX;
        node.posY = posY;
        applyDefaultPins(node);

        switch (type) {
            case NodeType::RenderConfig:
                node.props["width"] = "1280";
                node.props["height"] = "720";
                node.props["outputPath"] = "output/render.png";
                break;
            case NodeType::RenderPass:
                node.props["name"] = "forward";
                node.props["passIndex"] = "1";
                node.props["loadOp"] = "Clear";
                node.props["storeOp"] = "Store";
                node.props["clearColor"] = "0.1,0.1,0.15,1";
                break;
            case NodeType::Subpass:
                node.props["name"] = "main";
                node.props["index"] = "0";
                node.props["purpose"] = "Generic";
                break;
            case NodeType::GraphicsPipeline:
                node.props["name"] = "phong";
                node.props["topology"] = "TriangleList";
                node.props["cullMode"] = "Back";
                node.props["polygonMode"] = "Fill";
                break;
            case NodeType::Texture:
                node.props["name"] = "color0";
                node.props["format"] = "RGBA8_UNORM";
                node.props["width"] = "1280";
                node.props["height"] = "720";
                node.props["usage"] = "ColorAttachment";
                break;
            case NodeType::Shader:
                node.props["name"] = "shader";
                node.props["stage"] = "Vertex";
                node.props["spv"] = "shaders/compiled/forward/phong.vert.spv";
                break;
            case NodeType::Vertex:
                node.props["meshPath"] = "models/bunny.obj";
                node.props["layout"] = "Position,Normal";
                break;
            case NodeType::DrawPass:
                node.props["name"] = "forward";
                break;
            case NodeType::FullscreenPass:
                node.props["name"] = "lighting";
                break;
            case NodeType::UiPass:
                node.props["name"] = "ui";
                break;
            case NodeType::Material:
                node.props["name"] = "material";
                node.props["vertSpv"] = "shaders/compiled/forward/phong.vert.spv";
                node.props["fragSpv"] = "shaders/compiled/forward/phong.frag.spv";
                break;
            case NodeType::Entity:
                node.props["name"] = "entity";
                break;
            case NodeType::Camera:
                node.props["eye"] = "0,1,3";
                node.props["target"] = "0,0,0";
                node.props["up"] = "0,1,0";
                node.props["fov"] = "45";
                node.props["near"] = "0.1";
                node.props["far"] = "100";
                break;
            case NodeType::Light:
                node.props["direction"] = "0.3,-1,0.2";
                node.props["intensity"] = "1";
                break;
            case NodeType::Scene:
                node.props["name"] = "scene";
                break;
            case NodeType::OutputPng:
                node.props["outputPath"] = "output/render.png";
                break;
        }

        const int newId = node.id;
        _nodes.push_back(std::move(node));
        return newId;
    }

    GraphNode* GraphDocument::findNode(int nodeId) {
        auto iter = std::find_if(_nodes.begin(), _nodes.end(),
                                 [nodeId](const GraphNode& node) { return node.id == nodeId; });
        return iter != _nodes.end() ? &(*iter) : nullptr;
    }

    const GraphNode* GraphDocument::findNode(int nodeId) const {
        return const_cast<GraphDocument*>(this)->findNode(nodeId);
    }

    bool GraphDocument::removeNode(int nodeId) {
        const auto nodeIter = std::find_if(_nodes.begin(), _nodes.end(),
                                           [nodeId](const GraphNode& node) { return node.id == nodeId; });
        if (nodeIter == _nodes.end()) {
            return false;
        }

        _links.erase(std::remove_if(_links.begin(), _links.end(),
                                     [nodeId, this](const GraphLink& link) {
                                         const auto from = pinFromAttr(link.startAttr);
                                         const auto to = pinFromAttr(link.endAttr);
                                         return (from && from->nodeId == nodeId) || (to && to->nodeId == nodeId);
                                     }),
                     _links.end());
        _nodes.erase(nodeIter);
        return true;
    }

    bool GraphDocument::tryConnect(int startAttr, int endAttr, std::string& error) {
        const auto fromPin = pinFromAttr(startAttr);
        const auto toPin = pinFromAttr(endAttr);
        if (!fromPin || !toPin) {
            error = "Invalid pin id";
            return false;
        }

        if (!fromPin->isOutput || toPin->isOutput) {
            error = "Links must connect output to input";
            return false;
        }

        if (!GraphValidator::canConnect(*this, *fromPin, *toPin, error)) {
            return false;
        }

        const GraphLink* conflict = nullptr;
        for (const GraphLink& link : _links) {
            if (link.endAttr != endAttr) {
                continue;
            }

            const auto existingTo = pinFromAttr(link.endAttr);
            if (!existingTo) {
                continue;
            }

            const GraphNode* targetNode = findNode(existingTo->nodeId);
            if (!targetNode || existingTo->pinIndex >= static_cast<int>(targetNode->inputs.size())) {
                continue;
            }

            if (!targetNode->inputs[existingTo->pinIndex].allowMultiple) {
                conflict = &link;
                break;
            }
        }

        if (conflict) {
            error = "Input pin already connected";
            return false;
        }

        GraphLink link{};
        link.id = _nextLinkId++;
        link.startAttr = startAttr;
        link.endAttr = endAttr;
        _links.push_back(link);
        return true;
    }

    bool GraphDocument::removeLink(int linkId) {
        const auto iter = std::find_if(_links.begin(), _links.end(),
                                       [linkId](const GraphLink& link) { return link.id == linkId; });
        if (iter == _links.end()) {
            return false;
        }
        _links.erase(iter);
        return true;
    }

    void GraphDocument::setNodePosition(int nodeId, float x, float y) {
        if (GraphNode* node = findNode(nodeId)) {
            node->posX = x;
            node->posY = y;
        }
    }

    void GraphDocument::syncNodePositionsFromEditor() {}

    void GraphDocument::clear() {
        _nodes.clear();
        _links.clear();
        _nextNodeId = 1;
        _nextLinkId = 1;
    }

}  // namespace mat::demo
