project(hello_window)

include_directories(${CMAKE_SOURCE_DIR}/tools)
include_directories(${CMAKE_SOURCE_DIR}/example/common)

include_directories($ENV{Vulkan_Include}include)
link_directories($ENV{Vulkan_Include}lib)

add_executable(hello_window main.cpp)
target_link_libraries(hello_window PRIVATE vulkan-1.lib)