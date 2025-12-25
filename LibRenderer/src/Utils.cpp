#include "../include/Utils.hpp"

uint32_t utils::findMemoryTypeIndex(vk::PhysicalDevice gpu, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    auto memProperties = gpu.getMemoryProperties();
    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void imageUtils::transitionImage(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout currentLayout,
                                 vk::ImageLayout newLayout) {
    vk::ImageAspectFlags aspectMask = newLayout == vk::ImageLayout::eDepthAttachmentOptimal
                                          ? vk::ImageAspectFlagBits::eDepth
                                          : vk::ImageAspectFlagBits::eColor;
    auto                 subresourceRange = vkStructsUtils::makeImageSubresourceRange(aspectMask);

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

void imageUtils::copyImageToImage(vk::CommandBuffer cmd, vk::Image src, vk::Image dst, vk::Extent2D srcSize,
                                  vk::Extent2D dstSize) {
    vk::ImageBlit2 blitRegion{
        .srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1, .mipLevel = 0},
        .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1, .mipLevel = 0},
    };
    blitRegion.srcOffsets[1] = {
        .x = static_cast<int32_t>(srcSize.width),
        .y = static_cast<int32_t>(srcSize.height),
        .z = 1,
    };
    blitRegion.dstOffsets[1] = {
        .x = static_cast<int32_t>(dstSize.width),
        .y = static_cast<int32_t>(dstSize.height),
        .z = 1,
    };

    vk::BlitImageInfo2 blitInfo{
        .srcImage = src,
        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
        .dstImage = dst,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &blitRegion,
        .filter = vk::Filter::eLinear,
    };
    cmd.blitImage2(blitInfo);
}