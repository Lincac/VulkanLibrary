#include "matVkEngineCommon.h"

#include <fstream>

namespace mat {

    uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        VK_PRINT("VK_NOT_READY");
    }

    std::vector<char> load(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            VK_PRINT("VK_NOT_READY");
        }

        size_t fileSize = (size_t)file.tellg();
        auto buffer = std::vector<char>(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

}  // namespace mat
