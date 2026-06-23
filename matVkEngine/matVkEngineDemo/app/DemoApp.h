#pragma once

namespace mat::demo {

    class RenderGraphEditor;

    class DemoApp {
    public:
        DemoApp();
        ~DemoApp();

        bool initialize();
        void run();
        void shutdown();

    private:
        struct Impl;
        Impl* _impl = nullptr;
        bool _shutdownCalled = false;
    };

}  // namespace mat::demo
