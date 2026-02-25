
#pragma once
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "./Structs.hpp"

class Engine;

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset {
    std::string             name;
    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers          meshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(Engine* engine, std::filesystem::path filePath);