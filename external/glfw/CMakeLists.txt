cmake_minimum_required(VERSION 3.20)

project(glfw LANGUAGES C)

file(GLOB_RECURSE SRC_FILES *.c)

file(GLOB_RECURSE HEADER_FILES *.h)

add_library(glfw STATIC
    ${SRC_FILES}
    ${HEADER_FILES}
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
