#pragma once

#include "graph/GraphTypes.h"

#include <vector>

namespace mat::demo {

    struct GraphNode {
        int id = 0;
        NodeType type = NodeType::VkPipeline;
        float worldX = 0.f;
        float worldY = 0.f;
        int renderPassIndex = 0;
        int inputAssemblyTopology = 5;
        bool inputAssemblyprimitiveRestart = false;
        int viewportCount = 1;
        int scissorCount = 1;
        bool rasterizerDepthClampEnable = false;
        bool rasterizerDiscard = false;
        int rasterizerPolygonMode = 0;
        float rasterizerLineWidth = 1.0f;
        int rasterizerCullMode = 2;
        int rasterizerFrontFace = 0;
        bool rasterizerDepthBiasEnable = false;
        bool multisampleSampleShadingEnable = false;
        int multisampleRasterizationSamples = 0;
        bool depthStencilDepthTestEnable = true;
        bool depthStencilDepthWriteEnable = true;
        int depthStencilDepthCompareOp = 1;
        bool depthStencilDepthBoundsTestEnable = false;
        bool depthStencilStencilTestEnable = false;
    };

    struct GraphLink {
        int id = 0;
        int fromNodeId = 0;
        int toNodeId = 0;
        int toPinIndex = 0;
    };

    class GraphDocument {
    public:
        int addNode(NodeType type, float worldX, float worldY);
        int addLink(int fromNodeId, int toNodeId, int toPinIndex);

        GraphNode* findNode(int nodeId);
        const GraphNode* findNode(int nodeId) const;
        bool setNodePosition(int nodeId, float worldX, float worldY);
        bool removeNode(int nodeId);

        const std::vector<GraphNode>& nodes() const { return _nodes; }
        const std::vector<GraphLink>& links() const { return _links; }

    private:
        void removeLinksToInput(int toNodeId, int toPinIndex);
        void removeLinksFromOutput(int fromNodeId);

        std::vector<GraphNode> _nodes;
        std::vector<GraphLink> _links;
        int _nextNodeId = 1;
        int _nextLinkId = 1;
    };

}  // namespace mat::demo
