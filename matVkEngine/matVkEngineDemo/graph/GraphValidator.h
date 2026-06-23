#pragma once

#include "graph/GraphTypes.h"

#include <string>
#include <vector>

namespace mat::demo {

    class GraphDocument;

    class GraphValidator {
    public:
        static bool canConnect(const GraphDocument& doc, const PinId& from, const PinId& to, std::string& error);
        static bool validate(const GraphDocument& doc, std::vector<std::string>& errors);
        static bool isCompatible(PinKind from, PinKind to);
    };

}  // namespace mat::demo
