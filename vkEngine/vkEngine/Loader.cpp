#include "Loader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    std::string toForwardSlash(std::string path)
    {
        for (char& ch : path) {
            if (ch == '\\') {
                ch = '/';
            }
        }
        return path;
    }

    std::string resolveTexturePath(const std::filesystem::path& baseDir, const aiString& rawPath)
    {
        if (rawPath.length == 0) {
            return {};
        }

        std::filesystem::path texturePath(rawPath.C_Str());
        if (texturePath.is_relative()) {
            texturePath = baseDir / texturePath;
        }

        return toForwardSlash(texturePath.lexically_normal().string());
    }

    std::string getFirstTextureOfTypes(
        const aiMaterial* material,
        const std::filesystem::path& baseDir,
        const std::vector<aiTextureType>& candidates)
    {
        for (aiTextureType type : candidates) {
            if (material->GetTextureCount(type) == 0) {
                continue;
            }

            aiString path{};
            if (material->GetTexture(type, 0, &path) == AI_SUCCESS) {
                std::string resolved = resolveTexturePath(baseDir, path);
                if (!resolved.empty()) {
                    return resolved;
                }
            }
        }

        return {};
    }

    MaterialTextureSet extractMaterialTextures(const aiMaterial* material, const std::filesystem::path& baseDir)
    {
        MaterialTextureSet textures{};

        // Matches OpaqueShader.frag bindings:
        // 1:BaseColor 2:MetalRough 3:Normal 4:Occlusion 5:Emissive
        textures.baseColorMap = getFirstTextureOfTypes(material, baseDir, {
            aiTextureType_BASE_COLOR,
            aiTextureType_DIFFUSE
            });

        textures.metalRoughMap = getFirstTextureOfTypes(material, baseDir, {
            aiTextureType_UNKNOWN,          // glTF metallic-roughness map is often placed here by importers
            aiTextureType_METALNESS,
            aiTextureType_DIFFUSE_ROUGHNESS
            });

        textures.normalMap = getFirstTextureOfTypes(material, baseDir, {
            aiTextureType_NORMALS,
            aiTextureType_HEIGHT
            });

        textures.occlusionMap = getFirstTextureOfTypes(material, baseDir, {
            aiTextureType_AMBIENT_OCCLUSION,
            aiTextureType_LIGHTMAP
            });

        textures.emissiveMap = getFirstTextureOfTypes(material, baseDir, {
            aiTextureType_EMISSIVE
            });

        return textures;
    }

    struct LoadContext
    {
        const aiScene* scene = nullptr;
        const std::filesystem::path* baseDir = nullptr;
        std::vector<std::optional<MaterialTextureSet>> materialTextureCache{};
        bool hasVertex = false;
        glm::vec3 minPos = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 maxPos = glm::vec3(std::numeric_limits<float>::lowest());
    };

    LoadedMesh processMesh(const aiMesh* mesh, LoadContext& context)
    {
        LoadedMesh loadedMesh{};
        loadedMesh.name = mesh->mName.C_Str();

        loadedMesh.vertices.reserve(mesh->mNumVertices);
        for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
            LoadedVertex vertex{};

            const aiVector3D& position = mesh->mVertices[i];
            vertex.position = glm::vec3(position.x, position.y, position.z);
            context.minPos = glm::min(context.minPos, vertex.position);
            context.maxPos = glm::max(context.maxPos, vertex.position);
            context.hasVertex = true;

            if (mesh->HasNormals()) {
                const aiVector3D& normal = mesh->mNormals[i];
                vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
            }

            if (mesh->HasTangentsAndBitangents()) {
                const aiVector3D& tangent = mesh->mTangents[i];
                const aiVector3D& bitangent = mesh->mBitangents[i];
                vertex.tangent = glm::vec3(tangent.x, tangent.y, tangent.z);
                vertex.bitangent = glm::vec3(bitangent.x, bitangent.y, bitangent.z);
            }

            if (mesh->HasTextureCoords(0)) {
                const aiVector3D& uv = mesh->mTextureCoords[0][i];
                vertex.uv = glm::vec2(uv.x, uv.y);
            }

            loadedMesh.vertices.push_back(std::move(vertex));
        }

        loadedMesh.indices.reserve(mesh->mNumFaces * 3);
        for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            for (uint32_t index = 0; index < face.mNumIndices; ++index) {
                loadedMesh.indices.push_back(face.mIndices[index]);
            }
        }

        if (mesh->mMaterialIndex < context.scene->mNumMaterials) {
            auto& cacheEntry = context.materialTextureCache[mesh->mMaterialIndex];
            if (!cacheEntry.has_value()) {
                cacheEntry = extractMaterialTextures(
                    context.scene->mMaterials[mesh->mMaterialIndex],
                    *context.baseDir);
            }
            loadedMesh.textures = *cacheEntry;
        }

        return loadedMesh;
    }

    void processNode(
        const aiNode* node,
        LoadContext& context,
        LoadedModel& outputModel)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
            const aiMesh* mesh = context.scene->mMeshes[node->mMeshes[i]];
            outputModel.meshes.push_back(processMesh(mesh, context));
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            processNode(node->mChildren[i], context, outputModel);
        }
    }

    void centerModelAtOrigin(LoadedModel& model, const LoadContext& context)
    {
        if (!context.hasVertex) {
            return;
        }

        const glm::vec3 center = (context.minPos + context.maxPos) * 0.5f;
        for (auto& mesh : model.meshes) {
            for (auto& vertex : mesh.vertices) {
                vertex.position -= center;
            }
        }
    }
}

LoadedModel Loader::loadModel(const std::string& modelPath)
{
    const std::filesystem::path path(modelPath);
    const std::filesystem::path baseDir = path.parent_path();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        modelPath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |   // auto-generate normals when source has none
        aiProcess_CalcTangentSpace |   // auto-generate tangent/bitangent based on UV + normal
        aiProcess_ImproveCacheLocality |
        aiProcess_SortByPType);

    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr) {
        throw std::runtime_error("assimp failed to load model: " + modelPath + " | " + importer.GetErrorString());
    }

    LoadedModel model{};
    model.sourcePath = toForwardSlash(path.lexically_normal().string());
    model.baseDirectory = toForwardSlash(baseDir.lexically_normal().string());
    model.meshes.reserve(scene->mNumMeshes);

    LoadContext context{};
    context.scene = scene;
    context.baseDir = &baseDir;
    context.materialTextureCache.resize(scene->mNumMaterials);

    processNode(scene->mRootNode, context, model);
    centerModelAtOrigin(model, context);
    return model;
}
