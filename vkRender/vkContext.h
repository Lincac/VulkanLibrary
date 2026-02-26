#include <volk.h>
#include <glfw3.h>
#include <vector>

class vkContext {
public:
    void initialize(GLFWwindow* window);
    void cleanup();

    VkDevice getDevice() const { return device; }
    VkInstance getInstance() const { return instance; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    uint32_t getGraphicsQueueFamily() const { return graphicsQueueFamily; }
    VkExtent2D getSwapchainExtent() const { return swapchainExtent; }
    VkFormat getSwapchainFormat() const { return swapchainFormat; }
    VkSwapchainKHR getSwapchain() const { return swapchain; }
    VkSurfaceKHR getSurface() const { return surface; }

private:
    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSurface(GLFWwindow* window);
    void createSwapchain();

    uint32_t findGraphicsQueueFamily(VkPhysicalDevice gpu);
    bool checkValidationLayerSupport();

private:
    // Vulkan 实例：全局入口，几乎所有 Vulkan 资源都依赖它创建/销毁。
    VkInstance instance = VK_NULL_HANDLE;
    // 选中的物理设备（GPU）。
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // 与 physicalDevice 对应的逻辑设备，用于创建和提交 GPU 相关资源/命令。
    VkDevice device = VK_NULL_HANDLE;
    // 窗口表面（GLFW 创建），用于交换链呈现。
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    // 交换链：管理可呈现图像集合。
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    // Debug Messenger：接收 validation layer 输出的告警/错误信息。
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // 图形队列句柄（当前用于图形工作，也用于后续提交绘制命令）。
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    // 图形队列族索引，用于创建设备队列和资源共享策略判断。
    uint32_t graphicsQueueFamily = 0;

    // 交换链图像格式（颜色格式）。
    VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    // 交换链分辨率（通常与窗口尺寸或 surface 能力协商结果一致）。
    VkExtent2D swapchainExtent{};

    // 是否启用验证层：Debug 默认开启，Release 默认关闭。
    const bool enableValidationLayers = 
#ifdef NDEBUG 
        false; 
#else 
        true; 
#endif

    // 需要启用的验证层列表。
    const std::vector<const char*> validationLayers = { 
        "VK_LAYER_KHRONOS_validation" 
    };
};
