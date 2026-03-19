#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <volk/volk.h>

#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <array>

#include <glm/glm.hpp>

const int MAX_FRAMES_IN_FLIGHT = 2;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

enum ShaderType
{
    Vertex,
    Fragment,
    Geometry,
    Compute
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, tangent);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, uv);

        return attributeDescriptions;
    }
};

class vkEngine
{
public:

    vkEngine(const std::string& appName, GLFWwindow* window);
    ~vkEngine();

    vkEngine(const vkEngine&) = delete;
    vkEngine& operator=(const vkEngine&) = delete;

    int init();

public:

    GLFWwindow* getWindow() const noexcept;

    VkInstance getInstance() const noexcept;

    VkSurfaceKHR getSurface() const noexcept;
    
    VkPhysicalDevice getPhysicalDevice() const noexcept;

    VkDevice getLogicalDevice() const noexcept;

    VkSwapchainKHR getSwapChain() const noexcept;

    VkFormat getSwapChainImageFormat() const noexcept;

    uint32_t getSwapChainImageCount() const noexcept;

    VkImageView getSwapChainImageView(uint32_t index) const noexcept;

    const VkCommandBuffer getCommandBuffer(uint32_t frame);

    VkCommandBuffer* getCommandBuffers();

    VkQueue getGraphicsQueue() const noexcept;

    VkQueue getPresentQueue() const noexcept;

    VkExtent2D getSwapChainExtent() const noexcept;

    void resetSwapChain();

private:

    void initInstance();

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

    void initPhysicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    void initLogicalDevice();

    void initSwapChain();

    void cleanupSwapChain();

    SwapChainSupportDetails querySwapChainSupport();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

public:

    VkShaderModule createShader(const std::string& path, ShaderType type);

    std::vector<char> readSpirvFile(const std::string& spirvPath);

    VkShaderStageFlagBits toVkShaderStage(ShaderType shaderType);

private:

    void initCommandPool();

public:

    void createVKBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
        VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

private:

    std::string _applicationName;
    GLFWwindow* _window;

    // 
    bool _enableValidationLayers;
    const std::vector<const char*> _validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerEXT _debugMessenger;

    VkInstance _instance;

    //
    VkSurfaceKHR _surface;

    // 
    const std::vector<const char*> _deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDevice _physicalDevice;
    
    //
    VkDevice _logicalDevice;

    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

    //
    VkSwapchainKHR _swapChain;
    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> _swapChainImageViews;

    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;

    //
    VkRenderPass _opaqueRenderPass;
    
    //
    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;
};