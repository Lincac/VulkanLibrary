#include "ShaderModule.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
std::string Quote(const std::string& path)
{
    return "\"" + path + "\"";
}

std::string ReadEnvOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    if (value == nullptr)
    {
        return {};
    }
    return std::string(value);
}
} // namespace

ShaderModule::ShaderModule(VkDevice device, const std::vector<uint32_t>& spirvCode)
    : device_(device)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot create shader module with a null device.");
    }
    Create(spirvCode);
}

ShaderModule::ShaderModule(VkDevice device, const std::string& spirvFilePath)
    : device_(device)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot create shader module with a null device.");
    }
    Create(LoadSpirvFile(spirvFilePath));
}

ShaderModule::~ShaderModule()
{
    Destroy();
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    : device_(other.device_),
      module_(other.module_)
{
    other.device_ = VK_NULL_HANDLE;
    other.module_ = VK_NULL_HANDLE;
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    device_ = other.device_;
    module_ = other.module_;

    other.device_ = VK_NULL_HANDLE;
    other.module_ = VK_NULL_HANDLE;
    return *this;
}

void ShaderModule::RecreateFromSpirv(const std::vector<uint32_t>& spirvCode)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot recreate shader module with a null device.");
    }

    Destroy();
    Create(spirvCode);
}

void ShaderModule::RecreateFromSpirvFile(const std::string& spirvFilePath)
{
    RecreateFromSpirv(LoadSpirvFile(spirvFilePath));
}

void ShaderModule::RecreateFromSourceFile(
    Stage stage,
    const std::string& sourceFilePath,
    const std::string& entryPoint)
{
    RecreateFromSpirv(CompileSourceToSpirv(stage, sourceFilePath, entryPoint));
}

void ShaderModule::Destroy() noexcept
{
    if (device_ != VK_NULL_HANDLE && module_ != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device_, module_, nullptr);
    }
    module_ = VK_NULL_HANDLE;
}

VkShaderModule ShaderModule::Get() const noexcept
{
    return module_;
}

bool ShaderModule::IsValid() const noexcept
{
    return module_ != VK_NULL_HANDLE;
}

VkPipelineShaderStageCreateInfo ShaderModule::GetStageCreateInfo(
    VkShaderStageFlagBits stage,
    const char* entryPoint) const
{
    if (module_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot build shader stage info from an invalid shader module.");
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = stage;
    stageInfo.module = module_;
    stageInfo.pName = entryPoint;
    return stageInfo;
}

std::vector<uint32_t> ShaderModule::LoadSpirvFile(const std::string& spirvFilePath)
{
    std::ifstream file(spirvFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open SPIR-V file: " + spirvFilePath);
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0 || (fileSize % static_cast<std::streamsize>(sizeof(uint32_t))) != 0)
    {
        throw std::runtime_error("SPIR-V file size is invalid: " + spirvFilePath);
    }

    std::vector<uint32_t> code(static_cast<size_t>(fileSize) / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(code.data()), fileSize);

    if (!file.good())
    {
        throw std::runtime_error("Failed to read SPIR-V file: " + spirvFilePath);
    }

    return code;
}

std::string ShaderModule::CompileSourceToSpirvFile(
    Stage stage,
    const std::string& sourceFilePath,
    const std::string& outputSpirvPath,
    const std::string& entryPoint)
{
    std::filesystem::path outputPath(outputSpirvPath);
    if (outputPath.has_parent_path())
    {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::string glslc = "glslc";
    const std::string vulkanSdk = ReadEnvOrEmpty("VULKAN_SDK");
    if (!vulkanSdk.empty())
    {
        const std::filesystem::path glslcFromSdk = std::filesystem::path(vulkanSdk) / "Bin" / "glslc.exe";
        if (std::filesystem::exists(glslcFromSdk))
        {
            glslc = glslcFromSdk.string();
        }
    }

    std::ostringstream command;
    command << Quote(glslc)
            << " -fshader-stage=" << ToGlslcStageArg(stage)
            << " -fentry-point=" << entryPoint
            << " -o " << Quote(outputSpirvPath)
            << " " << Quote(sourceFilePath);

    const int exitCode = std::system(command.str().c_str());
    if (exitCode != 0)
    {
        throw std::runtime_error("Failed to compile shader source to SPIR-V: " + sourceFilePath);
    }

    return outputSpirvPath;
}

std::vector<uint32_t> ShaderModule::CompileSourceToSpirv(
    Stage stage,
    const std::string& sourceFilePath,
    const std::string& entryPoint)
{
    const std::filesystem::path sourcePath(sourceFilePath);
    const std::filesystem::path outputPath =
        sourcePath.parent_path() / (sourcePath.filename().string() + ".spv");

    CompileSourceToSpirvFile(stage, sourceFilePath, outputPath.string(), entryPoint);
    return LoadSpirvFile(outputPath.string());
}

void ShaderModule::Create(const std::vector<uint32_t>& spirvCode)
{
    if (spirvCode.empty())
    {
        throw std::runtime_error("Cannot create shader module from empty SPIR-V code.");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    if (vkCreateShaderModule(device_, &createInfo, nullptr, &module_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan shader module.");
    }
}

const char* ShaderModule::ToGlslcStageArg(Stage stage)
{
    switch (stage)
    {
    case Stage::Vertex:
        return "vert";
    case Stage::Fragment:
        return "frag";
    case Stage::Compute:
        return "comp";
    case Stage::Geometry:
        return "geom";
    case Stage::TessControl:
        return "tesc";
    case Stage::TessEvaluation:
        return "tese";
    default:
        throw std::runtime_error("Unsupported shader stage.");
    }
}
