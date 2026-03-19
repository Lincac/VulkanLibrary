#include "Render.h"

#include <stdexcept>
#include <cstring>
#include "Render_Opaque.h"

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec4 cameraPos;
    alignas(16) glm::vec4 lightDir;
    alignas(16) glm::vec4 lightColor;
};

Render::Render(const std::string& appName, GLFWwindow* window)
{
    _engine = new vkEngine(appName, window);
    _engine->init();

    auto extent = _engine->getSwapChainExtent();

    _camera = new Camera(glm::vec3(0, 0, 3));
    _camera->SetResolution(extent.width, extent.height);

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
    cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraLayoutInfo.bindingCount = 1;
    cameraLayoutInfo.pBindings = &uboBinding;

    if (vkCreateDescriptorSetLayout(_engine->getLogicalDevice(), &cameraLayoutInfo, nullptr, &_sceneDescSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create camera descriptor set layout!");
    }

    initSyncObjects();
    initSceneDescriptorResources();

    _opaqueRender = new Render_Opaque(_engine, _sceneDescSetLayout);
}

Render::~Render()
{
    if (_engine == nullptr) {
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    cleanupSceneDescriptorResources();
    cleanupSyncObjects();

    delete _opaqueRender;
    _opaqueRender = nullptr;

    delete _camera;
    _camera = nullptr;

    delete _engine;
    _engine = nullptr;
}

void Render::addActor(Actor* actor)
{
    _opaqueRender->addActor(actor);
}

Camera* Render::getCamera()
{
    return _camera;
}

void Render::draw()
{
    vkWaitForFences(_engine->getLogicalDevice(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_engine->getLogicalDevice(), _engine->getSwapChain(), 
        UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChainResources();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto commandBuffer = _engine->getCommandBuffer(_currentFrame);

    vkResetFences(_engine->getLogicalDevice(), 1, &_inFlightFences[_currentFrame]);
    vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

    _opaqueRender->draw(commandBuffer, _sceneDescSets[_currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_engine->getGraphicsQueue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { _engine->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(_engine->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChainResources();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Render::initSceneDescriptorResources()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _sceneUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _sceneUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    _sceneUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    _sceneDescSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _engine->createVKBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _sceneUniformBuffers[i], _sceneUniformBuffersMemory[i]);

        vkMapMemory(_engine->getLogicalDevice(), _sceneUniformBuffersMemory[i],
            0, bufferSize, 0, &_sceneUniformBuffersMapped[i]);
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(_engine->getLogicalDevice(), &poolInfo, nullptr, &_sceneDescPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create camera descriptor pool!");
    }

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _sceneDescSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _sceneDescPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_engine->getLogicalDevice(), &allocInfo, _sceneDescSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate camera descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _sceneUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _sceneDescSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(_engine->getLogicalDevice(), 1, &write, 0, nullptr);
    }
}

void Render::initSyncObjects()
{
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(_engine->getLogicalDevice(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void Render::cleanupSyncObjects()
{
    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (auto fence : _inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, fence, nullptr);
        }
    }

    for (auto semaphore : _renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    for (auto semaphore : _imageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    _inFlightFences.clear();
    _renderFinishedSemaphores.clear();
    _imageAvailableSemaphores.clear();
}

void Render::cleanupSceneDescriptorResources()
{
    if (_engine == nullptr) {
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (size_t i = 0; i < _sceneUniformBuffers.size(); ++i) {
        if (_sceneUniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(device, _sceneUniformBuffersMemory[i]);
            _sceneUniformBuffersMapped[i] = nullptr;
        }
        if (_sceneUniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, _sceneUniformBuffers[i], nullptr);
        }
        if (_sceneUniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, _sceneUniformBuffersMemory[i], nullptr);
        }
    }
    _sceneUniformBuffers.clear();
    _sceneUniformBuffersMemory.clear();
    _sceneUniformBuffersMapped.clear();
    _sceneDescSets.clear();

    if (_sceneDescPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _sceneDescPool, nullptr);
        _sceneDescPool = VK_NULL_HANDLE;
    }
}

void Render::recreateSwapChainResources()
{
    _opaqueRender->cleanup();
    _engine->resetSwapChain();
    _opaqueRender->init();
}