#include "graph/GraphValidator.h"

#include "graph/GraphDocument.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mat::demo {

    namespace {

        bool pinKindMatches(PinKind from, PinKind to) {
            if (from == to) {
                return true;
            }
            if (from == PinKind::Color && to == PinKind::Resource) {
                return true;
            }
            if (from == PinKind::Depth && to == PinKind::Resource) {
                return true;
            }
            if (from == PinKind::Texture && to == PinKind::Resource) {
                return true;
            }
            return false;
        }

        const PinDesc* findPin(const GraphNode& node, int pinIndex, bool isOutput) {
            const auto& pins = isOutput ? node.outputs : node.inputs;
            if (pinIndex < 0 || pinIndex >= static_cast<int>(pins.size())) {
                return nullptr;
            }
            return &pins[pinIndex];
        }

        bool isPassStageNode(NodeType type) {
            return type == NodeType::DrawPass || type == NodeType::FullscreenPass || type == NodeType::UiPass;
        }

        std::optional<int> linkedSourceNodeId(const GraphDocument& doc, int nodeId, int inputPinIndex) {
            const int targetAttr = doc.attrFromPin(PinId{nodeId, inputPinIndex, false});
            for (const GraphLink& link : doc.links()) {
                if (link.endAttr != targetAttr) {
                    continue;
                }
                const auto from = doc.pinFromAttr(link.startAttr);
                if (from && from->isOutput) {
                    return from->nodeId;
                }
            }
            return std::nullopt;
        }

        bool hasExecCycle(const GraphDocument& doc) {
            std::unordered_map<int, std::vector<int>> adjacency;
            for (const GraphLink& link : doc.links()) {
                const auto from = doc.pinFromAttr(link.startAttr);
                const auto to = doc.pinFromAttr(link.endAttr);
                if (!from || !to) {
                    continue;
                }

                const GraphNode* fromNode = doc.findNode(from->nodeId);
                const GraphNode* toNode = doc.findNode(to->nodeId);
                if (!fromNode || !toNode) {
                    continue;
                }

                const PinDesc* fromPin = findPin(*fromNode, from->pinIndex, true);
                const PinDesc* toPin = findPin(*toNode, to->pinIndex, false);
                if (!fromPin || !toPin || fromPin->kind != PinKind::Exec || toPin->kind != PinKind::Exec) {
                    continue;
                }

                adjacency[from->nodeId].push_back(to->nodeId);
            }

            std::unordered_set<int> visiting;
            std::unordered_set<int> visited;

            const auto dfs = [&](auto&& self, int nodeId) -> bool {
                if (visiting.count(nodeId) != 0) {
                    return true;
                }
                if (visited.count(nodeId) != 0) {
                    return false;
                }

                visiting.insert(nodeId);
                for (int next : adjacency[nodeId]) {
                    if (self(self, next)) {
                        return true;
                    }
                }
                visiting.erase(nodeId);
                visited.insert(nodeId);
                return false;
            };

            for (const GraphNode& node : doc.nodes()) {
                if (dfs(dfs, node.id)) {
                    return true;
                }
            }
            return false;
        }

        void validatePassStages(const GraphDocument& doc, std::vector<std::string>& errors) {
            std::vector<const GraphNode*> stages;
            for (const GraphNode& node : doc.nodes()) {
                if (isPassStageNode(node.type)) {
                    stages.push_back(&node);
                }
            }

            if (stages.empty()) {
                errors.push_back("No pass stage node (Draw / Fullscreen / UI Pass)");
                return;
            }

            for (const GraphNode* stage : stages) {
                const auto stageName = stage->props.count("name") != 0 ? stage->props.at("name") : "stage";
                if (!linkedSourceNodeId(doc, stage->id, 1)) {
                    errors.push_back(std::string(nodeTypeName(stage->type)) + " \"" + stageName +
                                     "\" missing Render Pass");
                }
                if (!linkedSourceNodeId(doc, stage->id, 2)) {
                    errors.push_back(std::string(nodeTypeName(stage->type)) + " \"" + stageName +
                                     "\" missing Subpass");
                }
                if (!linkedSourceNodeId(doc, stage->id, 3)) {
                    errors.push_back(std::string(nodeTypeName(stage->type)) + " \"" + stageName +
                                     "\" missing Graphics Pipeline");
                }
                if (stage->type == NodeType::DrawPass && !linkedSourceNodeId(doc, stage->id, 4)) {
                    errors.push_back("Draw Pass \"" + stageName + "\" missing Scene");
                }
            }

            std::unordered_map<int, std::vector<std::pair<int, int>>> subpassesByRenderPass;
            for (const GraphNode* stage : stages) {
                const auto renderPassId = linkedSourceNodeId(doc, stage->id, 1);
                const auto subpassId = linkedSourceNodeId(doc, stage->id, 2);
                if (!renderPassId || !subpassId) {
                    continue;
                }

                const GraphNode* subpassNode = doc.findNode(*subpassId);
                if (!subpassNode) {
                    continue;
                }

                int index = 0;
                const auto indexIter = subpassNode->props.find("index");
                if (indexIter != subpassNode->props.end()) {
                    index = std::stoi(indexIter->second);
                }
                subpassesByRenderPass[*renderPassId].emplace_back(index, stage->id);
            }

            for (const auto& [renderPassId, usedSubpasses] : subpassesByRenderPass) {
                if (usedSubpasses.size() < 2) {
                    continue;
                }

                std::vector<std::pair<int, int>> ordered = usedSubpasses;
                std::sort(ordered.begin(), ordered.end());

                std::unordered_set<int> seen;
                for (const auto& [index, stageId] : ordered) {
                    if (seen.count(index) != 0) {
                        const GraphNode* rp = doc.findNode(renderPassId);
                        const std::string rpName =
                            rp && rp->props.count("name") != 0 ? rp->props.at("name") : "render pass";
                        errors.push_back("Duplicate subpass index " + std::to_string(index) + " on Render Pass \"" +
                                         rpName + "\"");
                    }
                    seen.insert(index);
                }

                for (size_t i = 1; i < ordered.size(); ++i) {
                    if (ordered[i].first < ordered[i - 1].first) {
                        errors.push_back("Subpass indices must ascend within the same Render Pass");
                        break;
                    }
                }
            }
        }

        void validateExecChain(const GraphDocument& doc, std::vector<std::string>& errors) {
            std::unordered_map<int, int> execInDegree;
            std::unordered_map<int, int> execOutDegree;
            std::unordered_set<int> stageIds;

            for (const GraphNode& node : doc.nodes()) {
                if (isPassStageNode(node.type)) {
                    stageIds.insert(node.id);
                    execInDegree[node.id] = 0;
                    execOutDegree[node.id] = 0;
                }
            }

            for (const GraphLink& link : doc.links()) {
                const auto from = doc.pinFromAttr(link.startAttr);
                const auto to = doc.pinFromAttr(link.endAttr);
                if (!from || !to) {
                    continue;
                }

                const GraphNode* fromNode = doc.findNode(from->nodeId);
                const GraphNode* toNode = doc.findNode(to->nodeId);
                if (!fromNode || !toNode) {
                    continue;
                }

                const PinDesc* fromPin = findPin(*fromNode, from->pinIndex, true);
                const PinDesc* toPin = findPin(*toNode, to->pinIndex, false);
                if (!fromPin || !toPin || fromPin->kind != PinKind::Exec || toPin->kind != PinKind::Exec) {
                    continue;
                }

                if (stageIds.count(from->nodeId) != 0 && stageIds.count(to->nodeId) != 0) {
                    execOutDegree[from->nodeId]++;
                    execInDegree[to->nodeId]++;
                }
            }

            if (stageIds.empty()) {
                return;
            }

            int entryCount = 0;
            int exitCount = 0;
            for (int stageId : stageIds) {
                if (execInDegree[stageId] == 0) {
                    entryCount++;
                }
                if (execOutDegree[stageId] == 0) {
                    exitCount++;
                }
            }

            if (stageIds.size() > 1) {
                if (entryCount != 1) {
                    errors.push_back("Pass exec chain must have exactly one entry stage (found " +
                                     std::to_string(entryCount) + ")");
                }
                if (exitCount != 1) {
                    errors.push_back("Pass exec chain must have exactly one exit stage (found " +
                                     std::to_string(exitCount) + ")");
                }
            }
        }

        void validateRenderPassIndices(const GraphDocument& doc, std::vector<std::string>& errors) {
            std::vector<std::pair<int, std::string>> passes;
            for (const GraphNode& node : doc.nodes()) {
                if (node.type != NodeType::RenderPass) {
                    continue;
                }
                int passIndex = 0;
                const auto iter = node.props.find("passIndex");
                if (iter != node.props.end()) {
                    passIndex = std::stoi(iter->second);
                }
                passes.emplace_back(passIndex, node.props.count("name") != 0 ? node.props.at("name") : "render pass");
            }

            if (passes.size() < 2) {
                return;
            }

            std::sort(passes.begin(), passes.end());
            for (size_t i = 1; i < passes.size(); ++i) {
                if (passes[i].first == passes[i - 1].first) {
                    errors.push_back("Duplicate Render Pass passIndex: " + std::to_string(passes[i].first));
                }
            }
        }

    }  // namespace

    bool GraphValidator::isCompatible(PinKind from, PinKind to) {
        return pinKindMatches(from, to);
    }

    bool GraphValidator::canConnect(const GraphDocument& doc, const PinId& from, const PinId& to,
                                    std::string& error) {
        if (from.nodeId == to.nodeId) {
            error = "Cannot connect a node to itself";
            return false;
        }

        const GraphNode* fromNode = doc.findNode(from.nodeId);
        const GraphNode* toNode = doc.findNode(to.nodeId);
        if (!fromNode || !toNode) {
            error = "Node not found";
            return false;
        }

        const PinDesc* fromPin = findPin(*fromNode, from.pinIndex, true);
        const PinDesc* toPin = findPin(*toNode, to.pinIndex, false);
        if (!fromPin || !toPin) {
            error = "Pin not found";
            return false;
        }

        if (!pinKindMatches(fromPin->kind, toPin->kind)) {
            error = "Pin type mismatch";
            return false;
        }

        if (fromPin->kind == PinKind::Exec || fromPin->kind == PinKind::Resource || fromPin->kind == PinKind::Color ||
            fromPin->kind == PinKind::Depth || fromPin->kind == PinKind::RenderPass ||
            fromPin->kind == PinKind::Pipeline || fromPin->kind == PinKind::Scene ||
            fromPin->kind == PinKind::Subpass) {
            for (const GraphLink& link : doc.links()) {
                if (link.startAttr != doc.attrFromPin(from)) {
                    continue;
                }

                const auto existingTo = doc.pinFromAttr(link.endAttr);
                if (!existingTo) {
                    continue;
                }

                const GraphNode* existingToNode = doc.findNode(existingTo->nodeId);
                if (!existingToNode) {
                    continue;
                }

                const PinDesc* existingToPin = findPin(*existingToNode, existingTo->pinIndex, false);
                if (!existingToPin) {
                    continue;
                }

                if (fromPin->kind == PinKind::Exec && existingToPin->kind == PinKind::Exec &&
                    existingTo->nodeId == to.nodeId) {
                    error = "Exec input already connected";
                    return false;
                }

                if (fromPin->kind != PinKind::Exec && fromPin->kind != PinKind::Resource &&
                    fromPin->kind != PinKind::Color && fromPin->kind != PinKind::Depth &&
                    existingTo->nodeId == to.nodeId && existingTo->pinIndex == to.pinIndex) {
                    error = "Input pin already connected";
                    return false;
                }
            }
        }

        return true;
    }

    bool GraphValidator::validate(const GraphDocument& doc, std::vector<std::string>& errors) {
        errors.clear();

        if (hasExecCycle(doc)) {
            errors.push_back("Exec link cycle detected");
        }

        bool hasOutput = false;
        for (const GraphNode& node : doc.nodes()) {
            if (node.type == NodeType::OutputPng) {
                hasOutput = true;
            }
        }
        if (!hasOutput) {
            errors.push_back("Missing Output PNG node");
        }

        validatePassStages(doc, errors);
        validateExecChain(doc, errors);
        validateRenderPassIndices(doc, errors);

        return errors.empty();
    }

}  // namespace mat::demo
