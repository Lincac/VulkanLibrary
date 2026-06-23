#include "editor/RenderGraphEditor.h"

#include "editor/NodeRegistry.h"
#include "graph/GraphSerializer.h"
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

        const int renderConfig = document.addNode(NodeType::RenderConfig, 40.f, 80.f);
        const int material = document.addNode(NodeType::Material, 40.f, 280.f);
        const int entity = document.addNode(NodeType::Entity, 280.f, 280.f);
        const int camera = document.addNode(NodeType::Camera, 40.f, 460.f);
        const int scene = document.addNode(NodeType::Scene, 520.f, 280.f);
        const int drawPass = document.addNode(NodeType::DrawPass, 520.f, 80.f);
        const int output = document.addNode(NodeType::OutputPng, 820.f, 80.f);

        std::string error;
        document.tryConnect(NodeRegistry::encodeAttr(material, 0, true), NodeRegistry::encodeAttr(entity, 0, false),
                            error);
        document.tryConnect(NodeRegistry::encodeAttr(entity, 0, true), NodeRegistry::encodeAttr(scene, 0, false),
                            error);
        document.tryConnect(NodeRegistry::encodeAttr(camera, 0, true), NodeRegistry::encodeAttr(scene, 1, false),
                            error);
        document.tryConnect(NodeRegistry::encodeAttr(drawPass, 0, true), NodeRegistry::encodeAttr(output, 0, false),
                            error);
        document.tryConnect(NodeRegistry::encodeAttr(drawPass, 1, true), NodeRegistry::encodeAttr(output, 1, false),
                            error);

        ImNodes::SetNodeGridSpacePos(renderConfig, ImVec2(40.f, 80.f));
        ImNodes::SetNodeGridSpacePos(material, ImVec2(40.f, 280.f));
        ImNodes::SetNodeGridSpacePos(entity, ImVec2(280.f, 280.f));
        ImNodes::SetNodeGridSpacePos(camera, ImVec2(40.f, 460.f));
        ImNodes::SetNodeGridSpacePos(scene, ImVec2(520.f, 280.f));
        ImNodes::SetNodeGridSpacePos(drawPass, ImVec2(520.f, 80.f));
        ImNodes::SetNodeGridSpacePos(output, ImVec2(820.f, 80.f));

        _lastMessage = "Created starter graph";
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
                if (ImGui::MenuItem("Render Config")) {
                    document.addNode(NodeType::RenderConfig, mouse.x, mouse.y);
                }
                if (ImGui::MenuItem("Draw Pass")) {
                    document.addNode(NodeType::DrawPass, mouse.x, mouse.y);
                }
                if (ImGui::MenuItem("Fullscreen Pass")) {
                    document.addNode(NodeType::FullscreenPass, mouse.x, mouse.y);
                }
                if (ImGui::MenuItem("Material")) {
                    document.addNode(NodeType::Material, mouse.x, mouse.y);
                }
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
                if (ImGui::MenuItem("Output PNG")) {
                    document.addNode(NodeType::OutputPng, mouse.x, mouse.y);
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
