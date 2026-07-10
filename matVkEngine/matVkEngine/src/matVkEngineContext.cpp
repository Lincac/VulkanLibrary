#include "matVkEngineContext.h"

#include <algorithm>
#include <set>

namespace mat {

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

#ifdef _DEBUG
    const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

    static bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData) {
        fprintf(stderr, "[Vulkan Message]: %s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }

    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
#endif

    static bool checkDeviceExtensionSupport(VkPhysicalDevice device,
                                            const std::vector<const char*>& requiredExtensions) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> missing(requiredExtensions.begin(), requiredExtensions.end());
        for (const auto& extension : availableExtensions) {
            missing.erase(extension.extensionName);
        }

        return missing.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkEngineContext::VkEngineContext(std::optional<VkEngineSurface> surface) {
        VK_CHECK(volkInitialize());

        std::vector<const char*> requiredDeviceExtensions = {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
        if (surface.has_value()) {
            requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        std::vector<const char*> enabledInstanceExtensions;
        if (surface.has_value()) {
            enabledInstanceExtensions = surface->instanceExtensions;
        }

#ifdef _DEBUG
        for (const char* ext : instanceExtensions) {
            if (std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(), ext) ==
                enabledInstanceExtensions.end()) {
                enabledInstanceExtensions.push_back(ext);
            }
        }
#endif

#ifdef _DEBUG
        if (!checkValidationLayerSupport()) {
            VK_CHECK(VK_NOT_READY);
        }
#endif

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "vkEngine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "vkEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;
        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensions.size());
        instanceInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();

#ifdef _DEBUG
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = nullptr;
#endif

        VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &_instance));

        volkLoadInstance(_instance);

#ifdef _DEBUG
        populateDebugMessengerCreateInfo(debugCreateInfo);

        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            VK_CHECK(func(_instance, &debugCreateInfo, nullptr, &_debugMessenger));
        }
#endif

        if (surface.has_value()) {
            VK_CHECK(surface.value().createSurface(_instance, &_surface));
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            VK_CHECK(VK_NOT_READY);
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        for (const auto& device : devices) {
            graphicsFamily.reset();
            presentFamily.reset();

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                const auto& queueFamily = queueFamilies[i];

                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphicsFamily = i;
                }

                if (surface.has_value()) {
                    VkBool32 presentSupport = VK_FALSE;
                    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
                    if (presentSupport) {
                        presentFamily = i;
                    }
                }
            }

            const bool hasGraphics = graphicsFamily.has_value();
            const bool hasPresent = !surface.has_value() || presentFamily.has_value();
            const bool hasExtensions = checkDeviceExtensionSupport(device, requiredDeviceExtensions);

            bool hasSwapchainSupport = true;
            if (surface.has_value()) {
                SwapChainSupportDetails details = querySwapChainSupport(device, _surface);
                hasSwapchainSupport = !details.formats.empty() && !details.presentModes.empty();
            }

            if (hasGraphics && hasPresent && hasExtensions && hasSwapchainSupport) {
                _device = device;
                _graphicsFamily = graphicsFamily;
                if (surface.has_value()) {
                    _presentFamily = presentFamily;
                }
                break;
            }
        }

        if (_device == VK_NULL_HANDLE) {
            VK_CHECK(VK_NOT_READY);
        }

        std::set<uint32_t> uniqueQueueFamilies = {_graphicsFamily.value()};
        if (_presentFamily.has_value()) {
            uniqueQueueFamilies.insert(_presentFamily.value());
        }

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

        VkPhysicalDeviceFeatures2 deviceFeatures{};
        deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures.pNext = nullptr;

        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.pNext = &deviceFeatures;

        VK_CHECK(vkCreateDevice(_device, &deviceCreateInfo, nullptr, &_logDevice));

        vkGetDeviceQueue(_logDevice, _graphicsFamily.value(), 0, &_graphicsQueue);
        if (_presentFamily.has_value()) {
            vkGetDeviceQueue(_logDevice, _presentFamily.value(), 0, &_presentQueue);
        }

        volkLoadDevice(_logDevice);

        if (surface.has_value()) {
            if (!surface->queryFramebufferExtent) {
                VK_CHECK(VK_ERROR_INITIALIZATION_FAILED);
            }

            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_device, _surface);

            auto chooseSwapSurfaceFormat = [&](const std::vector<VkSurfaceFormatKHR>& availableFormats) {
                for (const auto& availableFormat : availableFormats) {
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                        return availableFormat;
                    }
                }

                return availableFormats[0];
            };

            auto chooseSwapPresentMode = [&](const std::vector<VkPresentModeKHR>& availablePresentModes) {
                for (const auto& availablePresentMode : availablePresentModes) {
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                        return availablePresentMode;
                    }
                }

                return VK_PRESENT_MODE_FIFO_KHR;
            };

            auto chooseSwapExtent = [&](const VkSurfaceCapabilitiesKHR& capabilities,
                                        const std::function<VkExtent2D()>& queryFramebufferExtent) {
                if (capabilities.currentExtent.width != UINT32_MAX) {
                    return capabilities.currentExtent;
                }

                VkExtent2D extent = queryFramebufferExtent();
                extent.width =
                    std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                extent.height =
                    std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                return extent;
            };

            auto chooseCompositeAlpha = [](const VkSurfaceCapabilitiesKHR& capabilities) {
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
                    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                }
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
                    return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
                }
                if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
                    return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
                }
                return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
            };

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, surface.value().queryFramebufferExtent);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
            if (swapChainSupport.capabilities.maxImageCount > 0 &&
                imageCount > swapChainSupport.capabilities.maxImageCount) {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR swapchainCreateInfo{};
            swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCreateInfo.surface = _surface;

            swapchainCreateInfo.minImageCount = imageCount;
            swapchainCreateInfo.imageFormat = surfaceFormat.format;
            swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
            swapchainCreateInfo.imageExtent = extent;
            swapchainCreateInfo.imageArrayLayers = 1;
            swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = {_graphicsFamily.value(), _presentFamily.value()};
            if (_graphicsFamily != _presentFamily) {
                swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainCreateInfo.queueFamilyIndexCount = 2;
                swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }

            swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            swapchainCreateInfo.compositeAlpha = chooseCompositeAlpha(swapChainSupport.capabilities);
            swapchainCreateInfo.presentMode = presentMode;
            swapchainCreateInfo.clipped = VK_TRUE;
            swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

            VK_CHECK(vkCreateSwapchainKHR(_logDevice, &swapchainCreateInfo, nullptr, &_swapChain));

            vkGetSwapchainImagesKHR(_logDevice, _swapChain, &imageCount, nullptr);
            _swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(_logDevice, _swapChain, &imageCount, _swapChainImages.data());

            _swapChainImageFormat = surfaceFormat.format;
            _swapChainExtent = extent;

            _swapChainImageViews.resize(_swapChainImages.size());
            for (int i = 0; i < _swapChainImageViews.size(); i++) {
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = _swapChainImages[i];
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = _swapChainImageFormat;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                VK_CHECK(vkCreateImageView(_logDevice, &viewInfo, nullptr, &_swapChainImageViews[i]));
            }
        }

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = _graphicsFamily.value();

        VK_CHECK(vkCreateCommandPool(_logDevice, &poolInfo, nullptr, &_commandPool));        
    }

    VkEngineContext::~VkEngineContext() {
        if (_swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(_logDevice, _swapChain, nullptr);
            _swapChain = VK_NULL_HANDLE;
        }

        if (_logDevice != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(_logDevice);

            if (_commandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(_logDevice, _commandPool, nullptr);
                _commandPool = VK_NULL_HANDLE;
            }

            vkDestroyDevice(_logDevice, nullptr);
            _logDevice = VK_NULL_HANDLE;
        }

#ifdef _DEBUG
        if (_debugMessenger != VK_NULL_HANDLE && _instance != VK_NULL_HANDLE) {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (func != nullptr) {
                func(_instance, _debugMessenger, nullptr);
            }

            _debugMessenger = VK_NULL_HANDLE;
        }
#endif  // _DEBUG

        if (_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(_instance, _surface, nullptr);
            _surface = VK_NULL_HANDLE;
        }

        if (_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(_instance, nullptr);
            _instance = VK_NULL_HANDLE;
        }
    }

    VkInstance VkEngineContext::getInstance() const {
        return _instance;
    }

    VkPhysicalDevice VkEngineContext::getPhysicalDevice() const {
        return _device;
    }

    VkDevice VkEngineContext::getDevice() const {
        return _logDevice;
    }

    VkCommandPool VkEngineContext::getCommandPool() const {
        return _commandPool;
    }

    VkQueue VkEngineContext::getGraphicsQueue() const {
        return _graphicsQueue;
    }

    VkQueue VkEngineContext::getPresentQueue() const {
        return _presentQueue;
    }

    VkSwapchainKHR VkEngineContext::getSwapChain() const {
        return _swapChain;
    }

    VkFormat VkEngineContext::getSwapChainImageFormat() const {
        return _swapChainImageFormat;
    }

    VkExtent2D VkEngineContext::getSwapChainImageExtent() const {
        return _swapChainExtent;
    }

    std::vector<VkImageView> VkEngineContext::getSwapChainImageViews() const {
        return _swapChainImageViews;
    }

};  // namespace mat