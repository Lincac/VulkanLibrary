#pragma once

#include "graph/GraphTypes.h"

#include <vector>

namespace mat::demo {

    struct GraphNode {
        int id = 0;
        NodeType type = NodeType::VkPipeline;
        float worldX = 0.f;
        float worldY = 0.f;
    };

    class GraphDocument {
    public:
        int addNode(NodeType type, float worldX, float worldY);

        GraphNode* findNode(int nodeId);
        const GraphNode* findNode(int nodeId) const;
        bool setNodePosition(int nodeId, float worldX, float worldY);

        const std::vector<GraphNode>& nodes() const { return _nodes; }

    private:
        std::vector<GraphNode> _nodes;
        int _nextNodeId = 1;
    };

}  // namespace mat::demo
