#include "editor/GraphCanvas.h"

#include "editor/NodeSearchPopup.h"
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
            switch (type) {
                case NodeType::VkPipeline:
                    return {IM_COL32(62, 102, 152, 255), IM_COL32(48, 48, 56, 255), IM_COL32(72, 72, 84, 255),
                            IM_COL32(232, 196, 118, 255), IM_COL32(245, 245, 250, 255),
                            IM_COL32(190, 200, 215, 255)};
            }
            return {IM_COL32(90, 90, 100, 255), IM_COL32(48, 48, 56, 255), IM_COL32(72, 72, 84, 255),
                    IM_COL32(232, 196, 118, 255), IM_COL32(245, 245, 250, 255), IM_COL32(175, 175, 190, 255)};
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

        NodeScreenLayout buildNodeScreenLayout(float worldX, float worldY, NodeType type, const GridViewState& view,
                                               const ImVec2& displaySize) {
            const float zoom = view.zoom;
            const ImVec2 worldSize = nodeWorldSize(type);
            const ImVec2 screenPos = worldToScreen(ImVec2(worldX, worldY), view, displaySize);
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

        struct PinHit {
            bool valid = false;
            int nodeId = -1;
            int pinIndex = -1;
            bool isInput = false;
        };

        bool getOutputPinScreenPos(const GraphNode& node, const GridViewState& view, const ImVec2& displaySize,
                                   ImVec2& outPos) {
            if (!nodeHasOutputPin(node.type)) {
                return false;
            }

            const NodeScreenLayout layout = buildNodeScreenLayout(node.worldX, node.worldY, node.type, view, displaySize);
            const float bodyCenterY = layout.topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
            outPos = ImVec2(layout.bottomRight.x, bodyCenterY);
            return true;
        }

        bool getInputPinScreenPos(const GraphNode& node, int pinIndex, const GridViewState& view,
                                  const ImVec2& displaySize, ImVec2& outPos) {
            if (node.type != NodeType::VkPipeline || pinIndex < 0 || pinIndex >= kVkPipelineInputPinCount) {
                return false;
            }

            const NodeScreenLayout layout = buildNodeScreenLayout(node.worldX, node.worldY, node.type, view, displaySize);
            const float rowCenterY = layout.topLeft.y + layout.headerHeight + (pinIndex + 0.5f) * layout.pinRowHeight;
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

                if (node.type == NodeType::VkPipeline) {
                    for (int pinIndex = 0; pinIndex < kVkPipelineInputPinCount; ++pinIndex) {
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
                } else if (nodeHasOutputPin(node.type)) {
                    ImVec2 pinPos;
                    if (!getOutputPinScreenPos(node, view, displaySize, pinPos)) {
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
                        best.pinIndex = -1;
                        best.isInput = false;
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
            return getOutputPinScreenPos(*node, view, displaySize, outPos);
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

        void drawGraphLinks(const GraphDocument& document, const GridViewState& view, const ImVec2& displaySize) {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();
            const ImU32 linkColor = IM_COL32(210, 210, 220, 235);
            const float thickness = std::max(2.5f, 3.f * view.zoom);

            for (const GraphLink& link : document.links()) {
                const GraphNode* fromNode = document.findNode(link.fromNodeId);
                const GraphNode* toNode = document.findNode(link.toNodeId);
                if (fromNode == nullptr || toNode == nullptr) {
                    continue;
                }

                ImVec2 startPos;
                ImVec2 endPos;
                if (!getOutputPinScreenPos(*fromNode, view, displaySize, startPos) ||
                    !getInputPinScreenPos(*toNode, link.toPinIndex, view, displaySize, endPos)) {
                    continue;
                }

                drawBezierLink(drawList, startPos, endPos, true, linkColor, thickness);
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

        bool tryConnectPinLink(GraphDocument& document, const PinLinkDrag& drag, const PinHit& target) {
            if (!drag.active || !target.valid || drag.nodeId == target.nodeId) {
                return false;
            }

            if (drag.fromInput && !target.isInput) {
                const GraphNode* pipelineNode = document.findNode(drag.nodeId);
                const GraphNode* sourceNode = document.findNode(target.nodeId);
                if (pipelineNode == nullptr || sourceNode == nullptr || pipelineNode->type != NodeType::VkPipeline ||
                    !nodeHasOutputPin(sourceNode->type)) {
                    return false;
                }

                const VkPipelineInputPinDef* pinDef = vkPipelineInputPin(drag.pinIndex);
                if (pinDef == nullptr || pinDef->slotType != sourceNode->type) {
                    return false;
                }

                document.addLink(target.nodeId, drag.nodeId, drag.pinIndex);
                return true;
            }

            if (!drag.fromInput && target.isInput) {
                const GraphNode* sourceNode = document.findNode(drag.nodeId);
                const GraphNode* pipelineNode = document.findNode(target.nodeId);
                if (sourceNode == nullptr || pipelineNode == nullptr || pipelineNode->type != NodeType::VkPipeline ||
                    !nodeHasOutputPin(sourceNode->type)) {
                    return false;
                }

                const VkPipelineInputPinDef* pinDef = vkPipelineInputPin(target.pinIndex);
                if (pinDef == nullptr || pinDef->slotType != sourceNode->type) {
                    return false;
                }

                document.addLink(drag.nodeId, target.nodeId, target.pinIndex);
                return true;
            }

            return false;
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
                const VkPipelineInputPinDef* pinDef = vkPipelineInputPin(panel.pinLinkDrag.pinIndex);
                panel.searchTypeFilter = pinDef != nullptr ? pinDef->slotType : NodeType::VkPipeline;
            } else if (const GraphNode* sourceNode = document.findNode(panel.pinLinkDrag.nodeId)) {
                panel.searchTypeFilter = NodeType::VkPipeline;
                (void)sourceNode;
            } else {
                panel.searchTypeFilter = NodeType::VkPipeline;
            }
        }

        void finalizePinLinkSearch(GraphPanelState& panel, GraphDocument& document, NodeType selectedType) {
            const int newNodeId = document.addNode(selectedType, panel.spawnWorldPos.x, panel.spawnWorldPos.y);

            if (panel.pinLinkFromInput) {
                document.addLink(newNodeId, panel.pinLinkFromNodeId, panel.pinLinkFromPinIndex);
            } else {
                const GraphNode* sourceNode = document.findNode(panel.pinLinkFromNodeId);
                if (sourceNode != nullptr && selectedType == NodeType::VkPipeline) {
                    const int pinIndex = vkPipelineInputPinIndexForType(sourceNode->type);
                    if (pinIndex >= 0) {
                        document.addLink(panel.pinLinkFromNodeId, newNodeId, pinIndex);
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
                            getOutputPinScreenPos(*pinNode, view, displaySize, panel.pinLinkDrag.startScreen);
                        }
                    }
                }
            }
        }

        void nodeScreenBounds(const GraphNode& node, const GridViewState& view, const ImVec2& displaySize,
                              ImVec2& topLeft, ImVec2& bottomRight) {
            const ImVec2 screenPos = worldToScreen(ImVec2(node.worldX, node.worldY), view, displaySize);
            const ImVec2 worldSize = nodeWorldSize(node.type);
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

        bool isPinHighlighted(const PinHit& pin, int nodeId, int pinIndex, bool isInput) {
            return pin.valid && pin.nodeId == nodeId && pin.pinIndex == pinIndex && pin.isInput == isInput;
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
            const NodeScreenLayout layout = buildNodeScreenLayout(node.worldX, node.worldY, node.type, view, displaySize);
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
                for (int pinIndex = 0; pinIndex < kVkPipelineInputPinCount; ++pinIndex) {
                    const VkPipelineInputPinDef* pinDef = vkPipelineInputPin(pinIndex);
                    if (pinDef == nullptr) {
                        continue;
                    }

                    const float rowCenterY = topLeft.y + layout.headerHeight + (pinIndex + 0.5f) * layout.pinRowHeight;
                    const bool highlighted = isPinHighlighted(hoveredPin, node.id, pinIndex, true) ||
                                             isPinLinkSource(panel, node.id, pinIndex, true);
                    drawPin(drawList, ImVec2(topLeft.x, rowCenterY), layout.zoom, pinColor, highlighted);
                    drawScaledText(drawList,
                                   ImVec2(topLeft.x + labelPadX, rowCenterY - layout.fontSize * 0.5f), theme.pinLabel,
                                   pinDef->label, layout.fontSize);
                }

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
            } else if (nodeHasOutputPin(node.type)) {
                const float bodyCenterY = topLeft.y + layout.headerHeight + (layout.height - layout.headerHeight) * 0.5f;
                const bool highlighted = isPinHighlighted(hoveredPin, node.id, -1, false) ||
                                         isPinLinkSource(panel, node.id, -1, false);
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
                if (node.type != NodeType::VkPipeline || view.zoom < kMinWidgetZoom) {
                    continue;
                }

                GraphNode* editable = document.findNode(node.id);
                if (editable == nullptr) {
                    continue;
                }

                const NodeScreenLayout layout =
                    buildNodeScreenLayout(node.worldX, node.worldY, node.type, view, displaySize);
                const float indexRowCenterY = vkPipelineIndexRowCenterY(layout);
                const float fieldWidth = std::max(56.f * layout.zoom, layout.fontSize * 3.5f);
                const float fieldHeight = layout.fontSize + 4.f * layout.zoom;
                const float fieldX = layout.bottomRight.x - 10.f * layout.zoom - fieldWidth;
                const float fieldY = indexRowCenterY - fieldHeight * 0.5f;

                ImGui::PushID(node.id);
                ImGui::SetCursorScreenPos(ImVec2(fieldX, fieldY));
                ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * layout.zoom);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.f * layout.zoom, 1.f * layout.zoom));
                ImGui::SetNextItemWidth(fieldWidth);
                ImGui::InputInt("##renderPassIndex", &editable->renderPassIndex, 0, 0);
                if (interactive && (ImGui::IsItemHovered() || ImGui::IsItemActive())) {
                    blockGraphDrag = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopFont();
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

            if (io.MouseWheel != 0.f) {
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
        const PinHit hoveredPin =
            (!panel.pinLinkDrag.active && !panel.searchOpen)
                ? hitTestPin(ImGui::GetIO().MousePos, document, view, displaySize)
                : PinHit{};

        drawInfiniteGrid(ImVec2(0.f, 0.f), displaySize, view);
        drawGraphLinks(document, view, displaySize);
        drawPinLinkPreview(panel, document, view, displaySize);
        drawGraphNodes(document, view, displaySize, panel.selectedNodeId, panel, hoveredPin);

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

        if (!panel.searchOpen) {
            handlePinLinkInteraction(panel, document, view, displaySize, blockGraphDrag);
            handleNodeInteraction(panel, document, view, displaySize, blockGraphDrag);
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
