#include "app/DemoApp.h"

#include "editor/GraphCanvas.h"
#include "graph/DefaultDeferredScene.h"
#include "graph/GraphDocument.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace mat::demo {

    namespace {

        struct WglWindowData {
            HDC hDC = nullptr;
        };

        HGLRC g_renderContext = nullptr;
        int g_width = 1280;
        int g_height = 800;

        bool createDeviceWgl(HWND hwnd, WglWindowData* data) {
            HDC hdc = ::GetDC(hwnd);
            PIXELFORMATDESCRIPTOR pfd{};
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 32;

            const int pixelFormat = ::ChoosePixelFormat(hdc, &pfd);
            if (pixelFormat == 0) {
                return false;
            }
            if (::SetPixelFormat(hdc, pixelFormat, &pfd) == FALSE) {
                return false;
            }
            ::ReleaseDC(hwnd, hdc);

            data->hDC = ::GetDC(hwnd);
            if (!g_renderContext) {
                g_renderContext = wglCreateContext(data->hDC);
            }
            return g_renderContext != nullptr;
        }

        void cleanupDeviceWgl(HWND hwnd, WglWindowData* data) {
            wglMakeCurrent(nullptr, nullptr);
            ::ReleaseDC(hwnd, data->hDC);
            data->hDC = nullptr;
        }

        LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam) != 0) {
                return true;
            }

            switch (msg) {
                case WM_SIZE:
                    if (wParam != SIZE_MINIMIZED) {
                        g_width = LOWORD(lParam);
                        g_height = HIWORD(lParam);
                    }
                    return 0;
                case WM_SYSCOMMAND:
                    if ((wParam & 0xfff0) == SC_KEYMENU) {
                        return 0;
                    }
                    break;
                case WM_DESTROY:
                    ::PostQuitMessage(0);
                    return 0;
                default:
                    break;
            }
            return ::DefWindowProcW(hwnd, msg, wParam, lParam);
        }

    }  // namespace

    struct DemoApp::Impl {
        HWND hwnd = nullptr;
        WglWindowData windowData{};
        GridViewState gridView{};
        GraphPanelState panel{};
        GraphDocument document;
        bool running = false;
    };

    DemoApp::DemoApp() : _impl(new Impl()) {}

    DemoApp::~DemoApp() {
        shutdown();
        delete _impl;
        _impl = nullptr;
    }

    bool DemoApp::initialize() {
        ImGui_ImplWin32_EnableDpiAwareness();
        const float dpiScale =
            ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_OWNDC;
        windowClass.lpfnWndProc = wndProc;
        windowClass.hInstance = ::GetModuleHandle(nullptr);
        windowClass.lpszClassName = L"matVkEngineDemoClass";
        ::RegisterClassExW(&windowClass);

        _impl->hwnd = ::CreateWindowExW(0, windowClass.lpszClassName, L"matVkEngine Demo", WS_OVERLAPPEDWINDOW,
                                        100, 100, static_cast<int>(g_width * dpiScale),
                                        static_cast<int>(g_height * dpiScale), nullptr, nullptr,
                                        windowClass.hInstance, nullptr);

        if (_impl->hwnd == nullptr) {
            return false;
        }

        if (!createDeviceWgl(_impl->hwnd, &_impl->windowData)) {
            return false;
        }
        wglMakeCurrent(_impl->windowData.hDC, g_renderContext);

        ::ShowWindow(_impl->hwnd, SW_SHOWDEFAULT);
        ::UpdateWindow(_impl->hwnd);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(dpiScale);
        style.FontScaleDpi = dpiScale;

        ImGui_ImplWin32_InitForOpenGL(_impl->hwnd);
        ImGui_ImplOpenGL3_Init();

        loadDefaultDeferredScene(_impl->document, _impl->gridView);

        _impl->running = true;
        return true;
    }

    void DemoApp::run() {
        while (_impl->running) {
            MSG msg{};
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE) != 0) {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    _impl->running = false;
                }
            }

            if (!_impl->running) {
                break;
            }

            if (::IsIconic(_impl->hwnd) != 0) {
                ::Sleep(10);
                continue;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            const ImVec2 display = ImGui::GetIO().DisplaySize;
            drawGraphPanel(_impl->gridView, _impl->panel, _impl->document, display);

            ImGui::Render();
            glViewport(0, 0, g_width, g_height);
            glClearColor(0.11f, 0.11f, 0.13f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            ::SwapBuffers(_impl->windowData.hDC);
        }
    }

    void DemoApp::shutdown() {
        if (_impl == nullptr || _shutdownCalled || _impl->hwnd == nullptr) {
            return;
        }
        _shutdownCalled = true;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        cleanupDeviceWgl(_impl->hwnd, &_impl->windowData);
        if (g_renderContext != nullptr) {
            wglDeleteContext(g_renderContext);
            g_renderContext = nullptr;
        }

        ::DestroyWindow(_impl->hwnd);
        ::UnregisterClassW(L"matVkEngineDemoClass", ::GetModuleHandle(nullptr));
        _impl->hwnd = nullptr;
    }

}  // namespace mat::demo
