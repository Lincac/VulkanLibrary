#include "graph/GraphTypes.h"

namespace mat::demo {

    const char* nodeTypeName(NodeType type) {
        switch (type) {
            case NodeType::VkPipeline:
                return "VkPipeline";
        }
        return "Unknown";
    }

}  // namespace mat::demo
