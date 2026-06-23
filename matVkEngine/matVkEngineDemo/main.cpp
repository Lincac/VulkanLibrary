#include "app/DemoApp.h"

int main() {
    mat::demo::DemoApp app;
    if (!app.initialize()) {
        return 1;
    }

    app.run();
    app.shutdown();
    return 0;
}
