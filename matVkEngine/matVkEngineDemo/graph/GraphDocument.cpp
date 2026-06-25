#include "graph/GraphDocument.h"

#include <algorithm>
#include <cstdio>

namespace mat::demo {

    namespace {

        void setVertexAttributeName(VertexAttribute& attribute, const char* name) {
            std::snprintf(attribute.name, sizeof(attribute.name), "%s", name);
        }

    }  // namespace

    int vertexNodeBodyRowCount(const GraphNode& node) {
        return static_cast<int>(node.vertexAttributes.size()) + kVertexAddItemRowCount + kVertexOutputPinCount;
    }

    ImVec2 vertexNodeWorldSize(const GraphNode& node) {
        return ImVec2(kNodeWidth, kNodeHeaderHeight + vertexNodeBodyRowCount(node) * kNodePinRowHeight);
    }

    ImVec2 nodeWorldSize(const GraphNode& node) {
        if (node.type == NodeType::Vertex) {
            return vertexNodeWorldSize(node);
        }
        return nodeWorldSize(node.type);
    }

    void initDefaultVertexAttributes(GraphNode& node) {
        node.vertexAttributes.clear();

        VertexAttribute position{};
        setVertexAttributeName(position, "position");
        position.channelCount = 3;
        node.vertexAttributes.push_back(position);

        VertexAttribute color{};
        setVertexAttributeName(color, "color");
        color.channelCount = 3;
        color.values[0] = 1.f;
        color.values[1] = 1.f;
        color.values[2] = 1.f;
        node.vertexAttributes.push_back(color);

        VertexAttribute texcoord{};
        setVertexAttributeName(texcoord, "texcoord");
        texcoord.channelCount = 2;
        node.vertexAttributes.push_back(texcoord);
    }

    int GraphDocument::addNode(NodeType type, float worldX, float worldY) {
        GraphNode node{};
        node.id = _nextNodeId++;
        node.type = type;
        node.worldX = worldX;
        node.worldY = worldY;
        if (type == NodeType::Vertex) {
            initDefaultVertexAttributes(node);
        }
        _nodes.push_back(node);
        return node.id;
    }

    void GraphDocument::removeLinksToInput(int toNodeId, int toPinIndex) {
        _links.erase(std::remove_if(_links.begin(), _links.end(),
                                    [toNodeId, toPinIndex](const GraphLink& link) {
                                        return link.toNodeId == toNodeId && link.toPinIndex == toPinIndex;
                                    }),
                     _links.end());
    }

    void GraphDocument::removeLinksFromOutput(int fromNodeId, int fromPinIndex) {
        _links.erase(std::remove_if(_links.begin(), _links.end(),
                                    [fromNodeId, fromPinIndex](const GraphLink& link) {
                                        return link.fromNodeId == fromNodeId && link.fromPinIndex == fromPinIndex;
                                    }),
                     _links.end());
    }

    int GraphDocument::addLink(int fromNodeId, int fromPinIndex, int toNodeId, int toPinIndex) {
        if (fromPinIndex < 0) {
            fromPinIndex = 0;
        }

        const GraphNode* toNode = findNode(toNodeId);
        if (toNode == nullptr || !nodeInputPinAllowsMultipleLinks(toNode->type, toPinIndex)) {
            removeLinksToInput(toNodeId, toPinIndex);
        }
        removeLinksFromOutput(fromNodeId, fromPinIndex);

        GraphLink link{};
        link.id = _nextLinkId++;
        link.fromNodeId = fromNodeId;
        link.fromPinIndex = fromPinIndex;
        link.toNodeId = toNodeId;
        link.toPinIndex = toPinIndex;
        _links.push_back(link);
        return link.id;
    }

    GraphNode* GraphDocument::findNode(int nodeId) {
        auto iter = std::find_if(_nodes.begin(), _nodes.end(),
                                 [nodeId](const GraphNode& node) { return node.id == nodeId; });
        return iter != _nodes.end() ? &(*iter) : nullptr;
    }

    const GraphNode* GraphDocument::findNode(int nodeId) const {
        return const_cast<GraphDocument*>(this)->findNode(nodeId);
    }

    bool GraphDocument::setNodePosition(int nodeId, float worldX, float worldY) {
        if (GraphNode* node = findNode(nodeId)) {
            node->worldX = worldX;
            node->worldY = worldY;
            return true;
        }
        return false;
    }

    bool GraphDocument::removeNode(int nodeId) {
        const auto nodeIter = std::find_if(_nodes.begin(), _nodes.end(),
                                           [nodeId](const GraphNode& node) { return node.id == nodeId; });
        if (nodeIter == _nodes.end()) {
            return false;
        }

        _nodes.erase(nodeIter);
        _links.erase(std::remove_if(_links.begin(), _links.end(),
                                    [nodeId](const GraphLink& link) {
                                        return link.fromNodeId == nodeId || link.toNodeId == nodeId;
                                    }),
                     _links.end());
        return true;
    }

}  // namespace mat::demo
