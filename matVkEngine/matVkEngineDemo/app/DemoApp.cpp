#include "app/DemoApp.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

#include <cmath>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace mat::demo {

    namespace {

        struct GridViewState {
            ImVec2 pan{0.f, 0.f};
            float zoom = 1.f;
        };

        void drawInfiniteGrid(const ImVec2& origin, const ImVec2& size, const GridViewState& view) {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            const ImU32 bgColor = IM_COL32(28, 28, 34, 255);
            const ImU32 minorColor = IM_COL32(52, 52, 62, 255);
            const ImU32 majorColor = IM_COL32(78, 78, 94, 255);
            const ImU32 axisColor = IM_COL32(110, 90, 140, 255);

            drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), bgColor);

            const ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);

            float worldStep = 32.f;
            float screenStep = worldStep * view.zoom;
            constexpr float kMinStep = 12.f;
            constexpr float kMaxStep = 128.f;
            while (screenStep < kMinStep) {
                worldStep *= 2.f;
                screenStep *= 2.f;
            }
            while (screenStep > kMaxStep) {
                worldStep *= 0.5f;
                screenStep *= 0.5f;
            }

            const int majorEvery = 5;

            const float worldLeft = (origin.x - center.x - view.pan.x) / view.zoom;
            const float worldRight = (origin.x + size.x - center.x - view.pan.x) / view.zoom;
            const float worldTop = (origin.y - center.y - view.pan.y) / view.zoom;
            const float worldBottom = (origin.y + size.y - center.y - view.pan.y) / view.zoom;

            auto drawAxisLine = [&](bool vertical, float screenPos) {
                if (vertical) {
                    drawList->AddLine(ImVec2(screenPos, origin.y), ImVec2(screenPos, origin.y + size.y), axisColor,
                                      1.5f);
                } else {
                    drawList->AddLine(ImVec2(origin.x, screenPos), ImVec2(origin.x + size.x, screenPos), axisColor,
                                      1.5f);
                }
            };

            const float startX = std::floor(worldLeft / worldStep) * worldStep;
            for (float worldX = startX; worldX <= worldRight + worldStep; worldX += worldStep) {
                const float screenX = center.x + worldX * view.zoom + view.pan.x;
                if (screenX < origin.x - 1.f || screenX > origin.x + size.x + 1.f) {
                    continue;
                }

                const int index = static_cast<int>(std::llround(worldX / worldStep));
                const bool onAxis = std::fabs(worldX) < worldStep * 0.5f;
                const bool isMajor = (index % majorEvery) == 0;

                if (onAxis) {
                    drawAxisLine(true, screenX);
                    continue;
                }

                const ImU32 color = isMajor ? majorColor : minorColor;
                const float thickness = isMajor ? 1.2f : 1.f;
                drawList->AddLine(ImVec2(screenX, origin.y), ImVec2(screenX, origin.y + size.y), color, thickness);
            }

            const float startY = std::floor(worldTop / worldStep) * worldStep;
            for (float worldY = startY; worldY <= worldBottom + worldStep; worldY += worldStep) {
                const float screenY = center.y + worldY * view.zoom + view.pan.y;
                if (screenY < origin.y - 1.f || screenY > origin.y + size.y + 1.f) {
                    continue;
                }

                const int index = static_cast<int>(std::llround(worldY / worldStep));
                const bool onAxis = std::fabs(worldY) < worldStep * 0.5f;
                const bool isMajor = (index % majorEvery) == 0;

                if (onAxis) {
                    drawAxisLine(false, screenY);
                    continue;
                }

                const ImU32 color = isMajor ? majorColor : minorColor;
                const float thickness = isMajor ? 1.2f : 1.f;
                drawList->AddLine(ImVec2(origin.x, screenY), ImVec2(origin.x + size.x, screenY), color, thickness);
            }
        }

        void updateGridView(GridViewState& view, const ImVec2& displaySize) {
            ImGuiIO& io = ImGui::GetIO();
            const ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                view.pan.x += io.MouseDelta.x;
                view.pan.y += io.MouseDelta.y;
            }

            if (io.MouseWheel != 0.f) {
                const float zoomFactor = std::pow(1.12f, io.MouseWheel);
                const ImVec2 mouse = io.MousePos;
                const float worldX = (mouse.x - center.x - view.pan.x) / view.zoom;
                const float worldY = (mouse.y - center.y - view.pan.y) / view.zoom;
                view.zoom *= zoomFactor;
                if (view.zoom < 0.15f) {
                    view.zoom = 0.15f;
                }
                if (view.zoom > 8.f) {
                    view.zoom = 8.f;
                }
                view.pan.x = mouse.x - center.x - worldX * view.zoom;
                view.pan.y = mouse.y - center.y - worldY * view.zoom;
            }
        }

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

            updateGridView(_impl->gridView, ImGui::GetIO().DisplaySize);
            const ImVec2 display = ImGui::GetIO().DisplaySize;
            drawInfiniteGrid(ImVec2(0.f, 0.f), display, _impl->gridView);

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
