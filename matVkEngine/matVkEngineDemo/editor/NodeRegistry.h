#pragma once

#include "graph/GraphDocument.h"
#include "graph/GraphTypes.h"

namespace mat::demo {

    class NodeRegistry {
    public:
        static int encodeAttr(int nodeId, int pinIndex, bool isOutput);
        static int encodeStaticAttr(int nodeId);
        static const char* displayName(NodeType type);
        static void drawNodeBody(GraphNode& node);
    };

}  // namespace mat::demo
