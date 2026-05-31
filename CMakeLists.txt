cmake_minimum_required(VERSION 3.16)

project(VulkanLibrary
    VERSION 1.0.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(DEPENDENCE_DIR "${CMAKE_SOURCE_DIR}/Dependence" CACHE PATH "Path to third-party dependencies")

if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK "$ENV{VULKAN_SDK}" CACHE PATH "Vulkan SDK root directory")
endif()

find_package(Vulkan REQUIRED)

add_subdirectory(Dependence)
add_subdirectory(vkEngine)
add_subdirectory(vkEngineDemo)
