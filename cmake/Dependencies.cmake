include_guard(GLOBAL)

# Add custom CMake module path (if later you provide FindXXX.cmake files).
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# --- Vulkan SDK ---
find_package(Vulkan REQUIRED)
