#include <fstream>
#include <iostream>

#include "model.hpp"

using json = nlohmann::json;

int main() {
    std::ifstream f("gltf.json");
    auto          gltfJson = json::parse(f);

    gltf gltfData = std::move(gltfJson.get<gltf>());

    std::cout << (gltfData.bufferViews[0].target.has_value() ? "YES" : "NO") << std::endl;

    return 0;
}
