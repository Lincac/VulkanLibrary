#include "pipeline/matVkEngineRenderPipeline.h"

void func()
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    // -----------------------------
    // 1. 开始 RenderPass
    // -----------------------------
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = offscreenRenderPass;    // 多 subpass 的 render pass
    rpInfo.framebuffer = offscreenFramebuffer;  // 多 attachment 的 framebuffer
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = offscreenExtent;

    VkClearValue clearValues[3];
    clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};  // GBuffer color
    clearValues[1].color = {{0.f, 0.f, 0.f, 1.f}};  // GBuffer normal
    clearValues[2].depthStencil = {1.f, 0};         // depth

    rpInfo.clearValueCount = 3;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // ============================================================
    // SUBPASS 0: GBuffer Pass
    // ============================================================
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline);

    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float)offscreenExtent.width;
    viewport.height = (float)offscreenExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = offscreenExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkBuffer vbs[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vbs, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipelineLayout, 0, 1, &gbufferDescriptorSet, 0,
                            nullptr);

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);

    // -----------------------------
    // 进入 Subpass 1
    // -----------------------------
    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);

    // ============================================================
    // SUBPASS 1: Lighting Pass
    // ============================================================
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipelineLayout, 0, 1, &lightingDescriptorSet,
                            0, nullptr);

    // Fullscreen quad
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // -----------------------------
    // 结束 RenderPass
    // -----------------------------
    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));
}