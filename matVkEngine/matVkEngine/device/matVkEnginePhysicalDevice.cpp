#include "matVkEnginePhysicalDevice.h"

#include <set>
#include <stdexcept>

namespace mat {

    static bool checkDeviceExtensionSupport(VkPhysicalDevice device, const VkEnginePhysicalDeviceConfig& config) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(config.extensions.begin(), config.extensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    VkEnginePhysicalDevice::VkEnginePhysicalDevice(std::shared_ptr<VkEngineContext> context,
                                                   const VkEnginePhysicalDeviceConfig& config)
        : _config(config) {
        if (context->getVkInstance() == VK_NULL_HANDLE) {
            throw std::runtime_error("Vulkan instantiation not created!");
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(context->getVkInstance(), &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(context->getVkInstance(), &deviceCount, devices.data());

        for (const auto& device : devices) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            for (uint32_t i = 0; i < queueFamilyCount; ++i) {
                const auto& queueFamily = queueFamilies[i];

                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    _graphicsFamily = i;
                }

                if (_graphicsFamily.has_value() && checkDeviceExtensionSupport(device, _config)) {
                    _device = device;
                    break;
                }
            }
        }

        if (_device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    VkEnginePhysicalDevice::~VkEnginePhysicalDevice() {
        _device = VK_NULL_HANDLE;
    }

    VkPhysicalDevice VkEnginePhysicalDevice::getVkPhysicalDevice() const {
        return _device;
    }

    std::optional<uint32_t> VkEnginePhysicalDevice::getGraphicsFamily() const {
        return _graphicsFamily;
    }

    VkEnginePhysicalDeviceConfig VkEnginePhysicalDevice::getConfig() const {
        return _config;
    }

};  // namespace mat