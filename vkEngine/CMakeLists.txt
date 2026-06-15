set(VKENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(VKENGINE_SOURCES
    ${VKENGINE_ROOT}/core/vkEngineContext.cpp
    ${VKENGINE_ROOT}/device/vkEnginePhysicalDevice.cpp
    ${VKENGINE_ROOT}/device/vkEngineLogicalDevice.cpp
    ${VKENGINE_ROOT}/device/vkEngineCommandPool.cpp
    ${VKENGINE_ROOT}/resource/vkEngineBuffer.cpp
    ${VKENGINE_ROOT}/resource/vkEngineImage.cpp
    ${VKENGINE_ROOT}/resource/vkEngineTexture.cpp
    ${VKENGINE_ROOT}/rt/vkEngineAccelerationStructure.cpp
    ${VKENGINE_ROOT}/rt/vkEngineRTDescriptor.cpp
    ${VKENGINE_ROOT}/rt/vkEngineRayTracingPipeline.cpp
)

add_library(vkEngine STATIC ${VKENGINE_SOURCES})

target_include_directories(vkEngine
    PUBLIC
        $<BUILD_INTERFACE:${VKENGINE_ROOT}>
)

# volk: compile from Vulkan SDK (Include/Volk/volk.c + volk.h), not Lib/volk*.lib
find_path(VOLK_SOURCE_DIR
    NAMES volk.c
    PATHS "${Vulkan_INCLUDE_DIRS}/Volk" "${Vulkan_INCLUDE_DIRS}/volk"
    NO_DEFAULT_PATH
)
if(NOT VOLK_SOURCE_DIR)
    message(FATAL_ERROR "volk.c not found under ${Vulkan_INCLUDE_DIRS}")
endif()

add_library(volk STATIC "${VOLK_SOURCE_DIR}/volk.c")
target_include_directories(volk PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_compile_definitions(vkEngine
    PUBLIC
        VK_NO_PROTOTYPES
)

target_link_libraries(vkEngine
    PUBLIC
        glm
        stb
        glfw
        volk
        Vulkan::Vulkan
)
