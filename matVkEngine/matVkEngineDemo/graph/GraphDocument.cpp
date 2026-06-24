#include "graph/GraphDocument.h"

#include <algorithm>

namespace mat::demo {

    int GraphDocument::addNode(NodeType type, float worldX, float worldY) {
        GraphNode node{};
        node.id = _nextNodeId++;
        node.type = type;
        node.worldX = worldX;
        node.worldY = worldY;
        _nodes.push_back(node);
        return node.id;
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

}  // namespace mat::demo
