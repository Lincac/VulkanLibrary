#include "vkEngineCommandPool.h"
#include "vkEngineImage.h"

#include <iostream>

int main(){
    vkEngine engine("Vulkan Demo", true);

    vkEnginePhysicalDevice physicalDevice(engine);
    vkEngineLogicalDevice logicalDevice(physicalDevice);
    vkEngineCommandPool commandPool(logicalDevice);

    vkEngineImage image(logicalDevice);
    image.setResolution(glm::ivec2(800,600));
    image.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
    image.setImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    image.generate();

    commandPool.submitOneTimeCommands([&](VkCommandBuffer cmd) {
        transitionImageLayout(cmd, image.getImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    });

    std::cout << "storage image ready" << std::endl;
    std::cout << "vulkan is init" << std::endl;

    return 0;
}