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

namespace utils {
    uint32_t findMemoryTypeIndex(vk::PhysicalDevice gpu, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
}

namespace imageUtils {
    void transitionImage(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout currentLayout,
                         vk::ImageLayout newLayout);
    void copyImageToImage(vk::CommandBuffer cmd, vk::Image src, vk::Image dst, vk::Extent2D srcSize,
                          vk::Extent2D dstSize);
}  // namespace imageUtils

namespace vkStructsUtils {
    inline vk::ImageSubresourceRange makeImageSubresourceRange(vk::ImageAspectFlags aspectMask) {
        return vk::ImageSubresourceRange{
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        };
    };

    inline vk::CommandBufferSubmitInfo makeCommandBufferSubmitInfo(vk::CommandBuffer cmd) {
        return vk::CommandBufferSubmitInfo{
            .commandBuffer = cmd,
        };
    }

    inline vk::SemaphoreSubmitInfo makeSemaphoreSubmitInfo(vk::PipelineStageFlags2 stageMask, vk::Semaphore semaphore) {
        return vk::SemaphoreSubmitInfo{
            .semaphore = semaphore,
            .stageMask = stageMask,
        };
    }

    inline vk::SubmitInfo2 makeSubmitInfo(vk::CommandBufferSubmitInfo* cmd, vk::SemaphoreSubmitInfo* signalSemaphore,
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

    inline vk::ImageCreateInfo makeImageCreateInfo(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent) {
        return vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = usage,
        };
    }

    inline vk::ImageViewCreateInfo makeImageViewCreateInfo(vk::Format format, vk::Image image,
                                                           vk::ImageAspectFlags aspectFlags) {
        return vk::ImageViewCreateInfo{
            .viewType = vk::ImageViewType::e2D,
            .image = image,
            .format = format,
            .subresourceRange =
                {.baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, .aspectMask = aspectFlags},
        };
    }
}  // namespace vkStructsUtils
