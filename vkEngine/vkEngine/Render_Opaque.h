#pragma once

#include "Actor.h"

class Render_Opaque
{
public:

	Render_Opaque(vkEngine* engine, VkDescriptorSetLayout sceneDescSetLayout, bool enableMSAA = true);
	~Render_Opaque();

	void addActor(Actor* actor);

	void init();

	void cleanup();

	VkImageView getMainColorImageView(uint32_t idx);

	void draw(VkCommandBuffer commandBuffer, VkDescriptorSet sceneDescSet, uint32_t imageIndex);

private:

	void initRenderPass();

	void initMaterialDescriptorSetLayout();

	void initPipeline();

	void initAttachment();

	void initFrameBuffers();

private:

	vkEngine* _engine;
	VkDescriptorSetLayout _sceneDescSetLayout;

private:

	std::vector<Actor*> _actors;

    bool _enableMSAA = true;
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkImage> _msaaColorImages;
    std::vector<VkDeviceMemory> _msaaColorImageMemories;
    std::vector<VkImageView> _msaaColorImageViews;

	std::vector<VkImage> _colorImages;
	std::vector<VkDeviceMemory> _colorImageMemories;
	std::vector<VkImageView> _colorImageViews;

	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;

	VkRenderPass _renderPass = VK_NULL_HANDLE;
	VkDescriptorSetLayout _materialDescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
	VkPipeline _pipeline = VK_NULL_HANDLE;

	std::vector<VkFramebuffer> _framebuffers;

};

