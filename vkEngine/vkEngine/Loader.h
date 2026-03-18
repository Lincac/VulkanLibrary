#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct LoadedVertex
{
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
    glm::vec3 tangent{ 1.0f, 0.0f, 0.0f };
    glm::vec3 bitangent{ 0.0f, 0.0f, 1.0f };
    glm::vec2 uv{ 0.0f, 0.0f };
};

struct MaterialTextureSet
{
    std::string baseColorMap;   // set=1,binding=1
    std::string metalRoughMap;  // set=1,binding=2
    std::string normalMap;      // set=1,binding=3
    std::string occlusionMap;   // set=1,binding=4
    std::string emissiveMap;    // set=1,binding=5
};

struct LoadedMesh
{
    std::string name;
    std::vector<LoadedVertex> vertices;
    std::vector<uint32_t> indices;
    MaterialTextureSet textures;
};

struct LoadedModel
{
    std::string sourcePath;
    std::string baseDirectory;
    std::vector<LoadedMesh> meshes;
};

class Loader
{
public:
    // Loaded vertices are translated so the model AABB center is at world origin.
    static LoadedModel loadModel(const std::string& modelPath);
};
