#include <vulkan/vulkan_raii.hpp>

struct FrameData {
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Semaphore     swapchainSemaphore = nullptr;
    vk::raii::Fence         renderFence = nullptr;
};

struct AllocatedImage {
    vk::raii::Image        image = nullptr;
    vk::raii::ImageView    imageView = nullptr;
    vk::raii::DeviceMemory imageMemory = nullptr;
    vk::Extent3D           imageExtent;
    vk::Format             format;
};