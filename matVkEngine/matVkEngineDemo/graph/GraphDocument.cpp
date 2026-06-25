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

    void GraphDocument::removeLinksToInput(int toNodeId, int toPinIndex) {
        _links.erase(std::remove_if(_links.begin(), _links.end(),
                                    [toNodeId, toPinIndex](const GraphLink& link) {
                                        return link.toNodeId == toNodeId && link.toPinIndex == toPinIndex;
                                    }),
                     _links.end());
    }

    void GraphDocument::removeLinksFromOutput(int fromNodeId) {
        _links.erase(std::remove_if(_links.begin(), _links.end(),
                                    [fromNodeId](const GraphLink& link) { return link.fromNodeId == fromNodeId; }),
                     _links.end());
    }

    int GraphDocument::addLink(int fromNodeId, int toNodeId, int toPinIndex) {
        const GraphNode* toNode = findNode(toNodeId);
        if (toNode == nullptr || !nodeInputPinAllowsMultipleLinks(toNode->type, toPinIndex)) {
            removeLinksToInput(toNodeId, toPinIndex);
        }
        removeLinksFromOutput(fromNodeId);

        GraphLink link{};
        link.id = _nextLinkId++;
        link.fromNodeId = fromNodeId;
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
