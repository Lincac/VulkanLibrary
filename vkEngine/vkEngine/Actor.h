#pragma once

#include "vkEngine.h"
#include "Loader.h"

struct Materials
{
    glm::vec4 baseColor = glm::vec4(1);
    float roughness = 0.1;
    float subSurface = 0;
    float sheen = 0;
    float sheenTint = 0;

    float metallic = 0;
    float specular = 0;
    float specularTint = 0;

    float clearcoat = 0;
    float clearcoatGloss = 0;
};

class Actor
{
public:

    Actor();
    ~Actor();

    void init(vkEngine* engine);

    void setInputData(const LoadedModel& data);

    void setMaterial(const Materials& materials);

    void setTransformMatrix(const glm::mat4& matrix);

    const glm::mat4 getTransformMatrix();

    VkDescriptorSetLayout getMaterialDescriptorSetLayout() const;

    VkDescriptorSet getMaterialDescriptorSet();

    void draw(VkCommandBuffer command, VkPipelineLayout pipelineLayout, VkDescriptorSet cameraDescriptorSet);

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
    std::vector<uint32_t> _indices;

    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;

    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

};
