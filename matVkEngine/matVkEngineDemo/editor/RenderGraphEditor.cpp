#include "editor/RenderGraphEditor.h"

#include "editor/NodeRegistry.h"
#include "graph/GraphSerializer.h"
#include "graph/GraphTemplates.h"
#include "graph/GraphValidator.h"

#include <imnodes.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace mat::demo {

    void RenderGraphEditor::initialize() {
        if (_initialized) {
            return;
        }

        ImNodes::CreateContext();
        ImNodesStyle& style = ImNodes::GetStyle();
        style.NodePadding = ImVec2(6.f, 4.f);
        style.NodeBorderThickness = 1.f;

        ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);

        _initialized = true;
    }

    void RenderGraphEditor::shutdown() {
        if (!_initialized) {
            return;
        }

        ImNodes::PopAttributeFlag();
        ImNodes::DestroyContext();
        _initialized = false;
    }

    void RenderGraphEditor::createStarterGraph(GraphDocument& document) {
        if (!document.nodes().empty()) {
            return;
        }

        GraphTemplates::buildForward(document);
        _lastMessage = "Created starter graph (Forward, single pass)";
    }

    void RenderGraphEditor::loadExampleGraph(GraphDocument& document, const char* path,
                                             void (*builder)(GraphDocument&)) {
        document.clear();
        builder(document);
        _graphPath = path;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(_graphPath).parent_path(), ec);
        std::string saveError;
        GraphSerializer::save(document, _graphPath, saveError);
        _lastMessage = std::string("Loaded example: ") + path;
        _validationErrors.clear();
    }

    void RenderGraphEditor::drawNode(GraphNode& node) {
        ImNodes::BeginNode(node.id);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted(NodeRegistry::displayName(node.type));
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginStaticAttribute(NodeRegistry::encodeStaticAttr(node.id));
        NodeRegistry::drawNodeBody(node);
        ImNodes::EndStaticAttribute();

        for (int i = 0; i < static_cast<int>(node.inputs.size()); ++i) {
            const PinDesc& pin = node.inputs[i];
            ImNodes::BeginInputAttribute(NodeRegistry::encodeAttr(node.id, i, false));
            ImGui::TextUnformatted(pin.label.c_str());
            ImNodes::EndInputAttribute();
        }

        for (int i = 0; i < static_cast<int>(node.outputs.size()); ++i) {
            const PinDesc& pin = node.outputs[i];
            ImNodes::BeginOutputAttribute(NodeRegistry::encodeAttr(node.id, i, true));
            ImGui::TextUnformatted(pin.label.c_str());
            ImNodes::EndOutputAttribute();
        }

        ImNodes::EndNode();
    }

    void RenderGraphEditor::handleInteraction(GraphDocument& document) {
        int startAttr = 0;
        int endAttr = 0;
        if (ImNodes::IsLinkCreated(&startAttr, &endAttr)) {
            std::string error;
            if (document.tryConnect(startAttr, endAttr, error)) {
                _lastMessage = "Link created";
            } else {
                _lastMessage = error;
            }
        }

        int linkId = 0;
        if (ImNodes::IsLinkDestroyed(&linkId)) {
            if (document.removeLink(linkId)) {
                _lastMessage = "Link removed";
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            const int count = ImNodes::NumSelectedNodes();
            if (count > 0) {
                std::vector<int> nodeIds(static_cast<size_t>(count));
                ImNodes::GetSelectedNodes(nodeIds.data());
                for (int nodeId : nodeIds) {
                    document.removeNode(nodeId);
                }
                _lastMessage = "Node(s) removed";
            }
        }
    }

    void RenderGraphEditor::drawNodeEditor(GraphDocument& document) {
        ImNodes::BeginNodeEditor();

        for (GraphNode& node : document.nodes()) {
            ImNodes::SetNodeGridSpacePos(node.id, ImVec2(node.posX, node.posY));
            drawNode(node);
        }

        for (const GraphLink& link : document.links()) {
            ImNodes::Link(link.id, link.startAttr, link.endAttr);
        }

        ImNodes::EndNodeEditor();
        handleInteraction(document);

        for (GraphNode& node : document.nodes()) {
            ImVec2 pos = ImNodes::GetNodeGridSpacePos(node.id);
            node.posX = pos.x;
            node.posY = pos.y;
        }
    }

    void RenderGraphEditor::drawToolbar(GraphDocument& document) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Add Node")) {
                const ImVec2 mouse = ImGui::GetMousePos();
                if (ImGui::BeginMenu("Pipeline")) {
                    if (ImGui::MenuItem("Render Pass")) {
                        document.addNode(NodeType::RenderPass, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Subpass")) {
                        document.addNode(NodeType::Subpass, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Graphics Pipeline")) {
                        document.addNode(NodeType::GraphicsPipeline, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Draw Pass")) {
                        document.addNode(NodeType::DrawPass, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Fullscreen Pass")) {
                        document.addNode(NodeType::FullscreenPass, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("UI Pass")) {
                        document.addNode(NodeType::UiPass, mouse.x, mouse.y);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Resources")) {
                    if (ImGui::MenuItem("Texture")) {
                        document.addNode(NodeType::Texture, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Shader")) {
                        document.addNode(NodeType::Shader, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Vertex Buffer")) {
                        document.addNode(NodeType::Vertex, mouse.x, mouse.y);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Scene")) {
                    if (ImGui::MenuItem("Entity")) {
                        document.addNode(NodeType::Entity, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Camera")) {
                        document.addNode(NodeType::Camera, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Light")) {
                        document.addNode(NodeType::Light, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Scene")) {
                        document.addNode(NodeType::Scene, mouse.x, mouse.y);
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("I/O")) {
                    if (ImGui::MenuItem("Render Config")) {
                        document.addNode(NodeType::RenderConfig, mouse.x, mouse.y);
                    }
                    if (ImGui::MenuItem("Output PNG")) {
                        document.addNode(NodeType::OutputPng, mouse.x, mouse.y);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Examples")) {
                if (ImGui::MenuItem("Forward (single pass)")) {
                    loadExampleGraph(document, "graphs/forward.graph.json", GraphTemplates::buildForward);
                }
                if (ImGui::MenuItem("Deferred + Post + UI (multi pass)")) {
                    loadExampleGraph(document, "graphs/deferred_multi_pass.graph.json",
                                     GraphTemplates::buildDeferredMultiPass);
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Save")) {
                std::error_code ec;
                std::filesystem::create_directories(std::filesystem::path(_graphPath).parent_path(), ec);
                std::string error;
                if (GraphSerializer::save(document, _graphPath, error)) {
                    _lastMessage = "Saved to " + _graphPath;
                } else {
                    _lastMessage = error;
                }
            }

            if (ImGui::MenuItem("Load")) {
                std::string error;
                if (GraphSerializer::load(document, _graphPath, error)) {
                    _lastMessage = "Loaded " + _graphPath;
                } else {
                    _lastMessage = error;
                }
            }

            if (ImGui::MenuItem("Validate")) {
                if (GraphValidator::validate(document, _validationErrors)) {
                    _lastMessage = "Graph is valid";
                } else {
                    _lastMessage = "Graph has " + std::to_string(_validationErrors.size()) + " issue(s)";
                }
            }

            ImGui::EndMainMenuBar();
        }
    }

    void RenderGraphEditor::drawStatusBar(const GraphDocument& document) {
        ImGui::Begin("Status");
        ImGui::Text("Nodes: %d  Links: %d", static_cast<int>(document.nodes().size()),
                    static_cast<int>(document.links().size()));
        ImGui::TextWrapped("%s", _lastMessage.c_str());

        if (!_validationErrors.empty()) {
            ImGui::Separator();
            for (const std::string& err : _validationErrors) {
                ImGui::BulletText("%s", err.c_str());
            }
        }
        ImGui::End();
    }

    void RenderGraphEditor::draw(GraphDocument& document) {
        createStarterGraph(document);
        drawToolbar(document);

        ImGui::SetNextWindowPos(ImVec2(0.f, ImGui::GetFrameHeight()), ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight() - 120.f),
            ImGuiCond_Always);
        ImGui::Begin("Render Graph", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        drawNodeEditor(document);
        ImGui::End();

        drawStatusBar(document);
    }

}  // namespace mat::demo
