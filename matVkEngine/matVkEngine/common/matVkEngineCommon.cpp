#include "matVkEngineCommon.h"

#include <stdexcept>

namespace mat {

    enum class ShaderStageSlot {
        None,
        Source,
        Destination,
    };

    struct ImageLayoutTransitionRule {
        VkImageLayout oldLayout;
        VkImageLayout newLayout;
        VkPipelineStageFlags srcStage;
        VkPipelineStageFlags dstStage;
        VkAccessFlags srcAccess;
        VkAccessFlags dstAccess;
        ShaderStageSlot srcShaderStage;
        ShaderStageSlot dstShaderStage;
    };

    VkPipelineStageFlags shaderStageForDomain(ImageShaderDomain domain) {
        switch (domain) {
            case ImageShaderDomain::Compute:
                return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            case ImageShaderDomain::Fragment:
                return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            case ImageShaderDomain::ColorAttachment:
                return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            case ImageShaderDomain::RayTracing:
                return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        }

        throw std::runtime_error("unknown ImageShaderDomain");
    }

    const ImageLayoutTransitionRule* findImageLayoutTransitionRule(VkImageLayout oldLayout, VkImageLayout newLayout) {
        static const ImageLayoutTransitionRule rules[] = {
            // compute / storage / ray-tracing writable image
            {
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_GENERAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                0,
                VK_ACCESS_SHADER_WRITE_BIT,
                ShaderStageSlot::None,
                ShaderStageSlot::Destination,
            },
            // shader output -> copy source
            {
                VK_IMAGE_LAYOUT_GENERAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                ShaderStageSlot::Source,
                ShaderStageSlot::None,
            },
            // upload destination
            {
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                ShaderStageSlot::None,
                ShaderStageSlot::None,
            },
            // uploaded data -> shader sampling
            {
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                ShaderStageSlot::None,
                ShaderStageSlot::Destination,
            },
            // raster render target setup
            {
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                ShaderStageSlot::None,
                ShaderStageSlot::None,
            },
            // render target -> present
            {
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                0,
                ShaderStageSlot::None,
                ShaderStageSlot::None,
            },
        };

        for (const ImageLayoutTransitionRule& rule : rules) {
            if (rule.oldLayout == oldLayout && rule.newLayout == newLayout) {
                return &rule;
            }
        }

        return nullptr;
    }

    VkPipelineStageFlags resolveStage(VkPipelineStageFlags stage, ShaderStageSlot slot,
                                      ImageShaderDomain shaderDomain) {
        if (slot == ShaderStageSlot::Source || slot == ShaderStageSlot::Destination) {
            return shaderStageForDomain(shaderDomain);
        }

        return stage;
    }

    uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
            if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                               ImageShaderDomain shaderDomain) {
        const ImageLayoutTransitionRule* rule = findImageLayoutTransitionRule(oldLayout, newLayout);
        if (rule == nullptr) {
            throw std::runtime_error("unsupported image layout transition");
        }

        const VkPipelineStageFlags srcStage = resolveStage(rule->srcStage, rule->srcShaderStage, shaderDomain);
        const VkPipelineStageFlags dstStage = resolveStage(rule->dstStage, rule->dstShaderStage, shaderDomain);

        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask = rule->srcAccess;
        barrier.dstAccessMask = rule->dstAccess;

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

}  // namespace mat
