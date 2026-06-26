#include "editor/GraphCanvas.h"

#include "editor/NodeSearchPopup.h"
#include "graph/GraphDocument.h"
#include "graph/GraphTypes.h"

#include <imgui.h>

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cfloat>

namespace mat::demo {

    namespace {

        constexpr float kDragThreshold = 4.f;
        constexpr float kNodeRounding = 8.f;
        constexpr float kNodeToolbarHeightWorld = 24.f;
        constexpr float kNodeToolbarGapWorld = 4.f;

        struct NodeTheme {
            ImU32 header;
            ImU32 body;
            ImU32 border;
            ImU32 selectedBorder;
            ImU32 title;
            ImU32 pinLabel;
        };

        ImVec2 screenCenter(const ImVec2& displaySize) {
            return ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);
        }

        ImVec2 worldToScreen(const ImVec2& world, const GridViewState& view, const ImVec2& displaySize) {
            const ImVec2 center = screenCenter(displaySize);
            return ImVec2(center.x + world.x * view.zoom + view.pan.x, center.y + world.y * view.zoom + view.pan.y);
        }

        ImVec2 screenToWorld(const ImVec2& screen, const GridViewState& view, const ImVec2& displaySize) {
            const ImVec2 center = screenCenter(displaySize);
            return ImVec2((screen.x - center.x - view.pan.x) / view.zoom,
                          (screen.y - center.y - view.pan.y) / view.zoom);
        }

        NodeTheme nodeTheme(NodeType type) {
            (void)type;
            return {IM_COL32(62, 102, 152, 255), IM_COL32(48, 48, 56, 255), IM_COL32(72, 72, 84, 255),
                    IM_COL32(232, 196, 118, 255), IM_COL32(245, 245, 250, 255), IM_COL32(190, 200, 215, 255)};
        }

        struct NodeScreenLayout {
            ImVec2 topLeft;
            ImVec2 bottomRight;
            float width = 0.f;
            float height = 0.f;
            float headerHeight = 0.f;
            float pinRowHeight = 0.f;
            float fontSize = 0.f;
            float zoom = 1.f;
        };

        NodeScreenLayout buildNodeScreenLayout(const GraphNode& node, const GridViewState& view,
                                               const ImVec2& displaySize) {
            const float zoom = view.zoom;
            const ImVec2 worldSize = nodeWorldSize(node);
            const ImVec2 screenPos = worldToScreen(ImVec2(node.worldX, node.worldY), view, displaySize);
            const float width = worldSize.x * zoom;
            const float height = worldSize.y * zoom;

            NodeScreenLayout layout{};
            layout.width = width;
            layout.height = height;
            layout.topLeft = ImVec2(screenPos.x - width * 0.5f, screenPos.y - height * 0.5f);
            layout.bottomRight = ImVec2(layout.topLeft.x + width, layout.topLeft.y + height);
            layout.headerHeight = kNodeHeaderHeight * zoom;
            layout.pinRowHeight = kNodePinRowHeight * zoom;
            layout.fontSize = ImGui::GetFontSize() * zoom;
            layout.zoom = zoom;
            return layout;
        }

        float vkPipelineIndexRowCenterY(const NodeScreenLayout& layout) {
            return layout.topLeft.y + layout.headerHeight + (kVkPipelineInputPinCount + 0.5f) * layout.pinRowHeight;
        }

        float nodeParamRowCenterY(const NodeScreenLayout& layout, int rowIndex) {
            return layout.topLeft.y + layout.headerHeight + (rowIndex + 0.5f) * layout.pinRowHeight;
        }

        struct PinHit {
            bool valid = false;
            int nodeId = -1;
            int pinIndex = -1;
            bool isInput = false;
        };

        bool getOutputPinScreenPos(const GraphNode& node, int pinIndex, const GridViewState& view,
                                   const ImVec2& displaySize, ImVec2& outPos) {
            if (!nodeHasOutputPin(node.type)) {
                return false;
            }

            const int resolvedPinIndex = pinIndex < 0 ? 0 : pinIndex;
            const int outputPinCount = nodeOutputPinCount(node.type);
            if (resolvedPinIndex >= outputPinCount) {
                return false;
            }

            const NodeScreenLayout layout = buildNodeScreenLayout(node, view, displaySize);
            if (node.type == NodeType::VkColorWriteMask || node.type == NodeType::VkDynamicState ||
                node.type == NodeType::Vertex) {
                int bodyRow = resolvedPinIndex;
                if (node.type == NodeType::Vertex) {
                    bodyRow = static_cast<int>(node.vertexAttributes.size()) + kVertexAddItemRowCount +
                              resolvedPinIndex;
                }
                const float rowCenterY =
                    layout.topLeft.y + layout.headerHeight + (bodyRow + 0.5f) * layout.pinRowHeight;
                outPos = ImVec2(layout.bottomRight.x, rowCenterY);
                return true;
            }

            const float bodyCenterY = layout.topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            outPos = ImVec2(layout.bottomRight.x, bodyCenterY);
            return true;
        }

        bool getInputPinScreenPos(const GraphNode& node, int pinIndex, const GridViewState& view,
                                  const ImVec2& displaySize, ImVec2& outPos) {
            const int pinCount = graphNodeInputPinCount(node);
            if (pinCount <= 0 || pinIndex < 0 || pinIndex >= pinCount) {
                return false;
            }

            const NodeScreenLayout layout = buildNodeScreenLayout(node, view, displaySize);
            const int bodyRow = graphNodeInputPinBodyRow(node, pinIndex);
            const float rowCenterY = layout.topLeft.y + layout.headerHeight + (bodyRow + 0.5f) * layout.pinRowHeight;
            outPos = ImVec2(layout.topLeft.x, rowCenterY);
            return true;
        }

        float pinHitRadius(float zoom) {
            return (5.f + 1.2f) * zoom + 6.f;
        }

        PinHit hitTestPin(const ImVec2& screenPos, const GraphDocument& document, const GridViewState& view,
                          const ImVec2& displaySize) {
            PinHit best{};
            float bestDistSq = FLT_MAX;

            for (auto iter = document.nodes().rbegin(); iter != document.nodes().rend(); ++iter) {
                const GraphNode& node = *iter;

                if (nodeHasInputPins(node.type)) {
                    const int pinCount = graphNodeInputPinCount(node);
                    for (int pinIndex = 0; pinIndex < pinCount; ++pinIndex) {
                        ImVec2 pinPos;
                        if (!getInputPinScreenPos(node, pinIndex, view, displaySize, pinPos)) {
                            continue;
                        }

                        const float radius = pinHitRadius(view.zoom);
                        const float dx = screenPos.x - pinPos.x;
                        const float dy = screenPos.y - pinPos.y;
                        const float distSq = dx * dx + dy * dy;
                        if (distSq <= radius * radius && distSq < bestDistSq) {
                            bestDistSq = distSq;
                            best.valid = true;
                            best.nodeId = node.id;
                            best.pinIndex = pinIndex;
                            best.isInput = true;
                        }
                    }
                }

                if (nodeHasOutputPin(node.type)) {
                    const int outputPinCount = nodeOutputPinCount(node.type);
                    for (int pinIndex = 0; pinIndex < outputPinCount; ++pinIndex) {
                        ImVec2 pinPos;
                        if (!getOutputPinScreenPos(node, pinIndex, view, displaySize, pinPos)) {
                            continue;
                        }

                        const float radius = pinHitRadius(view.zoom);
                        const float dx = screenPos.x - pinPos.x;
                        const float dy = screenPos.y - pinPos.y;
                        const float distSq = dx * dx + dy * dy;
                        if (distSq <= radius * radius && distSq < bestDistSq) {
                            bestDistSq = distSq;
                            best.valid = true;
                            best.nodeId = node.id;
                            best.pinIndex = pinIndex;
                            best.isInput = false;
                        }
                    }
                }
            }

            return best;
        }

        bool getPinScreenPos(const GraphDocument& document, int nodeId, int pinIndex, bool isInput,
                             const GridViewState& view, const ImVec2& displaySize, ImVec2& outPos) {
            const GraphNode* node = document.findNode(nodeId);
            if (node == nullptr) {
                return false;
            }

            if (isInput) {
                return getInputPinScreenPos(*node, pinIndex, view, displaySize, outPos);
            }
            return getOutputPinScreenPos(*node, pinIndex, view, displaySize, outPos);
        }

        float linkTangent(const ImVec2& start, const ImVec2& end) {
            return std::max(std::fabs(end.x - start.x) * 0.5f, 80.f);
        }

        void drawComfyBezierLink(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, bool startIsOutput,
                                 ImU32 color, float thickness, bool isPreview) {
            const float tangent = linkTangent(start, end);
            ImVec2 cp1;
            ImVec2 cp2;
            if (startIsOutput) {
                cp1 = ImVec2(start.x + tangent, start.y);
                cp2 = ImVec2(end.x - tangent, end.y);
            } else {
                cp1 = ImVec2(start.x - tangent, start.y);
                cp2 = ImVec2(end.x + tangent, end.y);
            }

            const float shadowThickness = thickness + 4.f;
            const ImU32 shadowColor = isPreview ? IM_COL32(0, 0, 0, 90) : IM_COL32(0, 0, 0, 120);
            drawList->AddBezierCubic(start, cp1, cp2, end, shadowColor, shadowThickness);
            drawList->AddBezierCubic(start, cp1, cp2, end, color, thickness);
        }

        void drawBezierLink(ImDrawList* drawList, const ImVec2& start, const ImVec2& end, bool startIsOutput,
                            ImU32 color, float thickness) {
            drawComfyBezierLink(drawList, start, end, startIsOutput, color, thickness, false);
        }

        bool isPinHighlighted(const PinHit& pin, int nodeId, int pinIndex, bool isInput,
                              const GraphDocument* document = nullptr) {
            if (pin.valid && pin.nodeId == nodeId && pin.pinIndex == pinIndex && pin.isInput == isInput) {
                return true;
            }
            if (document == nullptr || !pin.valid) {
                return false;
            }

            for (const GraphLink& link : document->links()) {
                if (pin.isInput) {
                    if (link.toNodeId == pin.nodeId && link.toPinIndex == pin.pinIndex && link.fromNodeId == nodeId &&
                        link.fromPinIndex == pinIndex && !isInput) {
                        return true;
                    }
                } else if (link.fromNodeId == pin.nodeId && link.fromPinIndex == pin.pinIndex &&
                           link.toNodeId == nodeId && link.toPinIndex == pinIndex && isInput) {
                    return true;
                }
            }
            return false;
        }

        bool isPinLinkSource(const GraphPanelState& panel, int nodeId, int pinIndex, bool isInput) {
            if (panel.pinLinkDrag.active &&
                isPinHighlighted({true, panel.pinLinkDrag.nodeId, panel.pinLinkDrag.pinIndex,
                                  panel.pinLinkDrag.fromInput},
                                 nodeId, pinIndex, isInput)) {
                return true;
            }
            return panel.pinLinkPreview.active &&
                   isPinHighlighted({true, panel.pinLinkPreview.nodeId, panel.pinLinkPreview.pinIndex,
                                     panel.pinLinkPreview.fromInput},
                                    nodeId, pinIndex, isInput);
        }

        bool isLinkHighlighted(const PinHit& hoveredPin, const GraphLink& link) {
            if (!hoveredPin.valid) {
                return false;
            }
            if (hoveredPin.isInput) {
                return link.toNodeId == hoveredPin.nodeId && link.toPinIndex == hoveredPin.pinIndex;
            }
            return link.fromNodeId == hoveredPin.nodeId && link.fromPinIndex == hoveredPin.pinIndex;
        }

        const GraphDocument* g_pinHighlightDocument = nullptr;
        const GraphPanelState* g_pinHighlightPanel = nullptr;
        PinHit g_hoveredPin{};

        void beginPinHighlightContext(const GraphDocument& document, const GraphPanelState& panel,
                                      const PinHit& hoveredPin) {
            g_pinHighlightDocument = &document;
            g_pinHighlightPanel = &panel;
            g_hoveredPin = hoveredPin;
        }

        void endPinHighlightContext() {
            g_pinHighlightDocument = nullptr;
            g_pinHighlightPanel = nullptr;
            g_hoveredPin = {};
        }

        bool shouldHighlightPin(int nodeId, int pinIndex, bool isInput) {
            if (g_pinHighlightDocument == nullptr || g_pinHighlightPanel == nullptr) {
                return false;
            }
            return isPinHighlighted(g_hoveredPin, nodeId, pinIndex, isInput, g_pinHighlightDocument) ||
                   isPinLinkSource(*g_pinHighlightPanel, nodeId, pinIndex, isInput);
        }

        void drawGraphLinks(const GraphDocument& document, const GridViewState& view, const ImVec2& displaySize,
                            const PinHit& hoveredPin) {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            const ImU32 linkColor = IM_COL32(210, 210, 220, 235);
            const ImU32 highlightLinkColor = IM_COL32(232, 196, 118, 255);
            const float thickness = std::max(2.5f, 3.f * view.zoom);
            const float highlightThickness = thickness + 1.2f;

            for (const GraphLink& link : document.links()) {
                const GraphNode* fromNode = document.findNode(link.fromNodeId);
                const GraphNode* toNode = document.findNode(link.toNodeId);
                if (fromNode == nullptr || toNode == nullptr) {
                    continue;
                }

                ImVec2 startPos;
                ImVec2 endPos;
                if (!getOutputPinScreenPos(*fromNode, link.fromPinIndex, view, displaySize, startPos) ||
                    !getInputPinScreenPos(*toNode, link.toPinIndex, view, displaySize, endPos)) {
                    continue;
                }

                const bool highlighted = isLinkHighlighted(hoveredPin, link);
                drawBezierLink(drawList, startPos, endPos, true, highlighted ? highlightLinkColor : linkColor,
                               highlighted ? highlightThickness : thickness);
            }
        }

        void drawPinLinkPreview(const GraphPanelState& panel, const GraphDocument& document, const GridViewState& view,
                                const ImVec2& displaySize) {
            ImVec2 startPos;
            ImVec2 endPos;
            bool fromInput = false;
            bool shouldDraw = false;

            if (panel.pinLinkDrag.active && panel.pinLinkDrag.dragged) {
                if (!getPinScreenPos(document, panel.pinLinkDrag.nodeId, panel.pinLinkDrag.pinIndex,
                                     panel.pinLinkDrag.fromInput, view, displaySize, startPos)) {
                    return;
                }
                endPos = ImGui::GetIO().MousePos;
                fromInput = panel.pinLinkDrag.fromInput;
                shouldDraw = true;
            } else if (panel.pinLinkPreview.active) {
                if (!getPinScreenPos(document, panel.pinLinkPreview.nodeId, panel.pinLinkPreview.pinIndex,
                                     panel.pinLinkPreview.fromInput, view, displaySize, startPos)) {
                    return;
                }
                endPos = panel.pinLinkPreview.endScreen;
                fromInput = panel.pinLinkPreview.fromInput;
                shouldDraw = true;
            }

            if (!shouldDraw) {
                return;
            }

            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            const ImU32 previewColor = IM_COL32(245, 245, 250, 245);
            const float thickness = std::max(2.5f, 3.f * view.zoom);
            drawComfyBezierLink(drawList, startPos, endPos, !fromInput, previewColor, thickness, true);
        }

        void clearPinLinkPreview(GraphPanelState& panel) {
            panel.pinLinkPreview.active = false;
            panel.pinLinkPreview.nodeId = -1;
            panel.pinLinkPreview.pinIndex = -1;
            panel.pinLinkPreview.fromInput = false;
            panel.pinLinkPreview.endScreen = ImVec2{};
        }

        bool canConnectPinLink(const GraphDocument& document, const PinLinkDrag& drag, const PinHit& target) {
            if (!drag.active || !target.valid || drag.nodeId == target.nodeId) {
                return false;
            }

            if (drag.fromInput && !target.isInput) {
                const GraphNode* inputNode = document.findNode(drag.nodeId);
                const GraphNode* sourceNode = document.findNode(target.nodeId);
                if (inputNode == nullptr || sourceNode == nullptr || !nodeHasInputPins(inputNode->type) ||
                    !nodeHasOutputPin(sourceNode->type)) {
                    return false;
                }

                const int sourcePinIndex = target.pinIndex < 0 ? 0 : target.pinIndex;
                return graphNodeInputPinAcceptsSource(*inputNode, drag.pinIndex, sourceNode->type, sourcePinIndex);
            }

            if (!drag.fromInput && target.isInput) {
                const GraphNode* sourceNode = document.findNode(drag.nodeId);
                const GraphNode* inputNode = document.findNode(target.nodeId);
                if (sourceNode == nullptr || inputNode == nullptr || !nodeHasInputPins(inputNode->type) ||
                    !nodeHasOutputPin(sourceNode->type)) {
                    return false;
                }

                const int sourcePinIndex = drag.pinIndex < 0 ? 0 : drag.pinIndex;
                return graphNodeInputPinAcceptsSource(*inputNode, target.pinIndex, sourceNode->type, sourcePinIndex);
            }

            return false;
        }

        bool tryConnectPinLink(GraphDocument& document, const PinLinkDrag& drag, const PinHit& target) {
            if (!canConnectPinLink(document, drag, target)) {
                return false;
            }

            if (drag.fromInput) {
                const int fromPinIndex = target.pinIndex < 0 ? 0 : target.pinIndex;
                document.addLink(target.nodeId, fromPinIndex, drag.nodeId, drag.pinIndex);
            } else {
                const int fromPinIndex = drag.pinIndex < 0 ? 0 : drag.pinIndex;
                document.addLink(drag.nodeId, fromPinIndex, target.nodeId, target.pinIndex);
            }
            return true;
        }

        void openPinLinkSearch(GraphPanelState& panel, const ImVec2& screenPos, const GridViewState& view,
                               const ImVec2& displaySize, const GraphDocument& document) {
            panel.searchOpen = true;
            panel.searchMode = GraphSearchMode::PinLink;
            panel.searchScreenPos = screenPos;
            panel.spawnWorldPos = screenToWorld(screenPos, view, displaySize);
            panel.searchBuffer[0] = '\0';
            panel.searchFocusInput = true;
            panel.searchOpenedFrame = ImGui::GetFrameCount();
            panel.leftDragActive = false;

            panel.pinLinkFromNodeId = panel.pinLinkDrag.nodeId;
            panel.pinLinkFromPinIndex = panel.pinLinkDrag.pinIndex;
            panel.pinLinkFromInput = panel.pinLinkDrag.fromInput;

            panel.searchTypeFilterEnabled = true;
            if (panel.pinLinkDrag.fromInput) {
                const GraphNode* inputNode = document.findNode(panel.pinLinkDrag.nodeId);
                NodeInputPinInfo pinInfo{};
                if (inputNode != nullptr) {
                    graphNodeGetInputPin(*inputNode, panel.pinLinkDrag.pinIndex, pinInfo);
                }
                panel.searchTypeFilter = pinInfo.label[0] != '\0' ? pinInfo.slotType : NodeType::VkPipeline;
            } else if (const GraphNode* sourceNode = document.findNode(panel.pinLinkDrag.nodeId)) {
                panel.searchTypeFilter = pinLinkTargetNodeTypeForSource(sourceNode->type);
            } else {
                panel.searchTypeFilter = NodeType::VkPipeline;
            }
        }

        void finalizePinLinkSearch(GraphPanelState& panel, GraphDocument& document, NodeType selectedType) {
            const int newNodeId = document.addNode(selectedType, panel.spawnWorldPos.x, panel.spawnWorldPos.y);

            if (panel.pinLinkFromInput) {
                const GraphNode* inputNode = document.findNode(panel.pinLinkFromNodeId);
                NodeInputPinInfo pinInfo{};
                int fromPinIndex = 0;
                if (inputNode != nullptr && graphNodeGetInputPin(*inputNode, panel.pinLinkFromPinIndex, pinInfo) &&
                    pinInfo.slotSourcePinIndex >= 0) {
                    fromPinIndex = pinInfo.slotSourcePinIndex;
                }
                document.addLink(newNodeId, fromPinIndex, panel.pinLinkFromNodeId, panel.pinLinkFromPinIndex);
            } else {
                const GraphNode* sourceNode = document.findNode(panel.pinLinkFromNodeId);
                const GraphNode* newNode = document.findNode(newNodeId);
                if (sourceNode != nullptr && newNode != nullptr && nodeHasInputPins(newNode->type)) {
                    const int fromPinIndex =
                        panel.pinLinkFromPinIndex < 0 ? 0 : panel.pinLinkFromPinIndex;
                    const int pinIndex =
                        graphNodeInputPinIndexForType(*newNode, sourceNode->type, fromPinIndex);
                    if (pinIndex >= 0) {
                        document.addLink(panel.pinLinkFromNodeId, fromPinIndex, newNodeId, pinIndex);
                    }
                }
            }

            panel.selectedNodeId = newNodeId;
            panel.searchMode = GraphSearchMode::AddNode;
            panel.searchTypeFilterEnabled = false;
            panel.pinLinkFromNodeId = -1;
            panel.pinLinkFromPinIndex = -1;
            panel.pinLinkFromInput = false;
            clearPinLinkPreview(panel);
        }

        void handlePinLinkInteraction(GraphPanelState& panel, GraphDocument& document, const GridViewState& view,
                                      const ImVec2& displaySize, bool blockGraphDrag) {
            ImGuiIO& io = ImGui::GetIO();

            if (panel.pinLinkDrag.active) {
                if (io.MouseDown[ImGuiMouseButton_Left]) {
                    const float dx = io.MousePos.x - panel.pinLinkDrag.startScreen.x;
                    const float dy = io.MousePos.y - panel.pinLinkDrag.startScreen.y;
                    if ((dx * dx + dy * dy) >= kDragThreshold * kDragThreshold) {
                        panel.pinLinkDrag.dragged = true;
                    }
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    if (panel.pinLinkDrag.dragged) {
                        const PinHit targetPin = hitTestPin(io.MousePos, document, view, displaySize);
                        if (!tryConnectPinLink(document, panel.pinLinkDrag, targetPin)) {
                            panel.pinLinkPreview.active = true;
                            panel.pinLinkPreview.fromInput = panel.pinLinkDrag.fromInput;
                            panel.pinLinkPreview.nodeId = panel.pinLinkDrag.nodeId;
                            panel.pinLinkPreview.pinIndex = panel.pinLinkDrag.pinIndex;
                            panel.pinLinkPreview.endScreen = io.MousePos;
                            openPinLinkSearch(panel, io.MousePos, view, displaySize, document);
                        } else {
                            clearPinLinkPreview(panel);
                        }
                    }
                    panel.pinLinkDrag.active = false;
                    panel.pinLinkDrag.dragged = false;
                }
                return;
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !blockGraphDrag) {
                const PinHit pinHit = hitTestPin(io.MousePos, document, view, displaySize);
                if (pinHit.valid) {
                    clearPinLinkPreview(panel);
                    panel.pinLinkDrag.active = true;
                    panel.pinLinkDrag.dragged = false;
                    panel.pinLinkDrag.fromInput = pinHit.isInput;
                    panel.pinLinkDrag.nodeId = pinHit.nodeId;
                    panel.pinLinkDrag.pinIndex = pinHit.pinIndex;
                    panel.leftDragActive = false;
                    panel.draggingNodeId = -1;

                    const GraphNode* pinNode = document.findNode(pinHit.nodeId);
                    if (pinNode != nullptr) {
                        if (pinHit.isInput) {
                            getInputPinScreenPos(*pinNode, pinHit.pinIndex, view, displaySize,
                                                 panel.pinLinkDrag.startScreen);
                        } else {
                            getOutputPinScreenPos(*pinNode, pinHit.pinIndex, view, displaySize,
                                                  panel.pinLinkDrag.startScreen);
                        }
                    }
                }
            }
        }

        void nodeScreenBounds(const GraphNode& node, const GridViewState& view, const ImVec2& displaySize,
                              ImVec2& topLeft, ImVec2& bottomRight) {
            const ImVec2 screenPos = worldToScreen(ImVec2(node.worldX, node.worldY), view, displaySize);
            const ImVec2 worldSize = nodeWorldSize(node);
            const ImVec2 nodeSize(worldSize.x * view.zoom, worldSize.y * view.zoom);
            topLeft = ImVec2(screenPos.x - nodeSize.x * 0.5f, screenPos.y - nodeSize.y * 0.5f);
            bottomRight = ImVec2(topLeft.x + nodeSize.x, topLeft.y + nodeSize.y);
        }

        int hitTestNode(const ImVec2& screenPos, const GraphDocument& document, const GridViewState& view,
                        const ImVec2& displaySize) {
            const auto& nodes = document.nodes();
            for (auto iter = nodes.rbegin(); iter != nodes.rend(); ++iter) {
                ImVec2 topLeft;
                ImVec2 bottomRight;
                nodeScreenBounds(*iter, view, displaySize, topLeft, bottomRight);
                if (screenPos.x >= topLeft.x && screenPos.x <= bottomRight.x && screenPos.y >= topLeft.y &&
                    screenPos.y <= bottomRight.y) {
                    return iter->id;
                }
            }
            return -1;
        }

        void drawScaledText(ImDrawList* drawList, const ImVec2& pos, ImU32 color, const char* text, float fontSize) {
            drawList->AddText(ImGui::GetFont(), fontSize, pos, color, text);
        }

        void drawPin(ImDrawList* drawList, const ImVec2& center, float zoom, ImU32 fillColor, bool highlighted) {
            const float slotRadius = 5.f * zoom;
            if (highlighted) {
                drawList->AddCircleFilled(center, slotRadius + 5.f * zoom, IM_COL32(232, 196, 118, 70));
                drawList->AddCircle(center, slotRadius + 2.8f * zoom, IM_COL32(232, 196, 118, 220), 0, 2.f * zoom);
                fillColor = IM_COL32(232, 196, 118, 255);
            }
            drawList->AddCircleFilled(center, slotRadius + 1.2f * zoom, IM_COL32(18, 18, 22, 255));
            drawList->AddCircleFilled(center, slotRadius, fillColor);
        }

        void trackWidgetInteraction(bool interactive, bool& blockGraphDrag) {
            if (interactive && (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                blockGraphDrag = true;
            }
        }

        struct InputAssemblyFieldLayout {
            float sTypeFieldX = 0.f;
            float sTypeFieldWidth = 0.f;
            float comboFieldX = 0.f;
            float comboFieldWidth = 0.f;
            float fieldHeight = 0.f;
        };

        InputAssemblyFieldLayout inputAssemblyStateFieldLayout(const NodeScreenLayout& layout) {
            const float labelAreaWidth = 110.f * layout.zoom;
            const float fieldPadRight = 18.f * layout.zoom;
            const float comboFieldWidth = std::max(100.f * layout.zoom, layout.fontSize * 5.5f);
            InputAssemblyFieldLayout result{};
            result.sTypeFieldWidth = layout.width - labelAreaWidth - fieldPadRight;
            result.sTypeFieldX = layout.topLeft.x + labelAreaWidth;
            result.comboFieldWidth = comboFieldWidth;
            result.comboFieldX = layout.bottomRight.x - fieldPadRight - comboFieldWidth;
            result.fieldHeight = layout.fontSize + 4.f * layout.zoom;
            return result;
        }

        bool beginNodeCombo(const char* label, const char* preview, float width) {
            ImGui::SetNextItemWidth(width);
            return ImGui::BeginCombo(label, preview);
        }

        void drawVkPipelineInputAssemblyStateContent(ImDrawList* drawList, const GraphNode& node,
                                                     const NodeScreenLayout& layout, const NodeTheme& theme,
                                                     const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            static const char* kParamLabels[kVkPipelineInputAssemblyStateParamCount] = {
                "sType", "topology", "primitiveRestart"};

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkPipelineInputAssemblyStateParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                const char* valueText = "";
                if (rowIndex == 0) {
                    valueText = kVkPipelineInputAssemblyStateSType;
                } else if (rowIndex == 1) {
                    valueText = vkPrimitiveTopologyOptionName(node.inputAssemblyTopology);
                } else {
                    valueText = node.inputAssemblyprimitiveRestart ? "true" : "false";
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkPipelineInputAssemblyStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                     bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineInputAssemblyStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineInputAssemblyStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float topologyRowY = nodeParamRowCenterY(layout, 1) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, topologyRowY));
            if (beginNodeCombo("##topology", vkPrimitiveTopologyOptionName(node.inputAssemblyTopology),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkPrimitiveTopologyOptionCount; ++optionIndex) {
                    const bool selected = node.inputAssemblyTopology == optionIndex;
                    if (ImGui::Selectable(vkPrimitiveTopologyOptionName(optionIndex), selected)) {
                        node.inputAssemblyTopology = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float restartRowY = nodeParamRowCenterY(layout, 2) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, restartRowY));
            const char* restartLabels[] = {"false", "true"};
            const int restartIndex = node.inputAssemblyprimitiveRestart ? 1 : 0;
            if (beginNodeCombo("##primitiveRestart", restartLabels[restartIndex], fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < 2; ++optionIndex) {
                    const bool selected = restartIndex == optionIndex;
                    if (ImGui::Selectable(restartLabels[optionIndex], selected)) {
                        node.inputAssemblyprimitiveRestart = (optionIndex == 1);
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineViewportStateContent(ImDrawList* drawList, const GraphNode& node,
                                                const NodeScreenLayout& layout, const NodeTheme& theme,
                                                const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            static const char* kParamLabels[kVkPipelineViewportStateParamCount] = {
                "sType", "viewportCount", "scissorCount"};

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkPipelineViewportStateParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                if (rowIndex == 0) {
                    std::snprintf(valueText, sizeof(valueText), "%s", kVkPipelineViewportStateSType);
                } else if (rowIndex == 1) {
                    std::snprintf(valueText, sizeof(valueText), "%d", node.viewportCount);
                } else {
                    std::snprintf(valueText, sizeof(valueText), "%d", node.scissorCount);
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkPipelineViewportStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineViewportStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineViewportStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float viewportCountRowY = nodeParamRowCenterY(layout, 1) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, viewportCountRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputInt("##viewportCount", &node.viewportCount, 0, 0);
            if (node.viewportCount < 0) {
                node.viewportCount = 0;
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float scissorCountRowY = nodeParamRowCenterY(layout, 2) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, scissorCountRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputInt("##scissorCount", &node.scissorCount, 0, 0);
            if (node.scissorCount < 0) {
                node.scissorCount = 0;
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkBool32Combo(const char* id, bool& value, float rowCenterY, const InputAssemblyFieldLayout& fieldLayout,
                               bool interactive, bool& blockGraphDrag) {
            ImGui::SetCursorScreenPos(
                ImVec2(fieldLayout.comboFieldX, rowCenterY - fieldLayout.fieldHeight * 0.5f));
            if (beginNodeCombo(id, vkBool32OptionName(value), fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < 2; ++optionIndex) {
                    const bool optionValue = optionIndex == 1;
                    const bool selected = value == optionValue;
                    if (ImGui::Selectable(vkBool32OptionName(optionValue), selected)) {
                        value = optionValue;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);
        }

        void drawVkPipelineRasterizationStateContent(ImDrawList* drawList, const GraphNode& node,
                                                     const NodeScreenLayout& layout, const NodeTheme& theme,
                                                     const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            static const char* kParamLabels[kVkPipelineRasterizationStateParamCount] = {
                "sType",
                "depthClampEnable",
                "rasterizerDiscard",
                "polygonMode",
                "lineWidth",
                "cullMode",
                "frontFace",
                "depthBiasEnable",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkPipelineRasterizationStateParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                switch (rowIndex) {
                    case 0:
                        std::snprintf(valueText, sizeof(valueText), "%s", kVkPipelineRasterizationStateSType);
                        break;
                    case 1:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.rasterizerDepthClampEnable));
                        break;
                    case 2:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.rasterizerDiscard));
                        break;
                    case 3:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkPolygonModeOptionName(node.rasterizerPolygonMode));
                        break;
                    case 4:
                        std::snprintf(valueText, sizeof(valueText), "%.1f", node.rasterizerLineWidth);
                        break;
                    case 5:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkCullModeOptionName(node.rasterizerCullMode));
                        break;
                    case 6:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkFrontFaceOptionName(node.rasterizerFrontFace));
                        break;
                    default:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.rasterizerDepthBiasEnable));
                        break;
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkPipelineRasterizationStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                     bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineRasterizationStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineRasterizationStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            drawVkBool32Combo("##depthClampEnable", node.rasterizerDepthClampEnable, nodeParamRowCenterY(layout, 1),
                              fieldLayout, interactive, blockGraphDrag);
            drawVkBool32Combo("##rasterizerDiscard", node.rasterizerDiscard,
                              nodeParamRowCenterY(layout, 2), fieldLayout, interactive, blockGraphDrag);

            const float polygonModeRowY = nodeParamRowCenterY(layout, 3) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, polygonModeRowY));
            if (beginNodeCombo("##polygonMode", vkPolygonModeOptionName(node.rasterizerPolygonMode),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkPolygonModeOptionCount; ++optionIndex) {
                    const bool selected = node.rasterizerPolygonMode == optionIndex;
                    if (ImGui::Selectable(vkPolygonModeOptionName(optionIndex), selected)) {
                        node.rasterizerPolygonMode = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float lineWidthRowY = nodeParamRowCenterY(layout, 4) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, lineWidthRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputFloat("##lineWidth", &node.rasterizerLineWidth, 0.f, 0.f, "%.1f");
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float cullModeRowY = nodeParamRowCenterY(layout, 5) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, cullModeRowY));
            if (beginNodeCombo("##cullMode", vkCullModeOptionName(node.rasterizerCullMode),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkCullModeOptionCount; ++optionIndex) {
                    const bool selected = node.rasterizerCullMode == optionIndex;
                    if (ImGui::Selectable(vkCullModeOptionName(optionIndex), selected)) {
                        node.rasterizerCullMode = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float frontFaceRowY = nodeParamRowCenterY(layout, 6) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, frontFaceRowY));
            if (beginNodeCombo("##frontFace", vkFrontFaceOptionName(node.rasterizerFrontFace),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkFrontFaceOptionCount; ++optionIndex) {
                    const bool selected = node.rasterizerFrontFace == optionIndex;
                    if (ImGui::Selectable(vkFrontFaceOptionName(optionIndex), selected)) {
                        node.rasterizerFrontFace = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            drawVkBool32Combo("##depthBiasEnable", node.rasterizerDepthBiasEnable, nodeParamRowCenterY(layout, 7),
                              fieldLayout, interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineMultisampleStateContent(ImDrawList* drawList, const GraphNode& node,
                                                   const NodeScreenLayout& layout, const NodeTheme& theme,
                                                   const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            static const char* kParamLabels[kVkPipelineMultisampleStateParamCount] = {
                "sType", "sampleShadingEnable", "rasterizationSamples"};

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkPipelineMultisampleStateParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                if (rowIndex == 0) {
                    std::snprintf(valueText, sizeof(valueText), "%s", kVkPipelineMultisampleStateSType);
                } else if (rowIndex == 1) {
                    std::snprintf(valueText, sizeof(valueText), "%s",
                                  vkBool32OptionName(node.multisampleSampleShadingEnable));
                } else {
                    std::snprintf(valueText, sizeof(valueText), "%s",
                                  vkSampleCountOptionName(node.multisampleRasterizationSamples));
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkPipelineMultisampleStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                   bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineMultisampleStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineMultisampleStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            drawVkBool32Combo("##sampleShadingEnable", node.multisampleSampleShadingEnable,
                              nodeParamRowCenterY(layout, 1), fieldLayout, interactive, blockGraphDrag);

            const float samplesRowY = nodeParamRowCenterY(layout, 2) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, samplesRowY));
            if (beginNodeCombo("##rasterizationSamples",
                               vkSampleCountOptionName(node.multisampleRasterizationSamples),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkSampleCountOptionCount; ++optionIndex) {
                    const bool selected = node.multisampleRasterizationSamples == optionIndex;
                    if (ImGui::Selectable(vkSampleCountOptionName(optionIndex), selected)) {
                        node.multisampleRasterizationSamples = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineDepthStencilStateContent(ImDrawList* drawList, const GraphNode& node,
                                                    const NodeScreenLayout& layout, const NodeTheme& theme,
                                                    const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            static const char* kParamLabels[kVkPipelineDepthStencilStateParamCount] = {
                "sType",
                "depthTestEnable",
                "depthWriteEnable",
                "depthCompareOp",
                "depthBoundsTestEnable",
                "stencilTestEnable",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkPipelineDepthStencilStateParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                switch (rowIndex) {
                    case 0:
                        std::snprintf(valueText, sizeof(valueText), "%s", kVkPipelineDepthStencilStateSType);
                        break;
                    case 1:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.depthStencilDepthTestEnable));
                        break;
                    case 2:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.depthStencilDepthWriteEnable));
                        break;
                    case 3:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkCompareOpOptionName(node.depthStencilDepthCompareOp));
                        break;
                    case 4:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.depthStencilDepthBoundsTestEnable));
                        break;
                    default:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.depthStencilStencilTestEnable));
                        break;
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkPipelineDepthStencilStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                    bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineDepthStencilStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineDepthStencilStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            drawVkBool32Combo("##depthTestEnable", node.depthStencilDepthTestEnable, nodeParamRowCenterY(layout, 1),
                              fieldLayout, interactive, blockGraphDrag);
            drawVkBool32Combo("##depthWriteEnable", node.depthStencilDepthWriteEnable, nodeParamRowCenterY(layout, 2),
                              fieldLayout, interactive, blockGraphDrag);

            const float compareOpRowY = nodeParamRowCenterY(layout, 3) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, compareOpRowY));
            if (beginNodeCombo("##depthCompareOp", vkCompareOpOptionName(node.depthStencilDepthCompareOp),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkCompareOpOptionCount; ++optionIndex) {
                    const bool selected = node.depthStencilDepthCompareOp == optionIndex;
                    if (ImGui::Selectable(vkCompareOpOptionName(optionIndex), selected)) {
                        node.depthStencilDepthCompareOp = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            drawVkBool32Combo("##depthBoundsTestEnable", node.depthStencilDepthBoundsTestEnable,
                              nodeParamRowCenterY(layout, 4), fieldLayout, interactive, blockGraphDrag);
            drawVkBool32Combo("##stencilTestEnable", node.depthStencilStencilTestEnable, nodeParamRowCenterY(layout, 5),
                              fieldLayout, interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawGraphNodeInputPinRow(ImDrawList* drawList, const GraphNode& node, int pinIndex,
                                      const NodeScreenLayout& layout, const NodeTheme& theme,
                                      const GraphPanelState& panel, const PinHit& hoveredPin, float labelPadX,
                                      ImU32 pinColor) {
            NodeInputPinInfo pinInfo{};
            if (!graphNodeGetInputPin(node, pinIndex, pinInfo)) {
                return;
            }

            const int bodyRow = graphNodeInputPinBodyRow(node, pinIndex);
            const float rowCenterY = layout.topLeft.y + layout.headerHeight + (bodyRow + 0.5f) * layout.pinRowHeight;
            const bool highlighted = shouldHighlightPin(node.id, pinIndex, true);
            drawPin(drawList, ImVec2(layout.topLeft.x, rowCenterY), layout.zoom, pinColor, highlighted);
            drawScaledText(drawList,
                           ImVec2(layout.topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                           pinInfo.label, layout.fontSize);
        }

        void drawNodeInputPinRow(ImDrawList* drawList, const GraphNode& node, int pinIndex, const NodeScreenLayout& layout,
                                 const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin,
                                 float labelPadX, ImU32 pinColor) {
            drawGraphNodeInputPinRow(drawList, node, pinIndex, layout, theme, panel, hoveredPin, labelPadX, pinColor);
        }

        void drawSingleOutputPin(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                 const GraphPanelState& panel, const PinHit& hoveredPin, ImU32 pinColor) {
            const float bodyCenterY =
                layout.topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(layout.bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        struct VertexRowLayout {
            float rowCenterY = 0.f;
            float deleteButtonCenterX = 0.f;
            float deleteButtonRadius = 0.f;
            float nameFieldX = 0.f;
            float nameFieldWidth = 0.f;
            float channelFieldWidth = 0.f;
            float channelFieldGap = 0.f;
            float firstChannelX = 0.f;
            float addChannelCenterX = 0.f;
            float removeChannelCenterX = 0.f;
            float actionButtonRadius = 0.f;
            float fieldHeight = 0.f;
        };

        VertexRowLayout buildVertexRowLayout(const NodeScreenLayout& layout, int rowIndex) {
            VertexRowLayout result{};
            result.rowCenterY = nodeParamRowCenterY(layout, rowIndex);
            result.fieldHeight = layout.fontSize + 4.f * layout.zoom;
            result.deleteButtonRadius = 7.f * layout.zoom;
            result.actionButtonRadius = 7.f * layout.zoom;

            const float leftPad = 8.f * layout.zoom;
            const float rightPad = 10.f * layout.zoom;
            const float nameGap = 4.f * layout.zoom;
            const float channelGap = 2.f * layout.zoom;
            const float channelActionGap = 14.f * layout.zoom;
            const float actionGap = 3.f * layout.zoom;
            const float actionButtonDiameter = result.actionButtonRadius * 2.f;

            result.nameFieldWidth = std::max(48.f * layout.zoom, layout.fontSize * 4.2f);
            result.channelFieldGap = channelGap;
            result.deleteButtonCenterX = layout.topLeft.x + leftPad + result.deleteButtonRadius;
            result.nameFieldX = layout.topLeft.x + leftPad + result.deleteButtonRadius * 2.f + nameGap;
            result.firstChannelX = result.nameFieldX + result.nameFieldWidth + nameGap;
            result.removeChannelCenterX = layout.bottomRight.x - rightPad - result.actionButtonRadius;
            result.addChannelCenterX = result.removeChannelCenterX - actionButtonDiameter - actionGap;

            const float channelsAreaRight =
                result.addChannelCenterX - result.actionButtonRadius - channelActionGap;
            result.channelFieldWidth =
                std::max(28.f * layout.zoom,
                         (channelsAreaRight - result.firstChannelX - channelGap * 3.f) / 4.f);
            return result;
        }

        void formatVertexAttributePreview(const VertexAttribute& attribute, char* outText, int outTextSize) {
            outText[0] = '(';
            int offset = 1;
            for (int channelIndex = 0; channelIndex < attribute.channelCount; ++channelIndex) {
                if (channelIndex > 0) {
                    offset += std::snprintf(outText + offset, outTextSize - offset, ", ");
                }
                offset += std::snprintf(outText + offset, outTextSize - offset, "%.1f", attribute.values[channelIndex]);
            }
            std::snprintf(outText + offset, outTextSize - offset, ")");
        }

        bool vertexCircleButton(const char* id, ImVec2 center, float radius, char glyph, bool enabled, ImU32 fillColor,
                                bool interactive, bool& blockGraphDrag) {
            ImGui::SetCursorScreenPos(ImVec2(center.x - radius, center.y - radius));
            ImGui::PushID(id);
            ImGui::InvisibleButton("##circle", ImVec2(radius * 2.f, radius * 2.f));
            const bool hovered = enabled && ImGui::IsItemHovered();
            const bool clicked = enabled && interactive && ImGui::IsItemClicked();
            trackWidgetInteraction(interactive, blockGraphDrag);
            ImGui::PopID();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImU32 background =
                enabled ? (hovered ? IM_COL32(95, 100, 118, 255) : fillColor) : IM_COL32(52, 52, 60, 180);
            drawList->AddCircleFilled(center, radius, background);
            drawList->AddCircle(center, radius, IM_COL32(120, 125, 140, enabled ? 255 : 120), 0, 1.f);

            if (glyph != '\0') {
                char text[2] = {glyph, '\0'};
                const ImVec2 textSize = ImGui::CalcTextSize(text);
                drawList->AddText(ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
                                  IM_COL32(235, 235, 245, enabled ? 255 : 120), text);
            }
            return clicked;
        }

        void drawVertexContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                               const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& bottomRight = layout.bottomRight;

            constexpr float kMinWidgetZoom = 0.35f;
            const bool drawStaticPreview = layout.zoom < kMinWidgetZoom;
            for (int rowIndex = 0; rowIndex < static_cast<int>(node.vertexAttributes.size()); ++rowIndex) {
                if (!drawStaticPreview) {
                    continue;
                }

                const VertexAttribute& attribute = node.vertexAttributes[static_cast<size_t>(rowIndex)];
                const VertexRowLayout rowLayout = buildVertexRowLayout(layout, rowIndex);

                if (rowIndex > 0) {
                    drawList->AddCircleFilled(ImVec2(rowLayout.deleteButtonCenterX, rowLayout.rowCenterY),
                                              rowLayout.deleteButtonRadius, IM_COL32(72, 76, 90, 255));
                    drawList->AddCircle(ImVec2(rowLayout.deleteButtonCenterX, rowLayout.rowCenterY),
                                        rowLayout.deleteButtonRadius, IM_COL32(120, 125, 140, 255), 0, 1.f);
                }

                drawScaledText(drawList,
                               ImVec2(rowLayout.nameFieldX, rowLayout.rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, attribute.name, layout.fontSize);

                char valueText[128];
                formatVertexAttributePreview(attribute, valueText, sizeof(valueText));
                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const float valueRightX =
                    rowLayout.addChannelCenterX - rowLayout.actionButtonRadius - 14.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowLayout.rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const int addItemRowIndex = static_cast<int>(node.vertexAttributes.size());
            const VertexRowLayout addItemRowLayout = buildVertexRowLayout(layout, addItemRowIndex);
            if (drawStaticPreview) {
                drawList->AddCircleFilled(ImVec2(addItemRowLayout.deleteButtonCenterX, addItemRowLayout.rowCenterY),
                                          addItemRowLayout.actionButtonRadius, IM_COL32(62, 102, 152, 255));
                drawList->AddCircle(ImVec2(addItemRowLayout.deleteButtonCenterX, addItemRowLayout.rowCenterY),
                                    addItemRowLayout.actionButtonRadius, IM_COL32(120, 125, 140, 255), 0, 1.f);
                const ImVec2 addGlyphSize = ImGui::CalcTextSize("+");
                drawList->AddText(ImVec2(addItemRowLayout.deleteButtonCenterX - addGlyphSize.x * 0.5f,
                                         addItemRowLayout.rowCenterY - addGlyphSize.y * 0.5f),
                                  IM_COL32(235, 235, 245, 255), "+");
            }

            for (int pinIndex = 0; pinIndex < kVertexOutputPinCount; ++pinIndex) {
                const int bodyRow = addItemRowIndex + kVertexAddItemRowCount + pinIndex;
                const float rowCenterY =
                    layout.topLeft.y + layout.headerHeight + (bodyRow + 0.5f) * layout.pinRowHeight;
                drawScaledText(drawList,
                               ImVec2(addItemRowLayout.nameFieldX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               nodeOutputPinLabel(node.type, pinIndex), layout.fontSize);

                const bool highlighted = shouldHighlightPin(node.id, pinIndex, false);
                drawPin(drawList, ImVec2(bottomRight.x, rowCenterY), layout.zoom, pinColor, highlighted);
            }
        }

        void drawVertexWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                               bool& blockGraphDrag) {
            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f * layout.zoom, 1.f * layout.zoom));

            for (int rowIndex = 0; rowIndex < static_cast<int>(node.vertexAttributes.size()); ++rowIndex) {
                VertexAttribute& attribute = node.vertexAttributes[static_cast<size_t>(rowIndex)];
                const VertexRowLayout rowLayout = buildVertexRowLayout(layout, rowIndex);
                const bool canDeleteItem = rowIndex > 0;

                ImGui::PushID(rowIndex);
                if (canDeleteItem) {
                    if (vertexCircleButton("##deleteItem", ImVec2(rowLayout.deleteButtonCenterX, rowLayout.rowCenterY),
                                           rowLayout.deleteButtonRadius, '-', true, IM_COL32(72, 76, 90, 255),
                                           interactive, blockGraphDrag)) {
                        node.vertexAttributes.erase(node.vertexAttributes.begin() + rowIndex);
                        ImGui::PopID();
                        ImGui::PopStyleVar();
                        ImGui::PopFont();
                        return;
                    }
                }

                const float nameRowY = rowLayout.rowCenterY - rowLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(rowLayout.nameFieldX, nameRowY));
                ImGui::SetNextItemWidth(rowLayout.nameFieldWidth);
                ImGui::InputText("##name", attribute.name, sizeof(attribute.name));
                trackWidgetInteraction(interactive, blockGraphDrag);

                for (int channelIndex = 0; channelIndex < attribute.channelCount; ++channelIndex) {
                    const float channelX =
                        rowLayout.firstChannelX + channelIndex * (rowLayout.channelFieldWidth + rowLayout.channelFieldGap);
                    ImGui::SetCursorScreenPos(ImVec2(channelX, nameRowY));
                    ImGui::SetNextItemWidth(rowLayout.channelFieldWidth);
                    ImGui::PushID(channelIndex);
                    ImGui::InputFloat("##channel", &attribute.values[channelIndex], 0.f, 0.f, "%.1f");
                    trackWidgetInteraction(interactive, blockGraphDrag);
                    ImGui::PopID();
                }

                const bool canAddChannel = attribute.channelCount < kMaxVertexAttributeChannels;
                const bool canRemoveChannel = attribute.channelCount > kMinVertexAttributeChannels;
                if (vertexCircleButton("##addChannel", ImVec2(rowLayout.addChannelCenterX, rowLayout.rowCenterY),
                                       rowLayout.actionButtonRadius, '+', canAddChannel, IM_COL32(62, 102, 152, 255),
                                       interactive, blockGraphDrag) &&
                    canAddChannel) {
                    ++attribute.channelCount;
                }
                if (vertexCircleButton("##removeChannel", ImVec2(rowLayout.removeChannelCenterX, rowLayout.rowCenterY),
                                       rowLayout.actionButtonRadius, '-', canRemoveChannel, IM_COL32(72, 76, 90, 255),
                                       interactive, blockGraphDrag) &&
                    canRemoveChannel) {
                    --attribute.channelCount;
                }
                ImGui::PopID();
            }

            const int addItemRowIndex = static_cast<int>(node.vertexAttributes.size());
            const VertexRowLayout addItemRowLayout = buildVertexRowLayout(layout, addItemRowIndex);
            if (vertexCircleButton("##addItem", ImVec2(addItemRowLayout.deleteButtonCenterX, addItemRowLayout.rowCenterY),
                                   addItemRowLayout.actionButtonRadius, '+', true, IM_COL32(62, 102, 152, 255),
                                   interactive, blockGraphDrag)) {
                VertexAttribute attribute{};
                std::snprintf(attribute.name, sizeof(attribute.name), "attr%d", addItemRowIndex);
                attribute.channelCount = 1;
                node.vertexAttributes.push_back(attribute);
            }

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineColorBlendStateContent(ImDrawList* drawList, const GraphNode& node,
                                                  const NodeScreenLayout& layout, const NodeTheme& theme,
                                                  const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;

            static const char* kParamLabels[kVkPipelineColorBlendStateParamCount] = {
                "sType",
                "logicOpEnable",
                "logicOp",
                "blendConstants R",
                "blendConstants G",
                "blendConstants B",
                "blendConstants A",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkPipelineColorBlendStateParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                switch (rowIndex) {
                    case 0:
                        std::snprintf(valueText, sizeof(valueText), "%s", kVkPipelineColorBlendStateSType);
                        break;
                    case 1:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkBool32OptionName(node.colorBlendLogicOpEnable));
                        break;
                    case 2:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkLogicOpOptionName(node.colorBlendLogicOp));
                        break;
                    case 3:
                        std::snprintf(valueText, sizeof(valueText), "%.1f", node.colorBlendConstantR);
                        break;
                    case 4:
                        std::snprintf(valueText, sizeof(valueText), "%.1f", node.colorBlendConstantG);
                        break;
                    case 5:
                        std::snprintf(valueText, sizeof(valueText), "%.1f", node.colorBlendConstantB);
                        break;
                    default:
                        std::snprintf(valueText, sizeof(valueText), "%.1f", node.colorBlendConstantA);
                        break;
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkPipelineColorBlendStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                  bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineColorBlendStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineColorBlendStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            drawVkBool32Combo("##logicOpEnable", node.colorBlendLogicOpEnable, nodeParamRowCenterY(layout, 1),
                              fieldLayout, interactive, blockGraphDrag);

            const float logicOpRowY = nodeParamRowCenterY(layout, 2) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, logicOpRowY));
            if (beginNodeCombo("##logicOp", vkLogicOpOptionName(node.colorBlendLogicOp), fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkLogicOpOptionCount; ++optionIndex) {
                    const bool selected = node.colorBlendLogicOp == optionIndex;
                    if (ImGui::Selectable(vkLogicOpOptionName(optionIndex), selected)) {
                        node.colorBlendLogicOp = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            float* blendConstants[] = {&node.colorBlendConstantR, &node.colorBlendConstantG, &node.colorBlendConstantB,
                                       &node.colorBlendConstantA};
            static const char* blendConstantIds[] = {"##blendConstantR", "##blendConstantG", "##blendConstantB",
                                                     "##blendConstantA"};
            for (int channelIndex = 0; channelIndex < 4; ++channelIndex) {
                const float rowY = nodeParamRowCenterY(layout, 3 + channelIndex) - fieldLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
                ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
                ImGui::InputFloat(blendConstantIds[channelIndex], blendConstants[channelIndex], 0.f, 0.f, "%.1f");
                trackWidgetInteraction(interactive, blockGraphDrag);
            }

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineColorBlendAttachmentStateContent(ImDrawList* drawList, const GraphNode& node,
                                                              const NodeScreenLayout& layout, const NodeTheme& theme,
                                                              const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;

            const float blendEnableRowY = nodeParamRowCenterY(layout, 0);
            drawScaledText(drawList,
                           ImVec2(topLeft.x + labelPadX, blendEnableRowY - layout.fontSize * 0.5f), theme.pinLabel,
                           "blendEnable", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            if (layout.zoom < kMinWidgetZoom) {
                const char* valueText = vkBool32OptionName(node.colorBlendAttachmentBlendEnable);
                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX =
                    fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList,
                               ImVec2(valueRightX - textSize.x, blendEnableRowY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkPipelineColorBlendAttachmentStateWidgets(GraphNode& node, const NodeScreenLayout& layout,
                                                            bool interactive, bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            drawVkBool32Combo("##blendEnable", node.colorBlendAttachmentBlendEnable, nodeParamRowCenterY(layout, 0),
                              fieldLayout, interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkColorWriteMaskContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                         const NodeTheme& theme, const GraphPanelState& panel,
                                         const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            for (int pinIndex = 0; pinIndex < kVkColorWriteMaskOutputPinCount; ++pinIndex) {
                const float rowCenterY = topLeft.y + layout.headerHeight + (pinIndex + 0.5f) * layout.pinRowHeight;
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               nodeOutputPinLabel(node.type, pinIndex), layout.fontSize);

                const bool highlighted = shouldHighlightPin(node.id, pinIndex, false);
                drawPin(drawList, ImVec2(bottomRight.x, rowCenterY), layout.zoom, pinColor, highlighted);
            }
        }

        void drawVkDynamicStateContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                       const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            for (int pinIndex = 0; pinIndex < kVkDynamicStateOutputPinCount; ++pinIndex) {
                const float rowCenterY = topLeft.y + layout.headerHeight + (pinIndex + 0.5f) * layout.pinRowHeight;
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               nodeOutputPinLabel(node.type, pinIndex), layout.fontSize);

                const bool highlighted = shouldHighlightPin(node.id, pinIndex, false);
                drawPin(drawList, ImVec2(bottomRight.x, rowCenterY), layout.zoom, pinColor, highlighted);
            }
        }

        void drawVkPipelineDynamicStateContent(ImDrawList* drawList, const GraphNode& node,
                                               const NodeScreenLayout& layout, const NodeTheme& theme,
                                               const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;

            const float sTypeRowY = nodeParamRowCenterY(layout, 0);
            drawScaledText(drawList, ImVec2(topLeft.x + labelPadX, sTypeRowY - layout.fontSize * 0.5f), theme.pinLabel,
                           "sType", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            if (layout.zoom < kMinWidgetZoom) {
                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f,
                                                                        kVkPipelineDynamicStateSType);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.sTypeFieldX + fieldLayout.sTypeFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, sTypeRowY - layout.fontSize * 0.5f),
                               theme.pinLabel, kVkPipelineDynamicStateSType, layout.fontSize);
            }

            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkPipelineDynamicStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                               bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkPipelineDynamicStateSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkPipelineDynamicStateSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawSTypeParamRow(ImDrawList* drawList, const NodeScreenLayout& layout, const NodeTheme& theme,
                               float labelPadX, const char* sTypeValue) {
            const float sTypeRowY = nodeParamRowCenterY(layout, 0);
            drawScaledText(drawList, ImVec2(layout.topLeft.x + labelPadX, sTypeRowY - layout.fontSize * 0.5f),
                           theme.pinLabel, "sType", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            if (layout.zoom >= kMinWidgetZoom) {
                return;
            }

            const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, sTypeValue);
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
            const float valueRightX = fieldLayout.sTypeFieldX + fieldLayout.sTypeFieldWidth - 4.f * layout.zoom;
            drawScaledText(drawList, ImVec2(valueRightX - textSize.x, sTypeRowY - layout.fontSize * 0.5f), theme.pinLabel,
                           sTypeValue, layout.fontSize);
        }

        void drawSTypeParamWidget(const char* sTypeValue, const NodeScreenLayout& layout, bool interactive,
                                  bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", sTypeValue);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", sTypeValue);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkRenderPassContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                     const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            const float headerRowY = nodeParamRowCenterY(layout, 0);
            drawScaledText(drawList, ImVec2(layout.topLeft.x + labelPadX, headerRowY - layout.fontSize * 0.5f),
                           theme.pinLabel, "VkAttachmentDescription", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            const bool drawStaticPreview = layout.zoom < kMinWidgetZoom;
            for (int slotIndex = 0; slotIndex < node.renderPassAttachmentSlotCount; ++slotIndex) {
                const int rowIndex = kVkRenderPassAttachmentHeaderRowCount + slotIndex;
                const VertexRowLayout rowLayout = buildVertexRowLayout(layout, rowIndex);
                const bool canDelete = node.renderPassAttachmentSlotCount > 1;
                const float rowCenterY = rowLayout.rowCenterY;

                if (drawStaticPreview && canDelete) {
                    drawList->AddCircleFilled(ImVec2(rowLayout.deleteButtonCenterX, rowCenterY),
                                              rowLayout.deleteButtonRadius, IM_COL32(72, 76, 90, 255));
                    drawList->AddCircle(ImVec2(rowLayout.deleteButtonCenterX, rowCenterY), rowLayout.deleteButtonRadius,
                                        IM_COL32(120, 125, 140, 255), 0, 1.f);
                }

                const bool highlighted = shouldHighlightPin(node.id, slotIndex, true);
                drawPin(drawList, ImVec2(layout.topLeft.x, rowCenterY), layout.zoom, pinColor, highlighted);
                drawScaledText(drawList, ImVec2(rowLayout.nameFieldX, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, "attachment", layout.fontSize);
            }

            const int addRowIndex = kVkRenderPassAttachmentHeaderRowCount + node.renderPassAttachmentSlotCount;
            const VertexRowLayout addRowLayout = buildVertexRowLayout(layout, addRowIndex);
            if (drawStaticPreview) {
                drawList->AddCircleFilled(ImVec2(addRowLayout.deleteButtonCenterX, addRowLayout.rowCenterY),
                                          addRowLayout.actionButtonRadius, IM_COL32(62, 102, 152, 255));
                drawList->AddCircle(ImVec2(addRowLayout.deleteButtonCenterX, addRowLayout.rowCenterY),
                                    addRowLayout.actionButtonRadius, IM_COL32(120, 125, 140, 255), 0, 1.f);
                const ImVec2 addGlyphSize = ImGui::CalcTextSize("+");
                drawList->AddText(ImVec2(addRowLayout.deleteButtonCenterX - addGlyphSize.x * 0.5f,
                                         addRowLayout.rowCenterY - addGlyphSize.y * 0.5f),
                                  IM_COL32(235, 235, 245, 255), "+");
            }

            drawGraphNodeInputPinRow(drawList, node, node.renderPassAttachmentSlotCount, layout, theme, panel,
                                     hoveredPin, labelPadX, pinColor);
            drawGraphNodeInputPinRow(drawList, node, node.renderPassAttachmentSlotCount + 1, layout, theme, panel,
                                     hoveredPin, labelPadX, pinColor);

            const float bodyCenterY =
                layout.topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(layout.bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkRenderPassWidgets(GraphDocument& document, GraphNode& node, const NodeScreenLayout& layout,
                                     bool interactive, bool& blockGraphDrag) {
            for (int slotIndex = 0; slotIndex < node.renderPassAttachmentSlotCount; ++slotIndex) {
                const int rowIndex = kVkRenderPassAttachmentHeaderRowCount + slotIndex;
                const VertexRowLayout rowLayout = buildVertexRowLayout(layout, rowIndex);
                const bool canDelete = node.renderPassAttachmentSlotCount > 1;

                ImGui::PushID(slotIndex);
                if (canDelete) {
                    if (vertexCircleButton("##deleteAttachment",
                                           ImVec2(rowLayout.deleteButtonCenterX, rowLayout.rowCenterY),
                                           rowLayout.deleteButtonRadius, '-', true, IM_COL32(72, 76, 90, 255),
                                           interactive, blockGraphDrag)) {
                        document.removeRenderPassAttachmentSlot(node.id, slotIndex);
                        ImGui::PopID();
                        return;
                    }
                }
                ImGui::PopID();
            }

            const int addRowIndex = kVkRenderPassAttachmentHeaderRowCount + node.renderPassAttachmentSlotCount;
            const VertexRowLayout addRowLayout = buildVertexRowLayout(layout, addRowIndex);
            if (vertexCircleButton("##addAttachment", ImVec2(addRowLayout.deleteButtonCenterX, addRowLayout.rowCenterY),
                                   addRowLayout.actionButtonRadius, '+', true, IM_COL32(62, 102, 152, 255), interactive,
                                   blockGraphDrag)) {
                document.addRenderPassAttachmentSlot(node.id);
            }
        }

        void drawVkAttachmentDescriptionContent(ImDrawList* drawList, const GraphNode& node,
                                                const NodeScreenLayout& layout, const NodeTheme& theme,
                                                const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            static const char* kParamLabels[kVkAttachmentDescriptionParamCount] = {
                "format", "samples", "loadOp", "storeOp", "stencilLoadOp", "stencilStoreOp", "initialLayout",
                "finalLayout",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkAttachmentDescriptionParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(layout.topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                switch (rowIndex) {
                    case 0:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.attachmentDescriptionFormat);
                        break;
                    case 1:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkSampleCountOptionName(node.attachmentDescriptionSamples));
                        break;
                    case 2:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkAttachmentLoadOpOptionName(node.attachmentDescriptionLoadOp));
                        break;
                    case 3:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkAttachmentStoreOpOptionName(node.attachmentDescriptionStoreOp));
                        break;
                    case 4:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkAttachmentLoadOpOptionName(node.attachmentDescriptionStencilLoadOp));
                        break;
                    case 5:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkAttachmentStoreOpOptionName(node.attachmentDescriptionStencilStoreOp));
                        break;
                    case 6:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkImageLayoutOptionName(node.attachmentDescriptionInitialLayout));
                        break;
                    default:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkImageLayoutOptionName(node.attachmentDescriptionFinalLayout));
                        break;
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkAttachmentDescriptionWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float formatRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, formatRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputInt("##format", &node.attachmentDescriptionFormat, 0, 0);
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float samplesRowY = nodeParamRowCenterY(layout, 1) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, samplesRowY));
            if (beginNodeCombo("##samples", vkSampleCountOptionName(node.attachmentDescriptionSamples),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkSampleCountOptionCount; ++optionIndex) {
                    const bool selected = node.attachmentDescriptionSamples == optionIndex;
                    if (ImGui::Selectable(vkSampleCountOptionName(optionIndex), selected)) {
                        node.attachmentDescriptionSamples = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            int* loadStoreIndices[] = {&node.attachmentDescriptionLoadOp, &node.attachmentDescriptionStoreOp,
                                       &node.attachmentDescriptionStencilLoadOp,
                                       &node.attachmentDescriptionStencilStoreOp};
            for (int fieldIndex = 0; fieldIndex < 4; ++fieldIndex) {
                const float rowY = nodeParamRowCenterY(layout, 2 + fieldIndex) - fieldLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
                ImGui::PushID(fieldIndex);
                const bool isLoadOp = fieldIndex == 0 || fieldIndex == 2;
                const int optionCount =
                    isLoadOp ? kVkAttachmentLoadOpOptionCount : kVkAttachmentStoreOpOptionCount;
                const char* preview =
                    isLoadOp ? vkAttachmentLoadOpOptionName(*loadStoreIndices[fieldIndex])
                             : vkAttachmentStoreOpOptionName(*loadStoreIndices[fieldIndex]);
                if (beginNodeCombo("##loadStore", preview, fieldLayout.comboFieldWidth)) {
                    for (int optionIndex = 0; optionIndex < optionCount; ++optionIndex) {
                        const bool selected = *loadStoreIndices[fieldIndex] == optionIndex;
                        const char* label = isLoadOp ? vkAttachmentLoadOpOptionName(optionIndex)
                                                     : vkAttachmentStoreOpOptionName(optionIndex);
                        if (ImGui::Selectable(label, selected)) {
                            *loadStoreIndices[fieldIndex] = optionIndex;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                trackWidgetInteraction(interactive, blockGraphDrag);
                ImGui::PopID();
            }

            int* layoutIndices[] = {&node.attachmentDescriptionInitialLayout, &node.attachmentDescriptionFinalLayout};
            for (int fieldIndex = 0; fieldIndex < 2; ++fieldIndex) {
                const float rowY = nodeParamRowCenterY(layout, 6 + fieldIndex) - fieldLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
                ImGui::PushID(fieldIndex + 4);
                if (beginNodeCombo("##layout", vkImageLayoutOptionName(*layoutIndices[fieldIndex]),
                                   fieldLayout.comboFieldWidth)) {
                    for (int optionIndex = 0; optionIndex < kVkImageLayoutOptionCount; ++optionIndex) {
                        const bool selected = *layoutIndices[fieldIndex] == optionIndex;
                        if (ImGui::Selectable(vkImageLayoutOptionName(optionIndex), selected)) {
                            *layoutIndices[fieldIndex] = optionIndex;
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                trackWidgetInteraction(interactive, blockGraphDrag);
                ImGui::PopID();
            }

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkSubpassDescriptionContent(ImDrawList* drawList, const GraphNode& node,
                                             const NodeScreenLayout& layout, const NodeTheme& theme,
                                             const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            const float bindPointRowY = nodeParamRowCenterY(layout, 0);
            drawScaledText(drawList, ImVec2(layout.topLeft.x + labelPadX, bindPointRowY - layout.fontSize * 0.5f),
                           theme.pinLabel, "pipelineBindPoint", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            if (layout.zoom < kMinWidgetZoom) {
                const char* valueText = vkPipelineBindPointOptionName(node.subpassDescriptionPipelineBindPoint);
                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, bindPointRowY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            for (int pinIndex = 0; pinIndex < kVkSubpassDescriptionInputPinCount; ++pinIndex) {
                drawNodeInputPinRow(drawList, node, pinIndex, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            }
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkSubpassDescriptionWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                             bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float rowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
            if (beginNodeCombo("##pipelineBindPoint",
                               vkPipelineBindPointOptionName(node.subpassDescriptionPipelineBindPoint),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkPipelineBindPointOptionCount; ++optionIndex) {
                    const bool selected = node.subpassDescriptionPipelineBindPoint == optionIndex;
                    if (ImGui::Selectable(vkPipelineBindPointOptionName(optionIndex), selected)) {
                        node.subpassDescriptionPipelineBindPoint = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkSubpassDependencyContent(ImDrawList* drawList, const GraphNode& node,
                                            const NodeScreenLayout& layout, const NodeTheme& theme,
                                            const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            static const char* kParamLabels[kVkSubpassDependencyParamCount] = {
                "srcSubpass", "dstSubpass", "srcStageMask", "srcAccessMask", "dstStageMask", "dstAccessMask",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkSubpassDependencyParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(layout.topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                const int* intFields[] = {
                    &node.subpassDependencySrcSubpass, &node.subpassDependencyDstSubpass,
                    &node.subpassDependencySrcStageMask, &node.subpassDependencySrcAccessMask,
                    &node.subpassDependencyDstStageMask, &node.subpassDependencyDstAccessMask,
                };
                if (rowIndex == 0 && *intFields[0] < 0) {
                    std::snprintf(valueText, sizeof(valueText), "%s", "VK_SUBPASS_EXTERNAL");
                } else if (rowIndex >= 2) {
                    std::snprintf(valueText, sizeof(valueText), "0x%08X", *intFields[rowIndex]);
                } else {
                    std::snprintf(valueText, sizeof(valueText), "%d", *intFields[rowIndex]);
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkSubpassDependencyWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                            bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            int* intFields[] = {&node.subpassDependencySrcSubpass, &node.subpassDependencyDstSubpass,
                                &node.subpassDependencySrcStageMask, &node.subpassDependencySrcAccessMask,
                                &node.subpassDependencyDstStageMask, &node.subpassDependencyDstAccessMask};
            static const char* fieldIds[] = {"##srcSubpass", "##dstSubpass", "##srcStageMask", "##srcAccessMask",
                                               "##dstStageMask", "##dstAccessMask"};
            for (int rowIndex = 0; rowIndex < kVkSubpassDependencyParamCount; ++rowIndex) {
                const float rowY = nodeParamRowCenterY(layout, rowIndex) - fieldLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
                ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
                ImGui::InputInt(fieldIds[rowIndex], intFields[rowIndex], 0, 0);
                trackWidgetInteraction(interactive, blockGraphDrag);
            }

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkAttachmentReferenceContent(ImDrawList* drawList, const GraphNode& node,
                                              const NodeScreenLayout& layout, const NodeTheme& theme,
                                              const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            static const char* kParamLabels[kVkAttachmentReferenceParamCount] = {"attachment", "layout"};

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkAttachmentReferenceParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(layout.topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                if (rowIndex == 0) {
                    std::snprintf(valueText, sizeof(valueText), "%d", node.attachmentReferenceAttachment);
                } else {
                    std::snprintf(valueText, sizeof(valueText), "%s",
                                  vkImageLayoutOptionName(node.attachmentReferenceLayout));
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkAttachmentReferenceWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                              bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float attachmentRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, attachmentRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputInt("##attachment", &node.attachmentReferenceAttachment, 0, 0);
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float layoutRowY = nodeParamRowCenterY(layout, 1) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, layoutRowY));
            if (beginNodeCombo("##layout", vkImageLayoutOptionName(node.attachmentReferenceLayout),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkImageLayoutOptionCount; ++optionIndex) {
                    const bool selected = node.attachmentReferenceLayout == optionIndex;
                    if (ImGui::Selectable(vkImageLayoutOptionName(optionIndex), selected)) {
                        node.attachmentReferenceLayout = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkImageViewContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                    const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;

            drawSTypeParamRow(drawList, layout, theme, labelPadX, kVkImageViewSType);
            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);

            static const char* kSuffixLabels[kVkImageViewSuffixParamCount] = {
                "viewType", "format", "aspectMask", "baseMipLevel", "levelCount", "baseArrayLayer", "layerCount",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            const int suffixStartRow = kVkImageViewPrefixParamCount + kVkImageViewInputPinCount;
            for (int suffixIndex = 0; suffixIndex < kVkImageViewSuffixParamCount; ++suffixIndex) {
                const int rowIndex = suffixStartRow + suffixIndex;
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kSuffixLabels[suffixIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                switch (suffixIndex) {
                    case 0:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkImageViewTypeOptionName(node.imageViewViewType));
                        break;
                    case 1:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.imageViewFormat);
                        break;
                    case 2:
                        std::snprintf(valueText, sizeof(valueText), "0x%08X", node.imageViewAspectMask);
                        break;
                    case 3:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.imageViewBaseMipLevel);
                        break;
                    case 4:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.imageViewLevelCount);
                        break;
                    case 5:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.imageViewBaseArrayLayer);
                        break;
                    default:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.imageViewLayerCount);
                        break;
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkImageViewWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                    bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            drawSTypeParamWidget(kVkImageViewSType, layout, interactive, blockGraphDrag);

            const int suffixStartRow = kVkImageViewPrefixParamCount + kVkImageViewInputPinCount;
            const float viewTypeRowY = nodeParamRowCenterY(layout, suffixStartRow) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, viewTypeRowY));
            if (beginNodeCombo("##viewType", vkImageViewTypeOptionName(node.imageViewViewType),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkImageViewTypeOptionCount; ++optionIndex) {
                    const bool selected = node.imageViewViewType == optionIndex;
                    if (ImGui::Selectable(vkImageViewTypeOptionName(optionIndex), selected)) {
                        node.imageViewViewType = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            int* intFields[] = {&node.imageViewFormat, &node.imageViewAspectMask, &node.imageViewBaseMipLevel,
                                &node.imageViewLevelCount, &node.imageViewBaseArrayLayer, &node.imageViewLayerCount};
            static const char* fieldIds[] = {"##format", "##aspectMask", "##baseMipLevel", "##levelCount",
                                             "##baseArrayLayer", "##layerCount"};
            for (int fieldIndex = 0; fieldIndex < 6; ++fieldIndex) {
                const float rowY =
                    nodeParamRowCenterY(layout, suffixStartRow + 1 + fieldIndex) - fieldLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
                ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
                ImGui::InputInt(fieldIds[fieldIndex], intFields[fieldIndex], 0, 0);
                trackWidgetInteraction(interactive, blockGraphDrag);
            }

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkFramebufferContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                      const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;

            static const char* kParamLabels[kVkFramebufferParamCount] = {"sType", "width", "height", "layers"};

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkFramebufferParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                if (rowIndex == 0) {
                    std::snprintf(valueText, sizeof(valueText), "%s", kVkFramebufferSType);
                } else if (rowIndex == 1) {
                    std::snprintf(valueText, sizeof(valueText), "%d", node.framebufferWidth);
                } else if (rowIndex == 2) {
                    std::snprintf(valueText, sizeof(valueText), "%d", node.framebufferHeight);
                } else {
                    std::snprintf(valueText, sizeof(valueText), "%d", node.framebufferLayers);
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float fieldWidth =
                    rowIndex == 0 ? fieldLayout.sTypeFieldWidth : fieldLayout.comboFieldWidth;
                const float fieldX = rowIndex == 0 ? fieldLayout.sTypeFieldX : fieldLayout.comboFieldX;
                const float valueRightX = fieldX + fieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            for (int pinIndex = 0; pinIndex < kVkFramebufferInputPinCount; ++pinIndex) {
                drawNodeInputPinRow(drawList, node, pinIndex, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            }
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkFramebufferWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                      bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float sTypeRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.sTypeFieldX, sTypeRowY));
            ImGui::SetNextItemWidth(fieldLayout.sTypeFieldWidth);
            char sTypeText[128];
            std::snprintf(sTypeText, sizeof(sTypeText), "%s", kVkFramebufferSType);
            ImGui::BeginDisabled();
            ImGui::InputText("##sType", sTypeText, sizeof(sTypeText), ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", kVkFramebufferSType);
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            int* intFields[] = {&node.framebufferWidth, &node.framebufferHeight, &node.framebufferLayers};
            static const char* fieldIds[] = {"##width", "##height", "##layers"};
            for (int fieldIndex = 0; fieldIndex < 3; ++fieldIndex) {
                const float rowY = nodeParamRowCenterY(layout, 1 + fieldIndex) - fieldLayout.fieldHeight * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, rowY));
                ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
                ImGui::InputInt(fieldIds[fieldIndex], intFields[fieldIndex], 0, 0);
                trackWidgetInteraction(interactive, blockGraphDrag);
            }

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineVertexInputStateContent(ImDrawList* drawList, const GraphNode& node,
                                                   const NodeScreenLayout& layout, const NodeTheme& theme,
                                                   const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            drawSTypeParamRow(drawList, layout, theme, labelPadX, kVkPipelineVertexInputStateSType);

            for (int pinIndex = 0; pinIndex < kVkPipelineVertexInputStateInputPinCount; ++pinIndex) {
                drawNodeInputPinRow(drawList, node, pinIndex, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            }
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkPipelineVertexInputStateWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                   bool& blockGraphDrag) {
            (void)node;
            drawSTypeParamWidget(kVkPipelineVertexInputStateSType, layout, interactive, blockGraphDrag);
        }

        void drawVkPipelineShaderStageContent(ImDrawList* drawList, const GraphNode& node,
                                              const NodeScreenLayout& layout, const NodeTheme& theme,
                                              const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;

            drawSTypeParamRow(drawList, layout, theme, labelPadX, kVkPipelineShaderStageSType);

            const float stageRowY = nodeParamRowCenterY(layout, 1);
            drawScaledText(drawList, ImVec2(topLeft.x + labelPadX, stageRowY - layout.fontSize * 0.5f), theme.pinLabel,
                           "stage", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            if (layout.zoom < kMinWidgetZoom) {
                const char* stageText = vkShaderStageFlagOptionName(node.shaderStage);
                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, stageText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, stageRowY - layout.fontSize * 0.5f),
                               theme.pinLabel, stageText, layout.fontSize);
            }

            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);

            const int pNameRowIndex =
                kVkPipelineShaderStagePrefixParamCount + kVkPipelineShaderStageInputPinCount;
            const float pNameRowY = nodeParamRowCenterY(layout, pNameRowIndex);
            drawScaledText(drawList, ImVec2(topLeft.x + labelPadX, pNameRowY - layout.fontSize * 0.5f), theme.pinLabel,
                           "pName", layout.fontSize);

            if (layout.zoom < kMinWidgetZoom) {
                const ImVec2 textSize =
                    ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, node.shaderStageEntryName);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, pNameRowY - layout.fontSize * 0.5f),
                               theme.pinLabel, node.shaderStageEntryName, layout.fontSize);
            }

            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkPipelineShaderStageWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                              bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            drawSTypeParamWidget(kVkPipelineShaderStageSType, layout, interactive, blockGraphDrag);

            const float stageRowY = nodeParamRowCenterY(layout, 1) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, stageRowY));
            if (beginNodeCombo("##stage", vkShaderStageFlagOptionName(node.shaderStage), fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkShaderStageFlagOptionCount; ++optionIndex) {
                    const bool selected = node.shaderStage == optionIndex;
                    if (ImGui::Selectable(vkShaderStageFlagOptionName(optionIndex), selected)) {
                        node.shaderStage = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const int pNameRowIndex =
                kVkPipelineShaderStagePrefixParamCount + kVkPipelineShaderStageInputPinCount;
            const float pNameRowY = nodeParamRowCenterY(layout, pNameRowIndex) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, pNameRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputText("##pName", node.shaderStageEntryName, kMaxShaderStageEntryNameLen);
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkShaderModuleContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                       const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            const float pathRowY = nodeParamRowCenterY(layout, 0);
            drawScaledText(drawList, ImVec2(topLeft.x + labelPadX, pathRowY - layout.fontSize * 0.5f), theme.pinLabel,
                           "path", layout.fontSize);

            constexpr float kMinWidgetZoom = 0.35f;
            if (layout.zoom < kMinWidgetZoom && node.shaderModulePath[0] != '\0') {
                const ImVec2 textSize =
                    ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, node.shaderModulePath);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, pathRowY - layout.fontSize * 0.5f),
                               theme.pinLabel, node.shaderModulePath, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkShaderModuleWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                       bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float pathRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, pathRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputText("##shaderModulePath", node.shaderModulePath, kMaxShaderModulePathLen);
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawVkPipelineLayoutContent(ImDrawList* drawList, const GraphNode& node, const NodeScreenLayout& layout,
                                         const NodeTheme& theme, const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            drawSTypeParamRow(drawList, layout, theme, labelPadX, kVkPipelineLayoutSType);
            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkPipelineLayoutWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                         bool& blockGraphDrag) {
            (void)node;
            drawSTypeParamWidget(kVkPipelineLayoutSType, layout, interactive, blockGraphDrag);
        }

        void drawVkDescriptorSetLayoutContent(ImDrawList* drawList, const GraphNode& node,
                                              const NodeScreenLayout& layout, const NodeTheme& theme,
                                              const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            drawSTypeParamRow(drawList, layout, theme, labelPadX, kVkDescriptorSetLayoutSType);
            drawNodeInputPinRow(drawList, node, 0, layout, theme, panel, hoveredPin, labelPadX, pinColor);
            drawSingleOutputPin(drawList, node, layout, panel, hoveredPin, pinColor);
        }

        void drawVkDescriptorSetLayoutWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                              bool& blockGraphDrag) {
            (void)node;
            drawSTypeParamWidget(kVkDescriptorSetLayoutSType, layout, interactive, blockGraphDrag);
        }

        void drawVkDescriptorSetLayoutBindingContent(ImDrawList* drawList, const GraphNode& node,
                                                     const NodeScreenLayout& layout, const NodeTheme& theme,
                                                     const GraphPanelState& panel, const PinHit& hoveredPin) {
            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            static const char* kParamLabels[kVkDescriptorSetLayoutBindingParamCount] = {
                "binding",
                "descriptorCount",
                "descriptorType",
                "pImmutableSamplers",
                "stageFlags",
            };

            constexpr float kMinWidgetZoom = 0.35f;
            for (int rowIndex = 0; rowIndex < kVkDescriptorSetLayoutBindingParamCount; ++rowIndex) {
                const float rowCenterY = nodeParamRowCenterY(layout, rowIndex);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               kParamLabels[rowIndex], layout.fontSize);

                if (layout.zoom >= kMinWidgetZoom) {
                    continue;
                }

                char valueText[128];
                switch (rowIndex) {
                    case 0:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.descriptorSetLayoutBindingBinding);
                        break;
                    case 1:
                        std::snprintf(valueText, sizeof(valueText), "%d", node.descriptorSetLayoutBindingDescriptorCount);
                        break;
                    case 2:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkDescriptorTypeOptionName(node.descriptorSetLayoutBindingDescriptorType));
                        break;
                    case 3:
                        std::snprintf(valueText, sizeof(valueText), "%s", kVkDescriptorSetLayoutBindingNullSampler);
                        break;
                    default:
                        std::snprintf(valueText, sizeof(valueText), "%s",
                                      vkShaderStageFlagOptionName(node.descriptorSetLayoutBindingStageFlags));
                        break;
                }

                const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, valueText);
                const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);
                const float valueRightX = fieldLayout.comboFieldX + fieldLayout.comboFieldWidth - 4.f * layout.zoom;
                drawScaledText(drawList, ImVec2(valueRightX - textSize.x, rowCenterY - layout.fontSize * 0.5f),
                               theme.pinLabel, valueText, layout.fontSize);
            }

            const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            const bool highlighted = shouldHighlightPin(node.id, 0, false);
            drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
        }

        void drawVkDescriptorSetLayoutBindingWidgets(GraphNode& node, const NodeScreenLayout& layout, bool interactive,
                                                       bool& blockGraphDrag) {
            const InputAssemblyFieldLayout fieldLayout = inputAssemblyStateFieldLayout(layout);

            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));

            const float bindingRowY = nodeParamRowCenterY(layout, 0) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, bindingRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputInt("##binding", &node.descriptorSetLayoutBindingBinding, 0, 0);
            if (node.descriptorSetLayoutBindingBinding < 0) {
                node.descriptorSetLayoutBindingBinding = 0;
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float descriptorCountRowY = nodeParamRowCenterY(layout, 1) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, descriptorCountRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            ImGui::InputInt("##descriptorCount", &node.descriptorSetLayoutBindingDescriptorCount, 0, 0);
            if (node.descriptorSetLayoutBindingDescriptorCount < 0) {
                node.descriptorSetLayoutBindingDescriptorCount = 0;
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float descriptorTypeRowY = nodeParamRowCenterY(layout, 2) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, descriptorTypeRowY));
            if (beginNodeCombo("##descriptorType",
                               vkDescriptorTypeOptionName(node.descriptorSetLayoutBindingDescriptorType),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkDescriptorTypeOptionCount; ++optionIndex) {
                    const bool selected = node.descriptorSetLayoutBindingDescriptorType == optionIndex;
                    if (ImGui::Selectable(vkDescriptorTypeOptionName(optionIndex), selected)) {
                        node.descriptorSetLayoutBindingDescriptorType = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float nullSamplerRowY = nodeParamRowCenterY(layout, 3) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, nullSamplerRowY));
            ImGui::SetNextItemWidth(fieldLayout.comboFieldWidth);
            char nullSamplerText[32];
            std::snprintf(nullSamplerText, sizeof(nullSamplerText), "%s", kVkDescriptorSetLayoutBindingNullSampler);
            ImGui::BeginDisabled();
            ImGui::InputText("##pImmutableSamplers", nullSamplerText, sizeof(nullSamplerText),
                             ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            trackWidgetInteraction(interactive, blockGraphDrag);

            const float stageFlagsRowY = nodeParamRowCenterY(layout, 4) - fieldLayout.fieldHeight * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(fieldLayout.comboFieldX, stageFlagsRowY));
            if (beginNodeCombo("##stageFlags",
                               vkShaderStageFlagOptionName(node.descriptorSetLayoutBindingStageFlags),
                               fieldLayout.comboFieldWidth)) {
                for (int optionIndex = 0; optionIndex < kVkShaderStageFlagOptionCount; ++optionIndex) {
                    const bool selected = node.descriptorSetLayoutBindingStageFlags == optionIndex;
                    if (ImGui::Selectable(vkShaderStageFlagOptionName(optionIndex), selected)) {
                        node.descriptorSetLayoutBindingStageFlags = optionIndex;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            trackWidgetInteraction(interactive, blockGraphDrag);

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }

        void drawNodeChrome(ImDrawList* drawList, const ImVec2& topLeft, const ImVec2& bottomRight, float headerHeight,
                            float rounding, const NodeTheme& theme, bool selected) {
            const ImVec2 headerBottomRight(bottomRight.x, topLeft.y + headerHeight);

            if (selected) {
                const float glowPad = 3.f;
                drawList->AddRectFilled(ImVec2(topLeft.x - glowPad, topLeft.y - glowPad),
                                        ImVec2(bottomRight.x + glowPad, bottomRight.y + glowPad),
                                        IM_COL32(232, 196, 118, 36), rounding + glowPad);
            }

            drawList->AddRectFilled(ImVec2(topLeft.x + 1.f, topLeft.y + 2.f),
                                    ImVec2(bottomRight.x + 1.f, bottomRight.y + 2.f), IM_COL32(0, 0, 0, 70), rounding);
            drawList->AddRectFilled(topLeft, bottomRight, theme.body, rounding);
            drawList->AddRectFilled(topLeft, headerBottomRight, theme.header, rounding, ImDrawFlags_RoundCornersTop);
            drawList->AddRectFilled(ImVec2(topLeft.x, topLeft.y + headerHeight - 1.f), headerBottomRight, theme.header);

            const float borderThickness = selected ? 2.5f : 1.2f;
            const ImU32 borderColor = selected ? theme.selectedBorder : theme.border;
            drawList->AddRect(topLeft, bottomRight, borderColor, rounding, 0, borderThickness);
        }

        void drawComfyNode(ImDrawList* drawList, const GraphNode& node, const GridViewState& view,
                           const ImVec2& displaySize, bool selected, const GraphPanelState& panel,
                           const PinHit& hoveredPin) {
            const NodeTheme theme = nodeTheme(node.type);
            const NodeScreenLayout layout = buildNodeScreenLayout(node, view, displaySize);
            const float rounding = kNodeRounding * layout.zoom;
            const ImVec2& topLeft = layout.topLeft;
            const ImVec2& bottomRight = layout.bottomRight;

            drawNodeChrome(drawList, topLeft, bottomRight, layout.headerHeight, rounding, theme, selected);

            const char* title = nodeTypeName(node.type);
            const float titlePadX = 10.f * layout.zoom;
            const float titleY = topLeft.y + (layout.headerHeight - layout.fontSize) * 0.5f;
            drawScaledText(drawList, ImVec2(topLeft.x + titlePadX, titleY), theme.title, title, layout.fontSize);

            const float labelPadX = 14.f * layout.zoom;
            const ImU32 pinColor = IM_COL32(170, 170, 185, 255);

            if (node.type == NodeType::VkPipeline) {
                for (int pinIndex = 0; pinIndex < nodeInputPinCount(node.type); ++pinIndex) {
                    const NodeInputPinDef* pinDef = nodeInputPin(node.type, pinIndex);
                    if (pinDef == nullptr) {
                        continue;
                    }

                    const int bodyRow = nodeInputPinBodyRow(node.type, pinIndex);
                    const float rowCenterY = topLeft.y + layout.headerHeight + (bodyRow + 0.5f) * layout.pinRowHeight;
                    const bool highlighted = shouldHighlightPin(node.id, pinIndex, true);
                    drawPin(drawList, ImVec2(topLeft.x, rowCenterY), layout.zoom, pinColor, highlighted);
                    drawScaledText(drawList,
                                   ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                                   pinDef->label, layout.fontSize);
                }
            }

            if (node.type == NodeType::VkPipeline) {
                const float indexRowCenterY = vkPipelineIndexRowCenterY(layout);
                drawScaledText(drawList,
                               ImVec2(topLeft.x + labelPadX, indexRowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                               "VkRenderPass Index", layout.fontSize);

                constexpr float kMinWidgetZoom = 0.35f;
                if (layout.zoom < kMinWidgetZoom) {
                    char indexText[16];
                    std::snprintf(indexText, sizeof(indexText), "%d", node.renderPassIndex);
                    const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(layout.fontSize, FLT_MAX, 0.f, indexText);
                    drawScaledText(drawList,
                                   ImVec2(bottomRight.x - 10.f * layout.zoom - textSize.x,
                                          indexRowCenterY - layout.fontSize * 0.5f),
                                   theme.pinLabel, indexText, layout.fontSize);
                }
            } else if (node.type == NodeType::VkPipelineInputAssemblyState) {
                drawVkPipelineInputAssemblyStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineShaderStage) {
                drawVkPipelineShaderStageContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkShaderModule) {
                drawVkShaderModuleContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::Vertex) {
                drawVertexContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineVertexInputState) {
                drawVkPipelineVertexInputStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineViewportState) {
                drawVkPipelineViewportStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineRasterizationState) {
                drawVkPipelineRasterizationStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineMultisampleState) {
                drawVkPipelineMultisampleStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineDepthStencilState) {
                drawVkPipelineDepthStencilStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineColorBlendState) {
                drawVkPipelineColorBlendStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineColorBlendAttachmentState) {
                drawVkPipelineColorBlendAttachmentStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkColorWriteMask) {
                drawVkColorWriteMaskContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkDynamicState) {
                drawVkDynamicStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineDynamicState) {
                drawVkPipelineDynamicStateContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkPipelineLayout) {
                drawVkPipelineLayoutContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkDescriptorSetLayout) {
                drawVkDescriptorSetLayoutContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkDescriptorSetLayoutBinding) {
                drawVkDescriptorSetLayoutBindingContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkRenderPass) {
                drawVkRenderPassContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkAttachmentDescription) {
                drawVkAttachmentDescriptionContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkSubpassDescription) {
                drawVkSubpassDescriptionContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkSubpassDependency) {
                drawVkSubpassDependencyContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkAttachmentReference) {
                drawVkAttachmentReferenceContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkImageView) {
                drawVkImageViewContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (node.type == NodeType::VkFramebuffer) {
                drawVkFramebufferContent(drawList, node, layout, theme, panel, hoveredPin);
            } else if (nodeHasOutputPin(node.type)) {
                const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
                const bool highlighted = shouldHighlightPin(node.id, 0, false);
                drawPin(drawList, ImVec2(bottomRight.x, bodyCenterY), layout.zoom, pinColor, highlighted);
            }
        }

        void drawNodeWidgets(GraphDocument& document, const GridViewState& view, const ImVec2& displaySize,
                             bool& blockGraphDrag, bool interactive) {
            blockGraphDrag = false;
            constexpr float kMinWidgetZoom = 0.35f;

            if (!interactive) {
                ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
            }

            for (const GraphNode& node : document.nodes()) {
                if (view.zoom < kMinWidgetZoom) {
                    continue;
                }

                GraphNode* editable = document.findNode(node.id);
                if (editable == nullptr) {
                    continue;
                }

                const NodeScreenLayout layout =
                    buildNodeScreenLayout(node, view, displaySize);

                ImGui::PushID(node.id);

                if (node.type == NodeType::VkPipeline) {
                    const float indexRowCenterY = vkPipelineIndexRowCenterY(layout);
                    const float fieldWidth = std::max(56.f * layout.zoom, layout.fontSize * 3.5f);
                    const float fieldHeight = layout.fontSize + 4.f * layout.zoom;
                    const float fieldX = layout.bottomRight.x - 10.f * layout.zoom - fieldWidth;
                    const float fieldY = indexRowCenterY - fieldHeight * 0.5f;

                    ImGui::SetCursorScreenPos(ImVec2(fieldX, fieldY));
                    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));
                    ImGui::SetNextItemWidth(fieldWidth);
                    ImGui::InputInt("##renderPassIndex", &editable->renderPassIndex, 0, 0);
                    trackWidgetInteraction(interactive, blockGraphDrag);
                    ImGui::PopStyleVar();
                    ImGui::PopFont();
                } else if (node.type == NodeType::VkPipelineInputAssemblyState) {
                    drawVkPipelineInputAssemblyStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineShaderStage) {
                    drawVkPipelineShaderStageWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkShaderModule) {
                    drawVkShaderModuleWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::Vertex) {
                    drawVertexWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineVertexInputState) {
                    drawVkPipelineVertexInputStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineViewportState) {
                    drawVkPipelineViewportStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineRasterizationState) {
                    drawVkPipelineRasterizationStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineMultisampleState) {
                    drawVkPipelineMultisampleStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineDepthStencilState) {
                    drawVkPipelineDepthStencilStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineColorBlendState) {
                    drawVkPipelineColorBlendStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineColorBlendAttachmentState) {
                    drawVkPipelineColorBlendAttachmentStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineDynamicState) {
                    drawVkPipelineDynamicStateWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkPipelineLayout) {
                    drawVkPipelineLayoutWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkDescriptorSetLayout) {
                    drawVkDescriptorSetLayoutWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkDescriptorSetLayoutBinding) {
                    drawVkDescriptorSetLayoutBindingWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkRenderPass) {
                    drawVkRenderPassWidgets(document, *editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkAttachmentDescription) {
                    drawVkAttachmentDescriptionWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkSubpassDescription) {
                    drawVkSubpassDescriptionWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkSubpassDependency) {
                    drawVkSubpassDependencyWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkAttachmentReference) {
                    drawVkAttachmentReferenceWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkImageView) {
                    drawVkImageViewWidgets(*editable, layout, interactive, blockGraphDrag);
                } else if (node.type == NodeType::VkFramebuffer) {
                    drawVkFramebufferWidgets(*editable, layout, interactive, blockGraphDrag);
                }

                ImGui::PopID();
            }

            if (!interactive) {
                ImGui::PopItemFlag();
            }
        }

        void closeSearchPopup(GraphPanelState& panel, bool clearPinPreview) {
            panel.searchOpen = false;
            panel.searchBuffer[0] = '\0';
            panel.searchMode = GraphSearchMode::AddNode;
            panel.searchTypeFilterEnabled = false;
            panel.suppressNodeWidgetFocus = true;
            if (clearPinPreview) {
                clearPinLinkPreview(panel);
            }
        }

        void drawInfiniteGrid(const ImVec2& origin, const ImVec2& size, const GridViewState& view) {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            const ImU32 bgColor = IM_COL32(28, 28, 34, 255);
            const ImU32 minorColor = IM_COL32(52, 52, 62, 255);
            const ImU32 majorColor = IM_COL32(78, 78, 94, 255);
            const ImU32 axisColor = IM_COL32(110, 90, 140, 255);

            drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), bgColor);

            const ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);

            float worldStep = 32.f;
            float screenStep = worldStep * view.zoom;
            constexpr float kMinStep = 12.f;
            constexpr float kMaxStep = 128.f;
            while (screenStep < kMinStep) {
                worldStep *= 2.f;
                screenStep *= 2.f;
            }
            while (screenStep > kMaxStep) {
                worldStep *= 0.5f;
                screenStep *= 0.5f;
            }

            const int majorEvery = 5;

            const float worldLeft = (origin.x - center.x - view.pan.x) / view.zoom;
            const float worldRight = (origin.x + size.x - center.x - view.pan.x) / view.zoom;
            const float worldTop = (origin.y - center.y - view.pan.y) / view.zoom;
            const float worldBottom = (origin.y + size.y - center.y - view.pan.y) / view.zoom;

            auto drawAxisLine = [&](bool vertical, float screenPos) {
                if (vertical) {
                    drawList->AddLine(ImVec2(screenPos, origin.y), ImVec2(screenPos, origin.y + size.y), axisColor,
                                      1.5f);
                } else {
                    drawList->AddLine(ImVec2(origin.x, screenPos), ImVec2(origin.x + size.x, screenPos), axisColor,
                                      1.5f);
                }
            };

            const float startX = std::floor(worldLeft / worldStep) * worldStep;
            for (float worldX = startX; worldX <= worldRight + worldStep; worldX += worldStep) {
                const float screenX = center.x + worldX * view.zoom + view.pan.x;
                if (screenX < origin.x - 1.f || screenX > origin.x + size.x + 1.f) {
                    continue;
                }

                const int index = static_cast<int>(std::llround(worldX / worldStep));
                const bool onAxis = std::fabs(worldX) < worldStep * 0.5f;
                const bool isMajor = (index % majorEvery) == 0;

                if (onAxis) {
                    drawAxisLine(true, screenX);
                    continue;
                }

                const ImU32 color = isMajor ? majorColor : minorColor;
                const float thickness = isMajor ? 1.2f : 1.f;
                drawList->AddLine(ImVec2(screenX, origin.y), ImVec2(screenX, origin.y + size.y), color, thickness);
            }

            const float startY = std::floor(worldTop / worldStep) * worldStep;
            for (float worldY = startY; worldY <= worldBottom + worldStep; worldY += worldStep) {
                const float screenY = center.y + worldY * view.zoom + view.pan.y;
                if (screenY < origin.y - 1.f || screenY > origin.y + size.y + 1.f) {
                    continue;
                }

                const int index = static_cast<int>(std::llround(worldY / worldStep));
                const bool onAxis = std::fabs(worldY) < worldStep * 0.5f;
                const bool isMajor = (index % majorEvery) == 0;

                if (onAxis) {
                    drawAxisLine(false, screenY);
                    continue;
                }

                const ImU32 color = isMajor ? majorColor : minorColor;
                const float thickness = isMajor ? 1.2f : 1.f;
                drawList->AddLine(ImVec2(origin.x, screenY), ImVec2(origin.x + size.x, screenY), color, thickness);
            }
        }

        void drawGraphNodes(const GraphDocument& document, const GridViewState& view, const ImVec2& displaySize,
                            int selectedNodeId, const GraphPanelState& panel, const PinHit& hoveredPin) {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            for (const GraphNode& node : document.nodes()) {
                if (node.id == selectedNodeId) {
                    continue;
                }
                drawComfyNode(drawList, node, view, displaySize, false, panel, hoveredPin);
            }

            if (selectedNodeId >= 0) {
                if (const GraphNode* selected = document.findNode(selectedNodeId)) {
                    drawComfyNode(drawList, *selected, view, displaySize, true, panel, hoveredPin);
                }
            }
        }

        void deleteNode(GraphPanelState& panel, GraphDocument& document, int nodeId) {
            if (nodeId < 0 || !document.removeNode(nodeId)) {
                return;
            }

            if (panel.selectedNodeId == nodeId) {
                panel.selectedNodeId = -1;
            }
            if (panel.draggingNodeId == nodeId) {
                panel.draggingNodeId = -1;
            }
            if (panel.pinLinkDrag.nodeId == nodeId) {
                panel.pinLinkDrag.active = false;
                panel.pinLinkDrag.dragged = false;
            }
            if (panel.pinLinkPreview.nodeId == nodeId) {
                clearPinLinkPreview(panel);
            }
        }

        bool getSelectedNodeToolbarRect(const GraphDocument& document, int selectedNodeId, const GridViewState& view,
                                        const ImVec2& displaySize, ImVec2& outTopLeft, ImVec2& outSize) {
            if (selectedNodeId < 0) {
                return false;
            }

            const GraphNode* node = document.findNode(selectedNodeId);
            if (node == nullptr) {
                return false;
            }

            const NodeScreenLayout layout =
                buildNodeScreenLayout(*node, view, displaySize);
            const float toolbarHeight = kNodeToolbarHeightWorld * layout.zoom;
            const float gap = kNodeToolbarGapWorld * layout.zoom;
            outTopLeft = ImVec2(layout.topLeft.x, layout.topLeft.y - toolbarHeight - gap);
            outSize = ImVec2(layout.width, toolbarHeight);
            return true;
        }

        bool hitTestSelectedNodeToolbar(const ImVec2& screenPos, const GraphDocument& document, int selectedNodeId,
                                        const GridViewState& view, const ImVec2& displaySize) {
            ImVec2 topLeft;
            ImVec2 size;
            if (!getSelectedNodeToolbarRect(document, selectedNodeId, view, displaySize, topLeft, size)) {
                return false;
            }

            const ImVec2 bottomRight(topLeft.x + size.x, topLeft.y + size.y);
            return screenPos.x >= topLeft.x && screenPos.x <= bottomRight.x && screenPos.y >= topLeft.y &&
                   screenPos.y <= bottomRight.y;
        }

        void drawSelectedNodeToolbar(GraphPanelState& panel, GraphDocument& document, const GridViewState& view,
                                     const ImVec2& displaySize, bool& blockGraphDrag, bool interactive) {
            ImVec2 toolbarPos;
            ImVec2 toolbarSize;
            if (!getSelectedNodeToolbarRect(document, panel.selectedNodeId, view, displaySize, toolbarPos,
                                            toolbarSize)) {
                return;
            }

            const GraphNode* node = document.findNode(panel.selectedNodeId);
            if (node == nullptr) {
                return;
            }

            const float zoom = view.zoom;
            const ImVec2 toolbarBottomRight(toolbarPos.x + toolbarSize.x, toolbarPos.y + toolbarSize.y);
            const float rounding = kNodeRounding * zoom;

            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            drawList->AddRectFilled(toolbarPos, toolbarBottomRight, IM_COL32(48, 48, 56, 255), rounding);
            drawList->AddRect(toolbarPos, toolbarBottomRight, IM_COL32(232, 196, 118, 255), rounding, 0, 1.2f);

            if (!interactive) {
                return;
            }

            ImGui::PushID(panel.selectedNodeId);
            ImGui::SetCursorScreenPos(toolbarPos);
            ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * zoom);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f * zoom, 2.f * zoom));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(180, 70, 70, 180));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 50, 50, 220));

            if (ImGui::Button("Delete", toolbarSize)) {
                deleteNode(panel, document, panel.selectedNodeId);
            }
            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                blockGraphDrag = true;
            }

            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(3);
            ImGui::PopFont();
            ImGui::PopID();
        }

        void handleNodeKeyboardDelete(GraphPanelState& panel, GraphDocument& document) {
            if (panel.searchOpen || panel.pinLinkDrag.active || panel.selectedNodeId < 0) {
                return;
            }

            ImGuiIO& io = ImGui::GetIO();
            if (!ImGui::IsAnyItemActive() && !io.WantTextInput) {
                if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
                    deleteNode(panel, document, panel.selectedNodeId);
                }
            }
        }

        void handleNodeInteraction(GraphPanelState& panel, GraphDocument& document, const GridViewState& view,
                                   const ImVec2& displaySize, bool blockGraphDrag) {
            if (panel.pinLinkDrag.active) {
                return;
            }

            ImGuiIO& io = ImGui::GetIO();
            const int hitNode = hitTestNode(io.MousePos, document, view, displaySize);

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !blockGraphDrag) {
                if (hitNode >= 0) {
                    panel.selectedNodeId = hitNode;
                    panel.draggingNodeId = hitNode;
                    panel.leftDragActive = false;

                    if (GraphNode* node = document.findNode(hitNode)) {
                        const ImVec2 mouseWorld = screenToWorld(io.MousePos, view, displaySize);
                        panel.nodeDragGrabOffset =
                            ImVec2(node->worldX - mouseWorld.x, node->worldY - mouseWorld.y);
                    }
                } else if (hitTestSelectedNodeToolbar(io.MousePos, document, panel.selectedNodeId, view,
                                                      displaySize)) {
                    panel.leftDragActive = false;
                } else {
                    panel.selectedNodeId = -1;
                    panel.draggingNodeId = -1;
                    panel.leftDragActive = true;
                    panel.leftDragStart = io.MousePos;
                }
            }

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                panel.draggingNodeId = -1;
            }

            if (panel.draggingNodeId >= 0 && io.MouseDown[ImGuiMouseButton_Left] && !blockGraphDrag) {
                const ImVec2 mouseWorld = screenToWorld(io.MousePos, view, displaySize);
                document.setNodePosition(panel.draggingNodeId, mouseWorld.x + panel.nodeDragGrabOffset.x,
                                         mouseWorld.y + panel.nodeDragGrabOffset.y);
            }
        }

        void updateGridView(GridViewState& view, const ImVec2& displaySize, GraphPanelState& panel) {
            ImGuiIO& io = ImGui::GetIO();
            const ImVec2 center = screenCenter(displaySize);

            if (panel.searchOpen || panel.draggingNodeId >= 0 || panel.pinLinkDrag.active) {
                return;
            }

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                panel.leftDragActive = false;
            }

            if (panel.leftDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                const float dx = io.MousePos.x - panel.leftDragStart.x;
                const float dy = io.MousePos.y - panel.leftDragStart.y;
                if ((dx * dx + dy * dy) >= kDragThreshold * kDragThreshold) {
                    view.pan.x += io.MouseDelta.x;
                    view.pan.y += io.MouseDelta.y;
                }
            }

            if (io.MouseWheel != 0.f && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup) &&
                !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive()) {
                const float zoomFactor = std::pow(1.12f, io.MouseWheel);
                const ImVec2 mouse = io.MousePos;
                const float worldX = (mouse.x - center.x - view.pan.x) / view.zoom;
                const float worldY = (mouse.y - center.y - view.pan.y) / view.zoom;
                view.zoom *= zoomFactor;
                if (view.zoom < 0.15f) {
                    view.zoom = 0.15f;
                }
                if (view.zoom > 8.f) {
                    view.zoom = 8.f;
                }
                view.pan.x = mouse.x - center.x - worldX * view.zoom;
                view.pan.y = mouse.y - center.y - worldY * view.zoom;
            }
        }

    }  // namespace

    void drawGraphPanel(GridViewState& view, GraphPanelState& panel, GraphDocument& document,
                        const ImVec2& displaySize) {
        const PinHit mousePin =
            !panel.searchOpen ? hitTestPin(ImGui::GetIO().MousePos, document, view, displaySize) : PinHit{};

        PinHit hoveredPin{};
        if (!panel.searchOpen) {
            if (panel.pinLinkDrag.active && panel.pinLinkDrag.dragged) {
                if (canConnectPinLink(document, panel.pinLinkDrag, mousePin)) {
                    hoveredPin = mousePin;
                }
            } else if (!panel.pinLinkDrag.active) {
                hoveredPin = mousePin;
            }
        }

        drawInfiniteGrid(ImVec2(0.f, 0.f), displaySize, view);
        beginPinHighlightContext(document, panel, hoveredPin);
        drawGraphLinks(document, view, displaySize, hoveredPin);
        drawPinLinkPreview(panel, document, view, displaySize);
        drawGraphNodes(document, view, displaySize, panel.selectedNodeId, panel, hoveredPin);
        endPinHighlightContext();

        const bool suppressingNodeWidgetFocus = panel.suppressNodeWidgetFocus;
        const bool nodeWidgetsInteractive = !panel.searchOpen && !suppressingNodeWidgetFocus;

        ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
        ImGui::SetNextWindowSize(displaySize);
        ImGui::Begin("##GraphPanel", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoBackground);

        bool blockGraphDrag = false;
        drawNodeWidgets(document, view, displaySize, blockGraphDrag, nodeWidgetsInteractive);
        drawSelectedNodeToolbar(panel, document, view, displaySize, blockGraphDrag, nodeWidgetsInteractive);

        if (!panel.searchOpen) {
            handlePinLinkInteraction(panel, document, view, displaySize, blockGraphDrag);
            handleNodeInteraction(panel, document, view, displaySize, blockGraphDrag);
            handleNodeKeyboardDelete(panel, document);
            updateGridView(view, displaySize, panel);

            if (!panel.pinLinkDrag.active && ImGui::IsWindowHovered() &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                hitTestNode(ImGui::GetIO().MousePos, document, view, displaySize) < 0 &&
                !hitTestPin(ImGui::GetIO().MousePos, document, view, displaySize).valid) {
                panel.searchOpen = true;
                panel.searchMode = GraphSearchMode::AddNode;
                panel.searchTypeFilterEnabled = false;
                panel.searchScreenPos = ImGui::GetIO().MousePos;
                panel.spawnWorldPos = screenToWorld(panel.searchScreenPos, view, displaySize);
                panel.searchBuffer[0] = '\0';
                panel.leftDragActive = false;
                panel.searchOpenedFrame = ImGui::GetFrameCount();
                panel.searchFocusInput = true;
            }
        }

        ImGui::End();

        if (suppressingNodeWidgetFocus) {
            panel.suppressNodeWidgetFocus = false;
        }

        if (panel.searchOpen) {
            NodeType selectedType = NodeType::VkPipeline;
            bool popupHovered = false;
            const bool focusInput = panel.searchFocusInput;
            panel.searchFocusInput = false;

            if (drawNodeSearchPopup(panel.searchScreenPos, panel.searchBuffer,
                                    static_cast<int>(sizeof(panel.searchBuffer)), &selectedType, &popupHovered,
                                    focusInput, panel.searchTypeFilterEnabled, panel.searchTypeFilter)) {
                if (panel.searchMode == GraphSearchMode::PinLink) {
                    finalizePinLinkSearch(panel, document, selectedType);
                } else {
                    document.addNode(selectedType, panel.spawnWorldPos.x, panel.spawnWorldPos.y);
                    panel.selectedNodeId = document.nodes().back().id;
                }
                closeSearchPopup(panel, false);
            } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                closeSearchPopup(panel, true);
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !popupHovered &&
                       ImGui::GetFrameCount() > panel.searchOpenedFrame) {
                closeSearchPopup(panel, true);
            }
        }
    }

}  // namespace mat::demo
