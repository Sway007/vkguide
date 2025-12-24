#include "../include/Utils.hpp"

void imageUtils::transitionImage(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout currentLayout,
                                 vk::ImageLayout newLayout) {
    vk::ImageAspectFlags aspectMask = newLayout == vk::ImageLayout::eDepthAttachmentOptimal
                                          ? vk::ImageAspectFlagBits::eDepth
                                          : vk::ImageAspectFlagBits::eColor;
    auto                 subresourceRange = vkStructsUtils::getImageSubresourceRange(aspectMask);

    vk::ImageMemoryBarrier2 imageBarrier{
        .srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask = vk::AccessFlagBits2::eMemoryWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
        .oldLayout = currentLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = subresourceRange,
    };

    vk::DependencyInfo depInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };
    cmd.pipelineBarrier2(depInfo);
}