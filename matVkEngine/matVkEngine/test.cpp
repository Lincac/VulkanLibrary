#include "pipeline/matVkEngineRenderPipeline.h"
#include "common/matVkEngineCommon.h"

VkPipeline pipelines[2];
VkCommandBuffer cmd;

void func() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = offscreenRenderPass;    
    rpInfo.framebuffer = offscreenFramebuffer;  
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent.width = 800;
    rpInfo.renderArea.extent.height = 600;

    VkClearValue clearValues[3];
    clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};  // GBuffer color
    clearValues[1].color = {{0.f, 0.f, 0.f, 1.f}};  // GBuffer normal
    clearValues[2].depthStencil = {1.f, 0};         // depth

    rpInfo.clearValueCount = 3;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    for (size_t i = 0; i < 2; i++) {
        if (i != 0) {
            vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[i]);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = 800;
        viewport.height = 600;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent.width = 800;
        scissor.extent.height = 600;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        VkBuffer vbs[] = {vertexBuffer};
        vkCmdBindVertexBuffers(cmd, 0, 1, vbs, {0});
        vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout, 0, 1,
                                &gbufferDescriptorSet, 0, nullptr);

        vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));
}