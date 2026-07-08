#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "matVkEngineContext.h"

int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("matVkEngineTriangle", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    Uint32 extCount = 0;

    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&extCount);

    mat::VkEngineSurface surf;
    surf.instanceExtensions.assign(sdlExtensions, sdlExtensions + extCount);
    surf.createSurface = [window](VkInstance inst, VkSurfaceKHR* out) -> VkResult {
        if (!SDL_Vulkan_CreateSurface(window, inst, nullptr, out)) {
            std::fprintf(stderr, "SDL_Vulkan_CreateSurface: %s\n", SDL_GetError());
            return VK_ERROR_INITIALIZATION_FAILED;
        }
        return VK_SUCCESS;
    };
    surf.queryFramebufferExtent = [window]() -> VkExtent2D {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);

        return VkExtent2D{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    };

    mat::VkEngineContext context(surf);

    while (true) {
        SDL_Event event;
        SDL_PollEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
            break;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}