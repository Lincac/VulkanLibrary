#include "graph/GraphDocument.h"

#include <algorithm>
#include <cstdio>

namespace mat::demo {

    namespace {

        void setVertexAttributeName(VertexAttribute& attribute, const char* name) {
            std::snprintf(attribute.name, sizeof(attribute.name), "%s", name);
        }

        void setIdentityMatrix4x4(float* matrix) {
            for (int index = 0; index < kMatrix4x4ElementCount; ++index) {
                matrix[index] = 0.f;
            }
            matrix[0] = 1.f;
            matrix[5] = 1.f;
            matrix[10] = 1.f;
            matrix[15] = 1.f;
        }

        void setIdentityMatrix3x3(float* matrix) {
            for (int index = 0; index < kMatrix3x3ElementCount; ++index) {
                matrix[index] = 0.f;
            }
            matrix[0] = 1.f;
            matrix[4] = 1.f;
            matrix[8] = 1.f;
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
        if (node.type == NodeType::VkRenderPass) {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + renderPassNodeBodyRowCount(node) * kNodePinRowHeight);
        }
        if (node.type == NodeType::Struct) {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + structNodeBodyRowCount(node) * kNodePinRowHeight);
        }
        return nodeWorldSize(node.type);
    }

    int renderPassNodeBodyRowCount(const GraphNode& node) {
        return kVkRenderPassAttachmentHeaderRowCount + node.renderPassAttachmentSlotCount +
               kVkRenderPassAddAttachmentRowCount + kVkRenderPassFixedInputPinCount;
    }

    int structNodeBodyRowCount(const GraphNode& node) {
        return node.structInputSlotCount + kStructAddInputRowCount;
    }

    int graphNodeInputPinCount(const GraphNode& node) {
        if (node.type == NodeType::VkRenderPass) {
            return node.renderPassAttachmentSlotCount + kVkRenderPassFixedInputPinCount;
        }
        if (node.type == NodeType::Struct) {
            return node.structInputSlotCount;
        }
        return nodeInputPinCount(node.type);
    }

    int graphNodeInputPinBodyRow(const GraphNode& node, int pinIndex) {
        if (node.type == NodeType::VkRenderPass) {
            if (pinIndex < 0) {
                return -1;
            }
            if (pinIndex < node.renderPassAttachmentSlotCount) {
                return kVkRenderPassAttachmentHeaderRowCount + pinIndex;
            }
            if (pinIndex == node.renderPassAttachmentSlotCount) {
                return kVkRenderPassAttachmentHeaderRowCount + node.renderPassAttachmentSlotCount +
                       kVkRenderPassAddAttachmentRowCount;
            }
            if (pinIndex == node.renderPassAttachmentSlotCount + 1) {
                return kVkRenderPassAttachmentHeaderRowCount + node.renderPassAttachmentSlotCount +
                       kVkRenderPassAddAttachmentRowCount + 1;
            }
            return -1;
        }
        if (node.type == NodeType::Struct) {
            if (pinIndex < 0 || pinIndex >= node.structInputSlotCount) {
                return -1;
            }
            return pinIndex;
        }
        return nodeInputPinBodyRow(node.type, pinIndex);
    }

    bool graphNodeInputPinAllowsMultipleLinks(const GraphNode& node, int pinIndex) {
        if (node.type == NodeType::VkRenderPass) {
            if (pinIndex < 0 || pinIndex >= graphNodeInputPinCount(node)) {
                return false;
            }
            return pinIndex >= node.renderPassAttachmentSlotCount;
        }
        return nodeInputPinAllowsMultipleLinks(node.type, pinIndex);
    }

    bool graphNodeGetInputPin(const GraphNode& node, int pinIndex, NodeInputPinInfo& out) {
        if (pinIndex < 0 || pinIndex >= graphNodeInputPinCount(node)) {
            return false;
        }
        if (node.type == NodeType::VkRenderPass) {
            if (pinIndex < node.renderPassAttachmentSlotCount) {
                out.label = "attachment";
                out.slotType = NodeType::VkAttachmentDescription;
                out.slotSourcePinIndex = -1;
                return true;
            }
            if (pinIndex == node.renderPassAttachmentSlotCount) {
                out.label = "VkSubpassDescription";
                out.slotType = NodeType::VkSubpassDescription;
                out.slotSourcePinIndex = -1;
                return true;
            }
            out.label = "VkSubpassDependency";
            out.slotType = NodeType::VkSubpassDependency;
            out.slotSourcePinIndex = -1;
            return true;
        }
        if (node.type == NodeType::Struct) {
            out.label = "member";
            out.slotType = NodeType::Float;
            out.slotSourcePinIndex = -1;
            return true;
        }
        const NodeInputPinDef* pinDef = nodeInputPin(node.type, pinIndex);
        if (pinDef == nullptr) {
            return false;
        }
        out.label = pinDef->label;
        out.slotType = pinDef->slotType;
        out.slotSourcePinIndex = pinDef->slotSourcePinIndex;
        return true;
    }

    bool graphNodeInputPinAcceptsSource(const GraphNode& inputNode, int inputPinIndex, NodeType sourceType,
                                        int sourcePinIndex) {
        if (inputNode.type == NodeType::Struct) {
            if (inputPinIndex < 0 || inputPinIndex >= inputNode.structInputSlotCount) {
                return false;
            }
            if (!isStructValueNodeType(sourceType)) {
                return false;
            }
            (void)sourcePinIndex;
            return true;
        }

        NodeInputPinInfo pinInfo{};
        if (!graphNodeGetInputPin(inputNode, inputPinIndex, pinInfo)) {
            return false;
        }
        if (pinInfo.slotType != sourceType) {
            return false;
        }
        if (pinInfo.slotSourcePinIndex >= 0 && sourcePinIndex >= 0 &&
            pinInfo.slotSourcePinIndex != sourcePinIndex) {
            return false;
        }
        return true;
    }

    int graphNodeInputPinIndexForType(const GraphNode& node, NodeType slotType, int slotSourcePinIndex) {
        const int pinCount = graphNodeInputPinCount(node);
        for (int index = 0; index < pinCount; ++index) {
            if (graphNodeInputPinAcceptsSource(node, index, slotType, slotSourcePinIndex)) {
                return index;
            }
        }
        return -1;
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

    void initIdentityMatrix4x4(GraphNode& node) {
        setIdentityMatrix4x4(node.matrix4x4);
    }

    void initIdentityMatrix3x3(GraphNode& node) {
        setIdentityMatrix3x3(node.matrix3x3);
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
        if (type == NodeType::Matrix4x4) {
            initIdentityMatrix4x4(node);
        }
        if (type == NodeType::Matrix3x3) {
            initIdentityMatrix3x3(node);
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
        if (toNode == nullptr || !graphNodeInputPinAllowsMultipleLinks(*toNode, toPinIndex)) {
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

    bool GraphDocument::addRenderPassAttachmentSlot(int nodeId) {
        GraphNode* node = findNode(nodeId);
        if (node == nullptr || node->type != NodeType::VkRenderPass) {
            return false;
        }
        ++node->renderPassAttachmentSlotCount;
        return true;
    }

    bool GraphDocument::removeRenderPassAttachmentSlot(int nodeId, int slotIndex) {
        GraphNode* node = findNode(nodeId);
        if (node == nullptr || node->type != NodeType::VkRenderPass || node->renderPassAttachmentSlotCount <= 1) {
            return false;
        }
        if (slotIndex < 0 || slotIndex >= node->renderPassAttachmentSlotCount) {
            return false;
        }

        removeLinksToInput(nodeId, slotIndex);
        for (GraphLink& link : _links) {
            if (link.toNodeId == nodeId && link.toPinIndex > slotIndex) {
                --link.toPinIndex;
            }
        }
        --node->renderPassAttachmentSlotCount;
        return true;
    }

    bool GraphDocument::addStructInputSlot(int nodeId) {
        GraphNode* node = findNode(nodeId);
        if (node == nullptr || node->type != NodeType::Struct) {
            return false;
        }
        ++node->structInputSlotCount;
        return true;
    }

    bool GraphDocument::removeStructInputSlot(int nodeId, int slotIndex) {
        GraphNode* node = findNode(nodeId);
        if (node == nullptr || node->type != NodeType::Struct || node->structInputSlotCount <= 0) {
            return false;
        }
        if (slotIndex < 0 || slotIndex >= node->structInputSlotCount) {
            return false;
        }

        removeLinksToInput(nodeId, slotIndex);
        for (GraphLink& link : _links) {
            if (link.toNodeId == nodeId && link.toPinIndex > slotIndex) {
                --link.toPinIndex;
            }
        }
        --node->structInputSlotCount;
        return true;
    }

}  // namespace mat::demo
