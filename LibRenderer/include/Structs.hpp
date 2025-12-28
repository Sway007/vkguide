#pragma once

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

struct DescriptorLayoutBuilder {
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, vk::DescriptorType type) {
        vk::DescriptorSetLayoutBinding newBind{
            .binding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
        };
        bindings.push_back(newBind);
    }

    void clear() { bindings.clear(); }

    vk::raii::DescriptorSetLayout build(vk::raii::Device& device, vk::ShaderStageFlags shaderStage) {
        for(auto& b : bindings) {
            b.stageFlags |= shaderStage;
        }

        vk::DescriptorSetLayoutCreateInfo info{
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };
        return vk::raii::DescriptorSetLayout(device, info);
    }
};

struct DescriptorAllocator {
    struct PoolSizeRatio {
        vk::DescriptorType type;
        float              ratio;
    };

    vk::raii::DescriptorPool pool = nullptr;

    void initPool(vk::raii::Device& device, uint32_t maxSet, std::span<PoolSizeRatio> poolSizeSpan) {
        std::vector<vk::DescriptorPoolSize> poolSizes(poolSizeSpan.size());
        size_t                              i = 0;
        for(const auto& size : poolSizeSpan) {
            poolSizes[i++] = vk::DescriptorPoolSize{
                .type = size.type,
                .descriptorCount = uint32_t(size.ratio * maxSet),
            };
        }

        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = maxSet,
            .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes = poolSizes.data(),
        };
        pool = vk::raii::DescriptorPool(device, poolInfo);
    }

    void clearDescriptorSets(vk::Device device) { device.resetDescriptorPool(pool); }

    vk::raii::DescriptorSet allocate(vk::raii::Device& device, vk::DescriptorSetLayout layout) {
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
        };

        auto descriptorSets = device.allocateDescriptorSets(allocInfo);
        return std::move(descriptorSets.front());
    }
};
