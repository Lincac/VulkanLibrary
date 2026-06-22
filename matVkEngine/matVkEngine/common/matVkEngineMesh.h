#pragma once

#include <string>
#include <vector>

namespace mat {

    struct Vertex {
        float pos[3];
        float normal[3];
    };

    class VkEngineMesh {
    public:
        VkEngineMesh();
        ~VkEngineMesh();

        void load(const std::string& path);

        void append(const std::string& path);

        void save(const std::string& path);

        void unitization();

        std::vector<Vertex> getVertices();

    private:
        VkEngineMesh(const VkEngineMesh&) = delete;
        VkEngineMesh(VkEngineMesh&&) = delete;
        VkEngineMesh& operator=(const VkEngineMesh&) = delete;
        VkEngineMesh& operator=(VkEngineMesh&&) = delete;

        std::vector<Vertex> _vertices;
    };

};  // namespace mat
