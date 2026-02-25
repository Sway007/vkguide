#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <filesystem>

#include "../include/Engine.hpp"
#include "../include/Loder.hpp"

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(Engine* engine, std::filesystem::path filePath) {
    std::cout << "Loading GLTF: " << filePath << std::endl;

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(filePath);

    fastgltf::Asset  gltf;
    fastgltf::Parser parser{};

    auto load = parser.loadGltfBinary(gltfFile.get(), filePath.parent_path(),
                                      fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers);
    if(load) {
        gltf = std::move(load.get());
    } else {
        std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << '\n';
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<uint32_t>                   indices;
    std::vector<Vertex>                     vertices;

    for(const auto& mesh : gltf.meshes) {
        MeshAsset newMesh;
        newMesh.name = mesh.name;

        indices.clear();
        vertices.clear();

        for(const auto& p : mesh.primitives) {
            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            auto initialVtx = vertices.size();

            // load indexed
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(
                    gltf, indexAccessor, [&](std::uint32_t idx) { indices.push_back(idx + initialVtx); });
            }

            // load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index) {
                    Vertex newVtx;
                    newVtx.position = v;
                    newVtx.normal = {1, 0, 0};
                    newVtx.color = glm::vec4{1.f};
                    newVtx.uvX = 0;
                    newVtx.uvY = 0;
                    vertices[initialVtx + index] = newVtx;
                });
            }

            // load vertex normals
            auto normals = p.findAttribute("NORMAL");
            if(normals != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, gltf.accessors[(*normals).accessorIndex],
                    [&](glm::vec3 v, size_t index) { vertices[initialVtx + index].normal = v; });
            }

            // load vertex colors
            auto colors = p.findAttribute("COLOR_0");
            if(colors != p.attributes.end()) {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                    gltf, gltf.accessors[(*colors).accessorIndex],
                    [&](glm::vec4 v, size_t index) { vertices[initialVtx + index].color = v; });
            }

            newMesh.surfaces.push_back(newSurface);
        }

        // display the vertex normals
        constexpr bool OverrideColors = true;
        if(OverrideColors) {
            for(Vertex& vtx : vertices) {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        newMesh.meshBuffers = engine->uploadMesh(indices, vertices);
        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
    }

    return meshes;
}