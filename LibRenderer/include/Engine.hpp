#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

using CAMetalLayer = void;

class Engine {
   public:
    vk::raii::Instance m_instance = nullptr;

   private:
    vk::raii::Context    m_context;
    vk::raii::SurfaceKHR m_surface = nullptr;

   public:
#ifdef VK_USE_PLATFORM_METAL_EXT
    Engine(CAMetalLayer* metalLayer, std::vector<const char*> extensions, std::vector<const char*> layers);
#endif
    Engine(const std::vector<const char*>& extensions, const std::vector<const char*>& layers);

#ifndef VK_USE_PLATFORM_METAL_EXT
    void initWithSurface(VkSurfaceKHR surface);
#else
    void init();
#endif

    void draw();

   private:
    void initVulkan();
};
