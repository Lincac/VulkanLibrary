#pragma once

#include "graph/GraphDocument.h"

#include <string>
#include <vector>

struct ImVec2;

namespace mat::demo {

    class RenderGraphEditor {
    public:
        void initialize();
        void shutdown();

        void draw(GraphDocument& document);

        const std::string& lastMessage() const { return _lastMessage; }

    private:
        void drawToolbar(GraphDocument& document);
        void drawNodeEditor(GraphDocument& document);
        void drawStatusBar(const GraphDocument& document);
        void drawNode(GraphNode& node);
        void handleInteraction(GraphDocument& document);
        void createStarterGraph(GraphDocument& document);

        int _selectedNodeId = -1;
        std::string _graphPath = "graphs/default.graph.json";
        std::string _lastMessage;
        std::vector<std::string> _validationErrors;
        bool _initialized = false;
    };

}  // namespace mat::demo
