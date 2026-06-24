#include "editor/NodeSearchPopup.h"

#include "editor/FuzzySearch.h"
#include "graph/GraphTypes.h"

#include <imgui.h>

#include <algorithm>
#include <vector>

namespace mat::demo {

    namespace {

        struct SearchEntry {
            NodeType type;
            const char* label;
        };

        constexpr SearchEntry kSearchEntries[] = {
            {NodeType::VkPipeline, "VkPipeline"},
        };

    }  // namespace

    bool drawNodeSearchPopup(const ImVec2& screenPos, char* buffer, int bufferSize, NodeType* outSelectedType,
                             bool* outHovered, bool focusInput) {
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
        if (buffer[0] != '\0') {
            matches.reserve(sizeof(kSearchEntries) / sizeof(kSearchEntries[0]));

            for (const SearchEntry& entry : kSearchEntries) {
                const int score = fuzzyMatchScore(entry.label, buffer);
                if (score >= 0) {
                    matches.push_back({entry.type, entry.label, score});
                }
            }

            std::sort(matches.begin(), matches.end(),
                      [](const NodeSearchResult& a, const NodeSearchResult& b) { return a.score > b.score; });
        }

        if (buffer[0] != '\0') {
            if (matches.empty()) {
                ImGui::TextDisabled("No matching node");
            } else {
                for (const NodeSearchResult& match : matches) {
                    ImGui::PushID(static_cast<int>(match.type));
                    if (ImGui::Selectable(match.label, false)) {
                        if (outSelectedType != nullptr) {
                            *outSelectedType = match.type;
                        }
                        selected = true;
                    }
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
