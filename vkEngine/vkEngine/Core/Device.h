#pragma once

#include "PhysicalDevice.h"

class Device
{
public:

    Device();

public:

    void setDependice(PhysicalDevice* physicalDevice);

    int create();

    VkDevice getDevice() const;

    PhysicalDevice* getPhysicalDevice() const;

    VkQueue getGraphicsQueue() const;

    VkQueue getPresentQueue() const;

private:

    PhysicalDevice* _physicalDevice;

private:

    VkDevice _device;

    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

};

