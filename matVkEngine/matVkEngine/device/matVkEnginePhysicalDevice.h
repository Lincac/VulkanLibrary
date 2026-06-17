#pragma once

#include "core/matVkEngineContext.h"

#include <memory>
#include <optional>

namespace mat {

	class VkEnginePhysicalDevice
	{
	public:
		explicit VkEnginePhysicalDevice(
			std::shared_ptr<VkEngineContext> context,
			const VkEnginePhysicalDeviceConfig& config = {});
		~VkEnginePhysicalDevice();

		VkPhysicalDevice getVkPhysicalDevice() const;

		std::optional<uint32_t> getGraphicsFamily() const;

		VkEnginePhysicalDeviceConfig getConfig() const;

	private:
		VkEnginePhysicalDevice(const VkEnginePhysicalDevice&) = delete;
		VkEnginePhysicalDevice(VkEnginePhysicalDevice&&) = delete;
		VkEnginePhysicalDevice& operator=(const VkEnginePhysicalDevice&) = delete;
		VkEnginePhysicalDevice& operator=(VkEnginePhysicalDevice&&) = delete;

		VkEnginePhysicalDeviceConfig _config;
		std::optional<uint32_t> _graphicsFamily;
		VkPhysicalDevice _device = VK_NULL_HANDLE;
	};
};

