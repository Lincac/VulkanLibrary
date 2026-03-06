#pragma once

#include <string>
#include <vector>

#include <volk/volk.h>

class Instance
{
public:

    Instance(
        const std::string& applicationName,
        bool enableValidationLayers = true);

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&& other) noexcept = default;
    Instance& operator=(Instance&& other) noexcept = default;
    ~Instance() = default;

    VkInstance getInstance() const noexcept;

private:

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

private:

    bool _enableValidationLayers;
    const std::vector<const char*> _validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerEXT _debugMessenger;

    VkInstance _instance;

};