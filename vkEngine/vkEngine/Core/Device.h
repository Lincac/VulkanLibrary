#pragma once

#include "PhysicalDevice.h"

class Device
{
public:

    Device(const PhysicalDevice& physicalDevice);

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept = default;
    Device& operator=(Device&& other) noexcept = default;
    ~Device() = default;

public:

    VkDevice getDevice() const;

    VkQueue getGraphicsQueue() const;

    VkQueue getPresentQueue() const;

private:

    friend class Swapchain;

    VkDevice _device;

    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

};

