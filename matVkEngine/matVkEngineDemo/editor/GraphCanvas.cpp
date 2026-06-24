#include "editor/GraphCanvas.h"

#include "editor/NodeSearchPopup.h"
#include "graph/GraphTypes.h"

#include <imgui.h>

#include <cmath>

namespace mat::demo {

    namespace {

        constexpr float kDragThreshold = 4.f;
        constexpr float kNodeWidth = 200.f;
        constexpr float kNodeHeaderHeight = 30.f;
        constexpr float kNodeBodyHeight = 28.f;
        constexpr float kNodeRounding = 8.f;

        struct NodeTheme {
            ImU32 header;
            ImU32 body;
            ImU32 border;
            ImU32 selectedBorder;
            ImU32 title;
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
                            IM_COL32(232, 196, 118, 255), IM_COL32(245, 245, 250, 255)};
            }
            return {IM_COL32(90, 90, 100, 255), IM_COL32(48, 48, 56, 255), IM_COL32(72, 72, 84, 255),
                    IM_COL32(232, 196, 118, 255), IM_COL32(245, 245, 250, 255)};
        }

        ImVec2 nodeWorldSize() {
            return ImVec2(kNodeWidth, kNodeHeaderHeight + kNodeBodyHeight);
        }

        void nodeScreenBounds(const GraphNode& node, const GridViewState& view, const ImVec2& displaySize,
                              ImVec2& topLeft, ImVec2& bottomRight) {
            const ImVec2 screenPos = worldToScreen(ImVec2(node.worldX, node.worldY), view, displaySize);
            const ImVec2 worldSize = nodeWorldSize();
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

        void drawComfyNode(ImDrawList* drawList, const GraphNode& node, const GridViewState& view,
                           const ImVec2& displaySize, bool selected) {
            const NodeTheme theme = nodeTheme(node.type);
            const ImVec2 screenPos = worldToScreen(ImVec2(node.worldX, node.worldY), view, displaySize);
            const float zoom = view.zoom;
            const float width = kNodeWidth * zoom;
            const float headerHeight = kNodeHeaderHeight * zoom;
            const float bodyHeight = kNodeBodyHeight * zoom;
            const float height = headerHeight + bodyHeight;
            const float rounding = kNodeRounding * zoom;
            const ImVec2 topLeft(screenPos.x - width * 0.5f, screenPos.y - height * 0.5f);
            const ImVec2 bottomRight(topLeft.x + width, topLeft.y + height);
            const ImVec2 headerBottomRight(bottomRight.x, topLeft.y + headerHeight);

            if (selected) {
                const float glowPad = 3.f * zoom;
                drawList->AddRectFilled(ImVec2(topLeft.x - glowPad, topLeft.y - glowPad),
                                        ImVec2(bottomRight.x + glowPad, bottomRight.y + glowPad),
                                        IM_COL32(232, 196, 118, 36), rounding + glowPad);
            }

            drawList->AddRectFilled(ImVec2(topLeft.x + 1.f, topLeft.y + 2.f),
                                    ImVec2(bottomRight.x + 1.f, bottomRight.y + 2.f), IM_COL32(0, 0, 0, 70), rounding);
            drawList->AddRectFilled(topLeft, bottomRight, theme.body, rounding);
            drawList->AddRectFilled(topLeft, headerBottomRight, theme.header, rounding, ImDrawFlags_RoundCornersTop);
            drawList->AddRectFilled(ImVec2(topLeft.x, topLeft.y + headerHeight - 1.f), headerBottomRight, theme.header);

            const float borderThickness = selected ? 2.5f * zoom : 1.2f * zoom;
            const ImU32 borderColor = selected ? theme.selectedBorder : theme.border;
            drawList->AddRect(topLeft, bottomRight, borderColor, rounding, 0, borderThickness);

            const char* title = nodeTypeName(node.type);
            const float titlePadX = 10.f * zoom;
            const float titleY = topLeft.y + (headerHeight - ImGui::GetFontSize()) * 0.5f;
            drawList->AddText(ImVec2(topLeft.x + titlePadX, titleY), theme.title, title);

            const float slotRadius = 5.f * zoom;
            const ImVec2 inputSlot(topLeft.x, topLeft.y + headerHeight + bodyHeight * 0.5f);
            const ImVec2 outputSlot(bottomRight.x, topLeft.y + headerHeight + bodyHeight * 0.5f);
            drawList->AddCircleFilled(inputSlot, slotRadius + 1.2f * zoom, IM_COL32(18, 18, 22, 255));
            drawList->AddCircleFilled(inputSlot, slotRadius, IM_COL32(170, 170, 185, 255));
            drawList->AddCircleFilled(outputSlot, slotRadius + 1.2f * zoom, IM_COL32(18, 18, 22, 255));
            drawList->AddCircleFilled(outputSlot, slotRadius, IM_COL32(170, 170, 185, 255));
        }

        void drawGraphNodes(const GraphDocument& document, const GridViewState& view, const ImVec2& displaySize,
                            int selectedNodeId) {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            for (const GraphNode& node : document.nodes()) {
                if (node.id == selectedNodeId) {
                    continue;
                }
                drawComfyNode(drawList, node, view, displaySize, false);
            }

            if (selectedNodeId >= 0) {
                if (const GraphNode* selected = document.findNode(selectedNodeId)) {
                    drawComfyNode(drawList, *selected, view, displaySize, true);
                }
            }
        }

        void handleNodeInteraction(GraphPanelState& panel, GraphDocument& document, const GridViewState& view,
                                   const ImVec2& displaySize) {
            ImGuiIO& io = ImGui::GetIO();
            const int hitNode = hitTestNode(io.MousePos, document, view, displaySize);

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
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

            if (panel.draggingNodeId >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                const ImVec2 mouseWorld = screenToWorld(io.MousePos, view, displaySize);
                document.setNodePosition(panel.draggingNodeId, mouseWorld.x + panel.nodeDragGrabOffset.x,
                                         mouseWorld.y + panel.nodeDragGrabOffset.y);
            }
        }

        void updateGridView(GridViewState& view, const ImVec2& displaySize, GraphPanelState& panel) {
            ImGuiIO& io = ImGui::GetIO();
            const ImVec2 center = screenCenter(displaySize);

            if (panel.searchOpen || panel.draggingNodeId >= 0) {
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
        drawInfiniteGrid(ImVec2(0.f, 0.f), displaySize, view);
        drawGraphNodes(document, view, displaySize, panel.selectedNodeId);

        if (!panel.searchOpen) {
            ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
            ImGui::SetNextWindowSize(displaySize);
            ImGui::Begin("##GraphPanel", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoBackground);

            handleNodeInteraction(panel, document, view, displaySize);
            updateGridView(view, displaySize, panel);

            if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
                hitTestNode(ImGui::GetIO().MousePos, document, view, displaySize) < 0) {
                panel.searchOpen = true;
                panel.searchScreenPos = ImGui::GetIO().MousePos;
                panel.spawnWorldPos = screenToWorld(panel.searchScreenPos, view, displaySize);
                panel.searchBuffer[0] = '\0';
                panel.leftDragActive = false;
                panel.searchOpenedFrame = ImGui::GetFrameCount();
                panel.searchFocusInput = true;
            }

            ImGui::End();
        }

        if (panel.searchOpen) {
            NodeType selectedType = NodeType::VkPipeline;
            bool popupHovered = false;
            const bool focusInput = panel.searchFocusInput;
            panel.searchFocusInput = false;

            if (drawNodeSearchPopup(panel.searchScreenPos, panel.searchBuffer,
                                    static_cast<int>(sizeof(panel.searchBuffer)), &selectedType, &popupHovered,
                                    focusInput)) {
                document.addNode(selectedType, panel.spawnWorldPos.x, panel.spawnWorldPos.y);
                panel.selectedNodeId = document.nodes().back().id;
                panel.searchOpen = false;
                panel.searchBuffer[0] = '\0';
            } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                panel.searchOpen = false;
                panel.searchBuffer[0] = '\0';
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !popupHovered &&
                       ImGui::GetFrameCount() > panel.searchOpenedFrame) {
                panel.searchOpen = false;
                panel.searchBuffer[0] = '\0';
            }
        }
    }

}  // namespace mat::demo
