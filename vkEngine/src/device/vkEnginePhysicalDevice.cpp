#include "device/vkEnginePhysicalDevice.h"

namespace {

bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extensions)
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, const vkEnginePhysicalDeviceReq& req)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        const auto& queueFamily = queueFamilies[i];

        if (req.enableGraphicsFamily
            && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            && !indices.graphicsFamily) {
            indices.graphicsFamily = i;
        }

        // VkBool32 enablePresent = false;
        // vkGetPhysicalDeviceSurfaceSupportKHR(device, i, , &enablePresent);
        // if(enablePresent)
        // {
        //     indices.presentFamily = i;
        // }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device, const vkEnginePhysicalDeviceReq& req)
{
    if (!findQueueFamilies(device, req).isComplete()) {
        return false;
    }

    return checkDeviceExtensionSupport(device, req.extensions);
}

} // namespace

vkEnginePhysicalDevice::vkEnginePhysicalDevice(std::shared_ptr<vkEngineContext> context)
    : _context(std::move(context))
{
    if (_context->getVkInstance() == VK_NULL_HANDLE) {
        throw std::runtime_error("Vulkan instantiation not created!");
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_context->getVkInstance(), &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_context->getVkInstance(), &deviceCount, devices.data());

    const auto& req = getPhysicalDeviceReq();

    for (const auto& device : devices) {
        if (isDeviceSuitable(device, req)) {
            _physicalDevice = device;
            break;
        }
    }

    if (_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    _queueFamilyIndices = findQueueFamilies(_physicalDevice, req);
}

vkEnginePhysicalDevice::~vkEnginePhysicalDevice()
{
    _physicalDevice = VK_NULL_HANDLE;
}

std::shared_ptr<vkEngineContext> vkEnginePhysicalDevice::getContext() const
{
    return _context;
}

VkPhysicalDevice vkEnginePhysicalDevice::getVkPhysicalDevice() const
{
    return _physicalDevice;
}

const QueueFamilyIndices& vkEnginePhysicalDevice::getQueueFamilies() const
{
    return _queueFamilyIndices;
}

const vkEnginePhysicalDeviceReq& vkEnginePhysicalDevice::getPhysicalDeviceReq() const
{
    return _context->getConfig().physicalDeviceReq;
}
