#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <filesystem>
#include <glm/gtx/transform.hpp>
#include <vulkan/vulkan_raii.hpp>

#ifndef VK_USE_PLATFORM_METAL_EXT
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#endif

#include "Loder.hpp"
#include "Structs.hpp"
#include "Utils.hpp"

#ifndef VK_USE_PLATFORM_METAL_EXT
#endif

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

    AllocatedImage                m_depthImage;
    AllocatedImage                m_drawImage;
    DescriptorAllocator           m_globalDescriptorAllocator;
    vk::raii::DescriptorSet       m_drawImageDescriptorSet = nullptr;
    vk::raii::DescriptorSetLayout m_drawImageDescriptorSetLayout = nullptr;

    vk::raii::Pipeline       m_gradientPipeline = nullptr;
    vk::raii::PipelineLayout m_gradientPipelineLayout = nullptr;

    std::vector<ComputeEffect> m_backgroundEffects;
    int                        m_currentBackgroundEffect = 0;

#ifndef VK_USE_PLATFORM_METAL_EXT
    vk::raii::DescriptorPool m_imguiPool = nullptr;
#endif

    // immediate command structs
    vk::raii::Fence         m_imFence = nullptr;
    vk::raii::CommandPool   m_imCommandPool = nullptr;
    vk::raii::CommandBuffer m_imCommandBuffer = nullptr;

    // Mesh pipeline realted
    vk::raii::PipelineLayout m_meshPipelineLayout = nullptr;
    vk::raii::Pipeline       m_meshPipeline = nullptr;
    GPUMeshBuffers           m_rectangle;

    std::vector<std::shared_ptr<MeshAsset>> testMeshes;

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
    void initImGUI(SDL_Window* pWindow);
    void drawImGui(vk::CommandBuffer cmd, vk::ImageView targetImageView);
    void setupGui();
#else
    void init();
#endif

    void           draw();
    GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

   private:
    void            initVulkan();
    uint32_t        getGraphicsQueueFamilyIndex();
    void            createSwapchain();
    FrameData&      getCurrentFame() { return m_frames[m_frameNumber % FRAME_OVERLAP]; }
    void            initFrameDatas();
    void            drawBackground(vk::CommandBuffer cmd, vk::Image image);
    void            initDescriptors();
    void            initComputePipeline();
    void            drawGeometry(vk::CommandBuffer cmd);
    AllocatedBuffer createBuffer(size_t allocSize, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memoryProperty);
    void            immediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function);
    void            initMeshPipeline();
    void            initPipelines();
    void            initDefaultData();
};
