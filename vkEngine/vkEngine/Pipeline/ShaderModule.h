#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <volk/volk.h>

class ShaderModule
{
public:
    enum class Stage
    {
        Vertex,
        Fragment,
        Compute,
        Geometry,
        TessControl,
        TessEvaluation
    };

    ShaderModule() = default;
    ShaderModule(VkDevice device, const std::vector<uint32_t>& spirvCode);
    ShaderModule(VkDevice device, const std::string& spirvFilePath);
    ~ShaderModule();

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;
    ShaderModule(ShaderModule&& other) noexcept;
    ShaderModule& operator=(ShaderModule&& other) noexcept;

    void RecreateFromSpirv(const std::vector<uint32_t>& spirvCode);
    void RecreateFromSpirvFile(const std::string& spirvFilePath);
    void RecreateFromSourceFile(
        Stage stage,
        const std::string& sourceFilePath,
        const std::string& entryPoint = "main");
    void Destroy() noexcept;

    VkShaderModule Get() const noexcept;
    bool IsValid() const noexcept;
    VkPipelineShaderStageCreateInfo GetStageCreateInfo(
        VkShaderStageFlagBits stage,
        const char* entryPoint = "main") const;

    static std::vector<uint32_t> LoadSpirvFile(const std::string& spirvFilePath);
    static std::string CompileSourceToSpirvFile(
        Stage stage,
        const std::string& sourceFilePath,
        const std::string& outputSpirvPath,
        const std::string& entryPoint = "main");
    static std::vector<uint32_t> CompileSourceToSpirv(
        Stage stage,
        const std::string& sourceFilePath,
        const std::string& entryPoint = "main");

private:
    void Create(const std::vector<uint32_t>& spirvCode);
    static const char* ToGlslcStageArg(Stage stage);

private:
    VkDevice device_{VK_NULL_HANDLE};
    VkShaderModule module_{VK_NULL_HANDLE};
};
