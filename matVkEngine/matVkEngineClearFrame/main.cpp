#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "matVkEngineContext.h"

#define MAX_FRAMES_IN_FLIGHT 2

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

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = context.getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    VK_CHECK(vkCreateRenderPass(context.getDevice(), &renderPassInfo, nullptr, &renderPass));

    const std::vector<VkImageView> swapChainImageViews = context.getSwapChainImageViews();
    const VkExtent2D swapChainExtent = context.getSwapChainImageExtent();

    std::vector<VkFramebuffer> framebuffers;
    framebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapChainImageViews[i];
        fbInfo.width = swapChainExtent.width;
        fbInfo.height = swapChainExtent.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(context.getDevice(), &fbInfo, nullptr, &framebuffers[i]);
    }

    std::vector<VkCommandBuffer> commandBuffers;
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.getVkCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    VK_CHECK(vkAllocateCommandBuffers(context.getDevice(), &allocInfo, commandBuffers.data()));

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(swapChainImageViews.size());
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
        VK_CHECK(vkCreateFence(context.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]));
    }
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VK_CHECK(vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
    }

    uint32_t curFrame = 0;
    const VkSwapchainKHR swapChain = context.getSwapChain();
    auto drawFunc = [&]() {
        vkWaitForFences(context.getDevice(), 1, &inFlightFences[curFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex = 0;
        vkAcquireNextImageKHR(context.getDevice(), swapChain, UINT64_MAX, imageAvailableSemaphores[curFrame],
                              VK_NULL_HANDLE, &imageIndex);

        vkResetFences(context.getDevice(), 1, &inFlightFences[curFrame]);

        VkCommandBuffer cmdBuffer = commandBuffers[curFrame];
        vkResetCommandBuffer(cmdBuffer, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(cmdBuffer, &beginInfo);

        VkClearValue clearValue{};
        clearValue.color = {{1, 0, 1, 1}};

        VkRenderPassBeginInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass;
        rpInfo.framebuffer = framebuffers[imageIndex];
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = context.getSwapChainImageExtent();
        rpInfo.clearValueCount = 1;
        rpInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(cmdBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(cmdBuffer);  // 不调用 vkCmdDraw，只靠 CLEAR 上色

        vkEndCommandBuffer(cmdBuffer);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailableSemaphores[curFrame];
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmdBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];
        vkQueueSubmit(context.getGraphicsQueue(), 1, &submit, inFlightFences[curFrame]);
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
        present.swapchainCount = 1;
        present.pSwapchains = &swapChain;
        present.pImageIndices = &imageIndex;
        vkQueuePresentKHR(context.getPresentQueue(), &present);
        curFrame = (curFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    };

    while (true) {
        SDL_Event event;
        SDL_PollEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            break;
        }

        drawFunc();
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}