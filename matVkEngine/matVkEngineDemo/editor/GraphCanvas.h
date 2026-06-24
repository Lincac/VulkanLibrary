#pragma once

#include "graph/GraphDocument.h"

#include <imgui.h>

namespace mat::demo {

    struct GridViewState {
        ImVec2 pan{0.f, 0.f};
        float zoom = 1.f;
    };

    struct GraphPanelState {
        bool searchOpen = false;
        ImVec2 searchScreenPos{};
        ImVec2 spawnWorldPos{};
        char searchBuffer[128]{};
        bool leftDragActive = false;
        ImVec2 leftDragStart{};
        int searchOpenedFrame = -1;
        bool searchFocusInput = false;
        int selectedNodeId = -1;
        int draggingNodeId = -1;
        ImVec2 nodeDragGrabOffset{};
    };

    void drawGraphPanel(GridViewState& view, GraphPanelState& panel, GraphDocument& document,
                        const ImVec2& displaySize);

}  // namespace mat::demo
