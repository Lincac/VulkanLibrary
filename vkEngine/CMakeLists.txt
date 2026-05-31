set(VKENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vkEngine")

file(GLOB VKENGINE_SOURCES CONFIGURE_DEPENDS
    "${VKENGINE_SOURCE_DIR}/*.cpp"
    "${VKENGINE_SOURCE_DIR}/*.h"
)

add_library(vkEngine STATIC ${VKENGINE_SOURCES})

target_include_directories(vkEngine
    PUBLIC
        $<BUILD_INTERFACE:${VKENGINE_SOURCE_DIR}>
)

get_filename_component(VULKAN_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)

add_library(volk STATIC IMPORTED GLOBAL)
set_target_properties(volk PROPERTIES
    IMPORTED_LOCATION_DEBUG "${VULKAN_LIB_DIR}/volkd.lib"
    IMPORTED_LOCATION_RELEASE "${VULKAN_LIB_DIR}/volk.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${VULKAN_LIB_DIR}/volk.lib"
    IMPORTED_LOCATION_MINSIZEREL "${VULKAN_LIB_DIR}/volk.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}"
)

target_compile_definitions(vkEngine
    PUBLIC
        VK_NO_PROTOTYPES
)

target_link_libraries(vkEngine
    PUBLIC
        glm
        glfw
        volk
        Vulkan::Vulkan
)
