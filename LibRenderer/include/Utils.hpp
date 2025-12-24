#include <iostream>
#include <vulkan/vulkan.hpp>

#define VK_CHECK(x)                                                \
    do {                                                           \
        vk::Result err = x;                                        \
        if(err != vk::Result::eSuccess) {                          \
            std::cerr << "Detected Vulkan error: " << err << "\n"; \
            abort();                                               \
        }                                                          \
    } while(0)

namespace imageUtils {
    void transitionImage(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout currentLayout,
                         vk::ImageLayout newLayout);
}  // namespace imageUtils

namespace vkStructsUtils {
    inline vk::ImageSubresourceRange getImageSubresourceRange(vk::ImageAspectFlags aspectMask) {
        return vk::ImageSubresourceRange{
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        };
    };

    inline vk::CommandBufferSubmitInfo getCommandBufferSubmitInfo(vk::CommandBuffer cmd) {
        return vk::CommandBufferSubmitInfo{
            .commandBuffer = cmd,
        };
    }

    inline vk::SemaphoreSubmitInfo getSemaphoreSubmitInfo(vk::PipelineStageFlags2 stageMask, vk::Semaphore semaphore) {
        return vk::SemaphoreSubmitInfo{
            .semaphore = semaphore,
            .stageMask = stageMask,
        };
    }

    inline vk::SubmitInfo2 getSubmitInfo(vk::CommandBufferSubmitInfo* cmd, vk::SemaphoreSubmitInfo* signalSemaphore,
                                         vk::SemaphoreSubmitInfo* waitSemaphore) {
        return vk::SubmitInfo2{
            .waitSemaphoreInfoCount = waitSemaphore == nullptr ? 0 : 1u,
            .pWaitSemaphoreInfos = waitSemaphore,
            .signalSemaphoreInfoCount = signalSemaphore == nullptr ? 0 : 1u,
            .pSignalSemaphoreInfos = signalSemaphore,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = cmd,
        };
    }
}  // namespace vkStructsUtils
