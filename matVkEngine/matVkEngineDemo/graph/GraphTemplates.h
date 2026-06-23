#pragma once

#include "graph/GraphDocument.h"

namespace mat::demo {

    class GraphTemplates {
    public:
        static void buildForward(GraphDocument& document);
        static void buildDeferredMultiPass(GraphDocument& document);
    };

}  // namespace mat::demo
