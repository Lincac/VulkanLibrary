project(hello_window)

include_directories(${CMAKE_SOURCE_DIR}/tools)
include_directories(${CMAKE_SOURCE_DIR}/example/common)

include_directories($ENV{Vulkan_Include}include)
link_directories($ENV{Vulkan_Include}lib)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE vulkan-1.lib)