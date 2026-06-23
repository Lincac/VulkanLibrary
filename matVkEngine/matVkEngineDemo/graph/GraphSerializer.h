#pragma once

#include "graph/GraphTypes.h"

#include <string>

namespace mat::demo {

    class GraphDocument;

    class GraphSerializer {
    public:
        static bool save(const GraphDocument& doc, const std::string& path, std::string& error);
        static bool load(GraphDocument& doc, const std::string& path, std::string& error);
    };

}  // namespace mat::demo
