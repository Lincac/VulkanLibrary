#pragma once

#include "graph/GraphDocument.h"

#include <imgui.h>

namespace mat::demo {

    struct GridViewState {
        ImVec2 pan{0.f, 0.f};
        float zoom = 1.f;
    };

    enum class GraphSearchMode {
        AddNode,
        PinLink,
    };

    struct PinLinkDrag {
        bool active = false;
        bool dragged = false;
        bool fromInput = false;
        bool pickingExistingLink = false;
        int nodeId = -1;
        int pinIndex = -1;
        int existingLinkId = -1;
        ImVec2 startScreen{};
    };

    struct PinLinkPreview {
        bool active = false;
        bool fromInput = false;
        int nodeId = -1;
        int pinIndex = -1;
        ImVec2 endScreen{};
    };

    struct GraphPanelState {
        bool searchOpen = false;
        GraphSearchMode searchMode = GraphSearchMode::AddNode;
        ImVec2 searchScreenPos{};
        ImVec2 spawnWorldPos{};
        char searchBuffer[128]{};
        bool leftDragActive = false;
        ImVec2 leftDragStart{};
        int searchOpenedFrame = -1;
        bool searchFocusInput = false;
        bool searchTypeFilterEnabled = false;
        NodeType searchTypeFilter = NodeType::VkPipeline;
        int selectedNodeId = -1;
        int draggingNodeId = -1;
        ImVec2 nodeDragGrabOffset{};
        PinLinkDrag pinLinkDrag{};
        PinLinkPreview pinLinkPreview{};
        int pinLinkFromNodeId = -1;
        int pinLinkFromPinIndex = -1;
        bool pinLinkFromInput = false;
        bool suppressNodeWidgetFocus = false;
    };

    void drawGraphPanel(GridViewState& view, GraphPanelState& panel, GraphDocument& document,
                        const ImVec2& displaySize);

}  // namespace mat::demo
