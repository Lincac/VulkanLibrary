set(VKENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vkEngine")

file(GLOB VKENGINE_SOURCES CONFIGURE_DEPENDS
    "${VKENGINE_SOURCE_DIR}/*.cpp"
    "${VKENGINE_SOURCE_DIR}/*.h"
    "${VKENGINE_SOURCE_DIR}/RT/*.cpp"
    "${VKENGINE_SOURCE_DIR}/RT/*.h"
)

add_library(vkEngine STATIC ${VKENGINE_SOURCES})

target_include_directories(vkEngine
    PUBLIC
        $<BUILD_INTERFACE:${VKENGINE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${VKENGINE_SOURCE_DIR}/RT>
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
