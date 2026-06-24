#include "app/DemoApp.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    mat::demo::DemoApp app;
    if (!app.initialize()) {
        return 1;
    }

    app.run();
    app.shutdown();
    return 0;
}
