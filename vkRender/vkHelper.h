#pragma once

#include <vector>
#include <fstream>
#include <stdexcept>
#include <filesystem>

std::vector<uint32_t> readSpvFile(const std::filesystem::path& path);