#include "matVkEngineMesh.h"

#include <glm/glm.hpp>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace mat {

    struct FaceCorner {
        int32_t posIndex = 0;
        int32_t normalIndex = 0;
    };

    struct FaceTri {
        FaceCorner i0;
        FaceCorner i1;
        FaceCorner i2;
    };

    static FaceCorner parseFaceCorner(const std::string& token) {
        FaceCorner corner{};

        const size_t firstSlash = token.find('/');
        if (firstSlash == std::string::npos) {
            corner.posIndex = static_cast<int32_t>(std::stoi(token));
            return corner;
        }

        corner.posIndex = static_cast<int32_t>(std::stoi(token.substr(0, firstSlash)));

        const size_t secondSlash = token.find('/', firstSlash + 1);
        if (secondSlash == std::string::npos) {
            return corner;
        }

        const std::string normalToken = token.substr(secondSlash + 1);
        if (!normalToken.empty()) {
            corner.normalIndex = static_cast<int32_t>(std::stoi(normalToken));
        }
        return corner;
    }

    static std::vector<Vertex> loadObjVertices(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open obj file: " + path);
        }

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<FaceTri> faceTris;

        auto resolveIndex = [&](int32_t index, size_t count) -> size_t {
            if (index > 0) {
                return static_cast<size_t>(index - 1);
            }
            if (index < 0) {
                return count - static_cast<size_t>(-index);
            }
            throw std::runtime_error("invalid vertex index 0 in obj: " + path);
        };

        auto parseFaceLine = [&](std::istringstream& iss) {
            std::vector<FaceCorner> faceCorners;
            std::string token;
            while (iss >> token) {
                faceCorners.push_back(parseFaceCorner(token));
            }

            if (faceCorners.size() < 3) {
                return;
            }

            for (size_t i = 1; i + 1 < faceCorners.size(); ++i) {
                faceTris.push_back({faceCorners[0], faceCorners[i], faceCorners[i + 1]});
            }
        };

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                glm::vec3 position{};
                iss >> position.x >> position.y >> position.z;
                positions.push_back(position);
            } else if (prefix == "vn") {
                glm::vec3 normal{};
                iss >> normal.x >> normal.y >> normal.z;
                const float len = glm::length(normal);
                normal = (len > 0.0f) ? normal / len : glm::vec3(0.0f, 0.0f, 1.0f);
                normals.push_back(normal);
            }
        }

        if (positions.empty()) {
            throw std::runtime_error("obj mesh has no vertices: " + path);
        }

        file.clear();
        file.seekg(0);

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "f") {
                parseFaceLine(iss);
            }
        }

        if (faceTris.empty()) {
            throw std::runtime_error("obj mesh has no triangles: " + path);
        }

        bool useFileNormals = !normals.empty();
        if (useFileNormals) {
            for (const FaceTri& tri : faceTris) {
                const FaceCorner corners[3] = {tri.i0, tri.i1, tri.i2};
                for (const FaceCorner& corner : corners) {
                    if (corner.normalIndex == 0) {
                        useFileNormals = false;
                        break;
                    }
                }
                if (!useFileNormals) {
                    break;
                }
            }
        }

        std::vector<Vertex> meshVertices;
        meshVertices.reserve(faceTris.size() * 3);

        if (useFileNormals) {
            for (const FaceTri& tri : faceTris) {
                const FaceCorner corners[3] = {tri.i0, tri.i1, tri.i2};
                for (const FaceCorner& corner : corners) {
                    const size_t posIdx = resolveIndex(corner.posIndex, positions.size());
                    const size_t normalIdx = resolveIndex(corner.normalIndex, normals.size());
                    if (posIdx >= positions.size() || normalIdx >= normals.size()) {
                        throw std::runtime_error("face index out of range in obj: " + path);
                    }

                    const glm::vec3& position = positions[posIdx];
                    const glm::vec3& normal = normals[normalIdx];
                    meshVertices.push_back({{position.x, position.y, position.z}, {normal.x, normal.y, normal.z}});
                }
            }
        } else {
            std::vector<glm::vec3> positionNormals(positions.size(), glm::vec3(0.0f));
            for (const FaceTri& tri : faceTris) {
                const size_t i0 = resolveIndex(tri.i0.posIndex, positions.size());
                const size_t i1 = resolveIndex(tri.i1.posIndex, positions.size());
                const size_t i2 = resolveIndex(tri.i2.posIndex, positions.size());
                if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size()) {
                    throw std::runtime_error("face index out of range in obj: " + path);
                }

                const glm::vec3& p0 = positions[i0];
                const glm::vec3& p1 = positions[i1];
                const glm::vec3& p2 = positions[i2];
                const glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

                positionNormals[i0] += faceNormal;
                positionNormals[i1] += faceNormal;
                positionNormals[i2] += faceNormal;
            }

            for (glm::vec3& normal : positionNormals) {
                const float len = glm::length(normal);
                normal = (len > 0.0f) ? normal / len : glm::vec3(0.0f, 0.0f, 1.0f);
            }

            for (const FaceTri& tri : faceTris) {
                const FaceCorner corners[3] = {tri.i0, tri.i1, tri.i2};
                for (const FaceCorner& corner : corners) {
                    const size_t posIdx = resolveIndex(corner.posIndex, positions.size());
                    if (posIdx >= positions.size()) {
                        throw std::runtime_error("face index out of range in obj: " + path);
                    }

                    const glm::vec3& position = positions[posIdx];
                    const glm::vec3& normal = positionNormals[posIdx];
                    meshVertices.push_back({{position.x, position.y, position.z}, {normal.x, normal.y, normal.z}});
                }
            }
        }

        if (meshVertices.empty() || meshVertices.size() % 3 != 0) {
            throw std::runtime_error("obj mesh has no triangles: " + path);
        }

        return meshVertices;
    }

    static std::string getLowercaseExtension(const std::string& path) {
        const size_t dot = path.find_last_of('.');
        if (dot == std::string::npos || dot + 1 >= path.size()) {
            return {};
        }

        std::string extension = path.substr(dot + 1);
        for (char& ch : extension) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        return extension;
    }

    static void saveObj(const std::string& path, const std::vector<Vertex>& meshVertices) {
        std::ofstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open obj file for writing: " + path);
        }

        file << std::fixed << std::setprecision(6);

        for (const Vertex& vertex : meshVertices) {
            file << "v " << vertex.pos[0] << ' ' << vertex.pos[1] << ' ' << vertex.pos[2] << '\n';
        }

        for (const Vertex& vertex : meshVertices) {
            file << "vn " << vertex.normal[0] << ' ' << vertex.normal[1] << ' ' << vertex.normal[2] << '\n';
        }

        for (size_t i = 0; i + 2 < meshVertices.size(); i += 3) {
            const size_t i0 = i + 1;
            const size_t i1 = i + 2;
            const size_t i2 = i + 3;
            file << "f " << i0 << '/' << i0 << ' ' << i1 << '/' << i1 << ' ' << i2 << '/' << i2 << '\n';
        }
    }

    static void saveStl(const std::string& path, const std::vector<Vertex>& meshVertices) {
        std::ofstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open stl file for writing: " + path);
        }

        file << std::fixed << std::setprecision(6);
        file << "solid matVkEngineMesh\n";

        for (size_t i = 0; i + 2 < meshVertices.size(); i += 3) {
            const glm::vec3 p0{meshVertices[i].pos[0], meshVertices[i].pos[1], meshVertices[i].pos[2]};
            const glm::vec3 p1{meshVertices[i + 1].pos[0], meshVertices[i + 1].pos[1], meshVertices[i + 1].pos[2]};
            const glm::vec3 p2{meshVertices[i + 2].pos[0], meshVertices[i + 2].pos[1], meshVertices[i + 2].pos[2]};

            glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
            const float len = glm::length(normal);
            normal = (len > 0.0f) ? normal / len : glm::vec3(0.0f, 0.0f, 1.0f);

            file << "  facet normal " << normal.x << ' ' << normal.y << ' ' << normal.z << '\n';
            file << "    outer loop\n";
            file << "      vertex " << p0.x << ' ' << p0.y << ' ' << p0.z << '\n';
            file << "      vertex " << p1.x << ' ' << p1.y << ' ' << p1.z << '\n';
            file << "      vertex " << p2.x << ' ' << p2.y << ' ' << p2.z << '\n';
            file << "    endloop\n";
            file << "  endfacet\n";
        }

        file << "endsolid matVkEngineMesh\n";
    }

    VkEngineMesh::VkEngineMesh() {}

    VkEngineMesh::~VkEngineMesh() {}

    void VkEngineMesh::load(const std::string& path) {
        _vertices = loadObjVertices(path);
    }

    void VkEngineMesh::append(const std::string& path) {
        const std::vector<Vertex> appended = loadObjVertices(path);
        _vertices.insert(_vertices.end(), appended.begin(), appended.end());
    }

    void VkEngineMesh::save(const std::string& path) {
        if (_vertices.empty() || _vertices.size() % 3 != 0) {
            throw std::runtime_error("mesh has no triangles to save");
        }

        const std::string extension = getLowercaseExtension(path);
        if (extension == "obj") {
            saveObj(path, _vertices);
            return;
        }
        if (extension == "stl") {
            saveStl(path, _vertices);
            return;
        }

        throw std::runtime_error("unsupported mesh file format: " + path);
    }

    void VkEngineMesh::unitization() {
        if (_vertices.empty()) {
            throw std::runtime_error("mesh has no vertices to unitize");
        }

        glm::vec3 minPos{_vertices[0].pos[0], _vertices[0].pos[1], _vertices[0].pos[2]};
        glm::vec3 maxPos = minPos;
        for (const Vertex& vertex : _vertices) {
            const glm::vec3 pos{vertex.pos[0], vertex.pos[1], vertex.pos[2]};
            minPos = glm::min(minPos, pos);
            maxPos = glm::max(maxPos, pos);
        }

        const glm::vec3 center = (minPos + maxPos) * 0.5f;
        const glm::vec3 extent = maxPos - minPos;
        const float maxExtent = std::max(std::max(extent.x, extent.y), extent.z);

        if (maxExtent <= 0.0f) {
            throw std::runtime_error("mesh has zero-size bounds");
        }

        for (Vertex& vertex : _vertices) {
            glm::vec3 pos{vertex.pos[0], vertex.pos[1], vertex.pos[2]};
            pos = (pos - center) / maxExtent;
            vertex.pos[0] = pos.x;
            vertex.pos[1] = pos.y;
            vertex.pos[2] = pos.z;
        }
    }

    std::vector<Vertex> VkEngineMesh::getVertices() {
        return _vertices;
    }

};  // namespace mat