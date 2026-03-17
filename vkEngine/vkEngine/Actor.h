#pragma once

#include "vkEngine.h"
#include "Loader.h"

struct Materials
{
    glm::vec4       baseColorFactor = glm::vec4(1);   // rgb: baseColor, a: alpha
    float           metallicFactor;
    float           roughnessFactor;
    float           normalScaleFactor;
    float           occlusionStrengthFactor;
    glm::vec3       emissiveFactor;
    float           alphaCutoffFactor;       // for mask
};

class Actor
{
public:

    Actor();
    ~Actor();

    void init(vkEngine* engine);

    void setInputData(const LoadedModel& data);

    void setTransformMatrix(const glm::mat4& matrix);

    VkDescriptorSet getMaterialDescriptorSet();

    void draw(VkCommandBuffer command);

public:

    void setBaseColor(const glm::vec4& color);

    void setMetallic(float factor);

    void setRoughness(float factor);

    void setNormalScale(float factor);

    void setOcclusionStrength(float factor);

    void setEmissive(const glm::vec3& factor);

    void setAlphaCutoff(float factor);

private:

    void initVertexData();
    void initMaterialDescriptorSet();
    void releaseResources();

private:

    vkEngine* _engine;

private:

    bool _updateMaterials;
    Materials _materials;
    VkBuffer _materialsBuffer;
    VkDeviceMemory _materialsBufferMemory;
    VkDescriptorPool _materialsDescriptorPool;
    VkDescriptorSetLayout _materialsDescriptorSetLayout;
    VkDescriptorSet _materialsDescriptorSet;

    glm::mat4 _matrix;

    std::vector<float> _vertices;
    std::vector<uint16_t> _indices;

    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;

    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

};
