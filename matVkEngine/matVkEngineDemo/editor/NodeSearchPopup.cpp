#include "editor/NodeSearchPopup.h"

#include "editor/FuzzySearch.h"
#include "graph/GraphTypes.h"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <vector>

namespace mat::demo {

    namespace {

        struct SearchEntry {
            NodeType type;
            const char* label;
        };

        constexpr SearchEntry kSearchEntries[] = {
            {NodeType::VkPipeline, "VkPipeline"},
            {NodeType::VkPipelineShaderStage, "VkPipelineShaderStage"},
            {NodeType::VkPipelineVertexInputState, "VkPipelineVertexInputState"},
            {NodeType::VkPipelineInputAssemblyState, "VkPipelineInputAssemblyState"},
            {NodeType::VkPipelineViewportState, "VkPipelineViewportState"},
            {NodeType::VkPipelineRasterizationState, "VkPipelineRasterizationState"},
            {NodeType::VkPipelineMultisampleState, "VkPipelineMultisampleState"},
            {NodeType::VkPipelineDepthStencilState, "VkPipelineDepthStencilState"},
            {NodeType::VkPipelineColorBlendState, "VkPipelineColorBlendState"},
            {NodeType::VkPipelineColorBlendAttachmentState, "VkPipelineColorBlendAttachmentState"},
            {NodeType::VkColorWriteMask, "VkColorWriteMask"},
            {NodeType::VkDynamicState, "VkDynamicState"},
            {NodeType::VkPipelineDynamicState, "VkPipelineDynamicState"},
            {NodeType::VkDescriptorSetLayoutBinding, "VkDescriptorSetLayoutBinding"},
            {NodeType::VkDescriptorSetLayout, "VkDescriptorSetLayout"},
            {NodeType::VkPipelineLayout, "VkPipelineLayout"},
            {NodeType::VkRenderPass, "VkRenderPass"},
            {NodeType::VkAttachmentDescription, "VkAttachmentDescription"},
            {NodeType::VkSubpassDescription, "VkSubpassDescription"},
            {NodeType::VkSubpassDependency, "VkSubpassDependency"},
        };

    }  // namespace

    bool drawNodeSearchPopup(const ImVec2& screenPos, char* buffer, int bufferSize, NodeType* outSelectedType,
                             bool* outHovered, bool focusInput, bool useTypeFilter, NodeType typeFilter) {
        ImGui::SetNextWindowPos(screenPos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(ImVec2(260.f, 0.f), ImGuiCond_Appearing);
        ImGui::SetNextWindowFocus();

        bool selected = false;
        if (!ImGui::Begin("##NodeSearchPopup", nullptr,
                          ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
            if (outHovered != nullptr) {
                *outHovered = false;
            }
            ImGui::End();
            return false;
        }

        if (focusInput || ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputTextWithHint("##search", "Search node...", buffer, static_cast<size_t>(bufferSize));

        std::vector<NodeSearchResult> matches;
        const bool hasQuery = buffer[0] != '\0';
        if (hasQuery || useTypeFilter) {
            matches.reserve(sizeof(kSearchEntries) / sizeof(kSearchEntries[0]));

            for (const SearchEntry& entry : kSearchEntries) {
                if (useTypeFilter && entry.type != typeFilter) {
                    continue;
                }

                int score = 0;
                if (hasQuery) {
                    score = fuzzyMatchScore(entry.label, buffer);
                    if (score < 0) {
                        continue;
                    }
                }

                matches.push_back({entry.type, entry.label, score});
            }

            std::sort(matches.begin(), matches.end(),
                      [](const NodeSearchResult& a, const NodeSearchResult& b) { return a.score > b.score; });
        }

        if (hasQuery || useTypeFilter) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (matches.empty()) {
                ImGui::TextDisabled("No matching node");
            } else {
                const float itemWidth = ImGui::GetContentRegionAvail().x;
                for (size_t i = 0; i < matches.size(); ++i) {
                    const NodeSearchResult& match = matches[i];
                    const bool isLast = (i + 1 == matches.size());

                    ImGui::PushID(static_cast<int>(match.type));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, isLast ? 0.f : 2.f));
                    if (ImGui::Selectable(match.label, false, ImGuiSelectableFlags_None,
                                          ImVec2(itemWidth > 0.f ? itemWidth : -FLT_MIN, 0.f))) {
                        if (outSelectedType != nullptr) {
                            *outSelectedType = match.type;
                        }
                        selected = true;
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }
            }
        }

        if (!selected && ImGui::IsKeyPressed(ImGuiKey_Enter) && !matches.empty()) {
            if (outSelectedType != nullptr) {
                *outSelectedType = matches.front().type;
            }
            selected = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            buffer[0] = '\0';
        }

        if (outHovered != nullptr) {
            *outHovered = ImGui::IsWindowHovered();
        }

        ImGui::End();
        return selected;
    }

}  // namespace mat::demo
