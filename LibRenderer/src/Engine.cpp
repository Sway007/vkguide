#include "../include/Engine.hpp"

#include <array>
#include <iostream>

Engine::Engine(const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
    vk::ApplicationInfo appInfo{
        .pEngineName = "SWAY",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    vk::InstanceCreateInfo instanceInfo{
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    m_instance = vk::raii::Instance(m_context, instanceInfo);

    std::cout << "Success to create vulkan instance.\n";
}

#ifndef VK_USE_PLATFORM_METAL_EXT
void Engine::initWithSurface(VkSurfaceKHR surface) {
    m_surface = vk::raii::SurfaceKHR(m_instance, surface);
    std::cout << "init with surface.\n";
    initVulkan();
}
#endif

void Engine::draw() {
    // TODO
}

void Engine::initVulkan() {}