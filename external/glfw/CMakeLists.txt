cmake_minimum_required(VERSION 3.20)

project(glfw LANGUAGES C)

add_library(glfw STATIC
    context.c
    init.c
    input.c
    monitor.c
    platform.c
    vulkan.c
    window.c
    win32_init.c
    win32_joystick.c
    win32_monitor.c
    win32_module.c
    win32_time.c
    win32_thread.c
    win32_window.c
    wgl_context.c
)

target_compile_definitions(glfw PRIVATE _GLFW_WIN32)

target_include_directories(glfw
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
    target_link_libraries(glfw
        PRIVATE
            gdi32
            user32
            shell32
            opengl32
            winmm
    )
endif()
