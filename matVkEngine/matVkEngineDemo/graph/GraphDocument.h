#pragma once

#include "graph/GraphTypes.h"

#include <optional>
#include <string>
#include <vector>

namespace mat::demo {

    class GraphDocument {
    public:
        friend class GraphSerializer;

        int addNode(NodeType type, float posX, float posY);
        bool removeNode(int nodeId);
        bool tryConnect(int startAttr, int endAttr, std::string& error);
        bool removeLink(int linkId);

        GraphNode* findNode(int nodeId);
        const GraphNode* findNode(int nodeId) const;

        std::optional<PinId> pinFromAttr(int attrId) const;
        int attrFromPin(const PinId& pin) const;

        const std::vector<GraphNode>& nodes() const { return _nodes; }
        std::vector<GraphNode>& nodes() { return _nodes; }
        const std::vector<GraphLink>& links() const { return _links; }

        int nextNodeId() const { return _nextNodeId; }
        int nextLinkId() const { return _nextLinkId; }

        void setNodePosition(int nodeId, float x, float y);
        void syncNodePositionsFromEditor();
        void clear();

    private:
        std::vector<GraphNode> _nodes;
        std::vector<GraphLink> _links;
        int _nextNodeId = 1;
        int _nextLinkId = 1;

        static constexpr int kPinsPerNode = 32;

        static int encodeAttr(int nodeId, int pinIndex, bool isOutput);
    };

}  // namespace mat::demo
