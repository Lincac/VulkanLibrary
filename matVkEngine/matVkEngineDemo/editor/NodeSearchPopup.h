#pragma once

#include "graph/GraphTypes.h"

#include <imgui.h>

namespace mat::demo {

    struct NodeSearchResult {
        NodeType type = NodeType::VkPipeline;
        const char* label = "";
        int score = 0;
    };

    bool drawNodeSearchPopup(const ImVec2& screenPos, char* buffer, int bufferSize, NodeType* outSelectedType,
                             bool* outHovered = nullptr, bool focusInput = false, bool useTypeFilter = false,
                             NodeType typeFilter = NodeType::VkPipeline);

}  // namespace mat::demo
