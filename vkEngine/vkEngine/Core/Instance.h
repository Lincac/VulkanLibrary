#pragma once

#include <string>
#include <vector>

#include <volk/volk.h>

class Instance
{
public:

    Instance();

    void setApplicationName(const std::string& appName);

    int create();

    VkInstance getInstance() const noexcept;

private:

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void setupDebugMessenger();

private:

    std::string _applicationName;

    bool _enableValidationLayers;
    const std::vector<const char*> _validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerEXT _debugMessenger;

    VkInstance _instance;

};