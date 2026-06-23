#include "graph/GraphValidator.h"

#include "graph/GraphDocument.h"

#include <unordered_map>
#include <unordered_set>

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
            return false;
        }

        const PinDesc* findPin(const GraphNode& node, int pinIndex, bool isOutput) {
            const auto& pins = isOutput ? node.outputs : node.inputs;
            if (pinIndex < 0 || pinIndex >= static_cast<int>(pins.size())) {
                return nullptr;
            }
            return &pins[pinIndex];
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
            fromPin->kind == PinKind::Depth) {
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
            }
        }

        if (fromPin->kind == PinKind::Resource || fromPin->kind == PinKind::Color || fromPin->kind == PinKind::Depth) {
            // Resource ordering against exec chain is validated in full graph validate (later).
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

        return errors.empty();
    }

}  // namespace mat::demo
