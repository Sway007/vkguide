#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Structs.hpp"
#include "Utils.hpp"

using CAMetalLayer = void;

/**
 * Frames in flight.
 */
constexpr uint8_t FRAME_OVERLAP = 2;

class Engine {
   public:
    vk::raii::Instance m_instance = nullptr;

   private:
    vk::raii::Context        m_context;
    vk::raii::SurfaceKHR     m_surface = nullptr;
    vk::raii::PhysicalDevice m_chosenGPU = nullptr;
    vk::raii::Device         m_device = nullptr;
    vk::raii::Queue          m_graphicsQueue = nullptr;

    vk::raii::SwapchainKHR           m_swapchain = nullptr;
    vk::SurfaceFormatKHR             m_swapchainImageFormat;
    vk::Extent2D                     m_swapchainExtent;
    std::vector<vk::Image>           m_swapchainImages;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;
    std::vector<vk::raii::Semaphore> m_swapchainRenderSemaphores;

    vk::raii::CommandPool m_commandPool = nullptr;
    FrameData             m_frames[FRAME_OVERLAP];
    uint32_t              m_frameNumber = 0;

    AllocatedImage m_drawImage;

   public:
#ifdef VK_USE_PLATFORM_METAL_EXT
    Engine(CAMetalLayer* metalLayer, std::vector<const char*> extensions, std::vector<const char*> layers);
#endif
    Engine(const std::vector<const char*>& extensions, const std::vector<const char*>& layers);
    Engine()
        : Engine({vk::KHRSurfaceExtensionName, "VK_EXT_metal_surface", vk::KHRPortabilityEnumerationExtensionName},
                 {"VK_LAYER_KHRONOS_validation"}) {};
    ~Engine();

#ifndef VK_USE_PLATFORM_METAL_EXT
    void initWithSurface(VkSurfaceKHR surface);
#else
    void init();
#endif

    void draw();

   private:
    void       initVulkan();
    uint32_t   getGraphicsQueueFamilyIndex();
    void       createSwapchain();
    FrameData& getCurrentFame() { return m_frames[m_frameNumber % FRAME_OVERLAP]; }
    void       initFrameDatas();
    void       drawBackground(vk::CommandBuffer cmd, vk::Image image);
};
