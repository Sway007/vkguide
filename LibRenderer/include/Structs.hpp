#include <vulkan/vulkan_raii.hpp>

struct FrameData {
    vk::raii::CommandBuffer commandBuffer = nullptr;
    vk::raii::Semaphore     swapchainSemaphore = nullptr;
    vk::raii::Fence         renderFence = nullptr;
};
