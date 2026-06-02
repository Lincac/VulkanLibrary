cd F:\repo\VulkanLibrary\vkEngineDemo\shaders

glslc --target-spv=spv1.4 -fshader-stage=rgen  raygen.rgen       -o raygen.spv
glslc --target-spv=spv1.4 -fshader-stage=rchit closesthit.rchit  -o closesthit.spv
glslc --target-spv=spv1.4 -fshader-stage=rmiss miss.rmiss        -o miss.spv