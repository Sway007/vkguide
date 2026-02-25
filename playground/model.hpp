#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>

struct Accessor {
    uint32_t             bufferView;
    uint32_t             byteOffset;
    uint32_t             componentType;
    uint32_t             count;
    std::string          type;
    std::vector<int32_t> max;
    std::vector<int32_t> min;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Accessor, bufferView, byteOffset, componentType, count, type, max, min)

struct Asset {
    std::string version;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Asset, version)

struct BufferView {
    uint32_t                buffer;
    uint32_t                byteOffset;
    uint32_t                byteLength;
    std::optional<uint32_t> target;  // 可选字段
};

// 手动实现 from_json 处理可选字段
inline void from_json(const nlohmann::json& j, BufferView& bv) {
    j.at("buffer").get_to(bv.buffer);
    j.at("byteOffset").get_to(bv.byteOffset);
    j.at("byteLength").get_to(bv.byteLength);

    // 检查可选字段是否存在
    if(j.contains("target")) {
        bv.target = j.at("target").get<uint32_t>();
    } else {
        bv.target = std::nullopt;
    }
}

// 手动实现 to_json 处理可选字段
inline void to_json(nlohmann::json& j, const BufferView& bv) {
    j = nlohmann::json{{"buffer", bv.buffer}, {"byteOffset", bv.byteOffset}, {"byteLength", bv.byteLength}};
    if(bv.target.has_value()) {
        j["target"] = bv.target.value();
    }
}

struct Buffer {
    std::string uri;
    uint32_t    byteLength;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Buffer, uri, byteLength)

// glTF attributes 是一个对象/字典，不是数组
// 例如: {"POSITION": 1, "NORMAL": 2, "TEXCOORD_0": 3}
struct Primitive {
    std::map<std::string, uint32_t> attributes;
    uint32_t                        indices;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Primitive, attributes, indices)

struct Mesh {
    std::vector<Primitive> primitives;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mesh, primitives)

struct Node {
    uint32_t mesh;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Node, mesh)

struct Scene {
    std::vector<uint32_t> nodes;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Scene, nodes)

struct gltf {
    uint16_t                scene;
    std::vector<Scene>      scenes;
    std::vector<Node>       nodes;
    std::vector<Mesh>       meshes;
    std::vector<Buffer>     buffers;
    std::vector<BufferView> bufferViews;
    std::vector<Accessor>   accessors;
    Asset                   asset;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(gltf, scene, scenes, nodes, meshes, buffers, bufferViews, accessors, asset)