add_executable(vkEngineDemo
    main.cpp
)

target_link_libraries(vkEngineDemo
    PRIVATE
        vkEngine
)

set_target_properties(vkEngineDemo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vkEngineDemo>"
)

add_custom_command(TARGET vkEngineDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vkEngineDemo>/shaders"
    COMMAND glslc --target-spv=spv1.4 -fshader-stage=rgen
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/raygen.rgen"
        -o "$<TARGET_FILE_DIR:vkEngineDemo>/shaders/raygen.spv"
    COMMAND glslc --target-spv=spv1.4 -fshader-stage=rmiss
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/miss.rmiss"
        -o "$<TARGET_FILE_DIR:vkEngineDemo>/shaders/miss.spv"
    COMMAND glslc --target-spv=spv1.4 -fshader-stage=rchit
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/closesthit.rchit"
        -o "$<TARGET_FILE_DIR:vkEngineDemo>/shaders/closesthit.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:vkEngineDemo>/shaders/raygen.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/output/raygen.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:vkEngineDemo>/shaders/miss.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/output/miss.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:vkEngineDemo>/shaders/closesthit.spv"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/output/closesthit.spv"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/models"
        "$<TARGET_FILE_DIR:vkEngineDemo>/models"
)
