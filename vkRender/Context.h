#include <volk.h>
#include <glfw3.h>
#include <vector>

class Context {
public:
    // 初始化 Vulkan 上下文（实例、设备、Surface、Swapchain 等）。
    // 需要在窗口创建完成后调用，window 用于创建 Vulkan Surface。
    void initialize(GLFWwindow* window);
    // 按依赖顺序释放 Vulkan 资源，通常在程序退出前调用一次。
    void cleanup();
    // 仅重建交换链（窗口尺寸变化后调用）。
    void recreateSwapchain();

    // 获取逻辑设备句柄，用于创建/管理 Vulkan 资源。
    VkDevice getDevice() const { return device; }
    // 获取 Vulkan 实例句柄（全局入口对象）。
    VkInstance getInstance() const { return instance; }
    // 获取已选择的物理设备（GPU）。
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    // 获取图形队列句柄（用于提交绘制命令）。
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    // 获取图形队列族索引。
    uint32_t getGraphicsQueueFamily() const { return graphicsQueueFamily; }
    // 获取交换链图像分辨率。
    VkExtent2D getSwapchainExtent() const { return swapchainExtent; }
    // 获取交换链图像格式。
    VkFormat getSwapchainFormat() const { return swapchainFormat; }
    // 获取交换链对象句柄。
    VkSwapchainKHR getSwapchain() const { return swapchain; }
    // 获取窗口 Surface 句柄。
    VkSurfaceKHR getSurface() const { return surface; }

private:
    // 创建 Vulkan 实例，并启用所需扩展/验证层。
    void createInstance();
    // 创建调试消息回调（仅在启用验证层时有效）。
    void setupDebugMessenger();
    // 选择满足渲染需求的物理设备（GPU）。
    void pickPhysicalDevice();
    // 基于已选物理设备创建逻辑设备与队列。
    void createLogicalDevice();
    // 从 GLFW 窗口创建 Vulkan Surface。
    void createSurface(GLFWwindow* window);
    // 根据 Surface 能力创建交换链。
    void createSwapchain();
    // 销毁当前交换链。
    void destroySwapchain();

    // 查询并返回支持图形能力的队列族索引。
    uint32_t findGraphicsQueueFamily(VkPhysicalDevice gpu);
    // 检查请求的验证层在当前系统中是否可用。
    bool checkValidationLayerSupport();

private:
    GLFWwindow* window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;

    VkFormat swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent{};

    const bool enableValidationLayers = 
#ifdef NDEBUG 
        false; 
#else 
        true; 
#endif

    const std::vector<const char*> validationLayers = { 
        "VK_LAYER_KHRONOS_validation" 
    };
};
