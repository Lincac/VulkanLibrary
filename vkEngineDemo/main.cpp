#include "vkEngineCommandPool.h"

#include <iostream>

int main(){
    vkEngine engine("Vulkan Demo", true);

    vkEnginePhysicalDevice physicalDevice(engine);
    vkEngineLogicalDevice logicalDevice(physicalDevice);
    vkEngineCommandPool commandPool(logicalDevice);

    std::cout << "vulkan is init" << std::endl;

    return 0;
}