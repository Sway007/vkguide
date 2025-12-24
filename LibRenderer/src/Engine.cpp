#include "../include/Engine.hpp"

#include <math.h>

#include <array>
#include <iostream>

Engine::Engine(const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
    vk::ApplicationInfo appInfo{
        .pEngineName = "SWAY",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 3, 0),
    };

    vk::InstanceCreateInfo instanceInfo{
        .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    m_instance = vk::raii::Instance(m_context, instanceInfo);

    std::cout << "Success to create vulkan instance.\n";
}

#ifndef VK_USE_PLATFORM_METAL_EXT
void Engine::initWithSurface(VkSurfaceKHR surface) {
    m_surface = vk::raii::SurfaceKHR(m_instance, surface);
    std::cout << "init with surface.\n";
    initVulkan();
}
#endif

void Engine::initVulkan() {
    auto gpus = m_instance.enumeratePhysicalDevices();
    m_chosenGPU = std::move(gpus[0]);

    vk::StructureChain<vk::PhysicalDeviceVulkan13Features> featureChain{
        {.dynamicRendering = true, .synchronization2 = true},
    };

    auto graphicsQueueIndex = getGraphicsQueueFamilyIndex();

    float                     queuePriority = 0.0;
    vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = graphicsQueueIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    std::array deviceExtensions = {vk::KHRSwapchainExtensionName, vk::KHRSynchronization2ExtensionName,
                                   "VK_KHR_portability_subset"};

    vk::DeviceCreateInfo deviceInfo{
        .pNext = featureChain.get(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };
    m_device = vk::raii::Device(m_chosenGPU, deviceInfo);
    m_graphicsQueue = m_device.getQueue(graphicsQueueIndex, 0);

    createSwapchain();
    initFrameDatas();
}

uint32_t Engine::getGraphicsQueueFamilyIndex() {
    auto     queueFamilyProperties = m_chosenGPU.getQueueFamilyProperties();
    uint32_t idx{0};
    for(const auto& prop : queueFamilyProperties) {
        if(prop.queueFlags & vk::QueueFlagBits::eGraphics) {
            return idx;
        }
        ++idx;
    }
    throw std::runtime_error("No graphics queue found!");
}

void Engine::createSwapchain() {
    const auto surfaceCapabilities = m_chosenGPU.getSurfaceCapabilitiesKHR(*m_surface);
    const auto surfaceFormats = m_chosenGPU.getSurfaceFormatsKHR(*m_surface);
    m_swapchainExtent = surfaceCapabilities.currentExtent;

    m_swapchainImageFormat = surfaceFormats[0];
    for(const auto& surfaceFormat : surfaceFormats) {
        if(surfaceFormat.format == vk::Format::eR8G8B8Unorm &&
           surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            m_swapchainImageFormat = surfaceFormat;
            break;
        }
    }

    vk::SwapchainCreateInfoKHR swapchainInfo{
        .surface = *m_surface,
        .minImageCount = std::max(3u, surfaceCapabilities.minImageCount),
        .imageFormat = m_swapchainImageFormat.format,
        .imageColorSpace = m_swapchainImageFormat.colorSpace,
        .imageExtent = m_swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true,
    };
    m_swapchain = vk::raii::SwapchainKHR(m_device, swapchainInfo);
    m_swapchainImages.clear();
    m_swapchainImages = m_swapchain.getImages();
    m_swapchainImageViews.clear();

    vk::ImageViewCreateInfo imageViewInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = m_swapchainImageFormat.format,
        .subresourceRange =
            {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    for(const auto& img : m_swapchainImages) {
        imageViewInfo.image = img;
        m_swapchainImageViews.emplace_back(m_device, imageViewInfo);
    }

    std::cout << "Success to create swapchain.\n";
}

void Engine::initFrameDatas() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = getGraphicsQueueFamilyIndex(),
    };
    m_commandPool = vk::raii::CommandPool(m_device, poolInfo);
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = FRAME_OVERLAP,
    };
    auto commandBuffers = m_device.allocateCommandBuffers(allocInfo);

    vk::FenceCreateInfo fenceInfo{
        .flags = vk::FenceCreateFlagBits::eSignaled,
    };
    vk::SemaphoreCreateInfo semaphoreInfo{};

    for(auto i = 0; i < FRAME_OVERLAP; i++) {
        m_frames[i].commandBuffer = std::move(commandBuffers[i]);
        m_frames[i].renderFence = vk::raii::Fence(m_device, fenceInfo);
        m_frames[i].swapchainSemaphore = vk::raii::Semaphore(m_device, semaphoreInfo);
    }

    m_swapchainRenderSemaphores.clear();
    for(auto i = 0; i < m_swapchainImages.size(); i++) {
        m_swapchainRenderSemaphores.emplace_back(m_device, semaphoreInfo);
    }
}

void Engine::draw() {
    const auto& currentFrameData = getCurrentFame();

    VK_CHECK(m_device.waitForFences(*currentFrameData.renderFence, vk::True, UINT64_MAX));
    m_device.resetFences(*currentFrameData.renderFence);

    auto [result, swapchainImageIndex] =
        m_swapchain.acquireNextImage(UINT16_MAX, currentFrameData.swapchainSemaphore, nullptr);
    VK_CHECK(result);

    vk::Semaphore swapchainRenderSemaphore = m_swapchainRenderSemaphores[swapchainImageIndex];

    auto acquiredSwapchainImage = m_swapchainImages[swapchainImageIndex];

    vk::CommandBuffer cmd = currentFrameData.commandBuffer;
    cmd.reset();
    cmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    imageUtils::transitionImage(cmd, acquiredSwapchainImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
    vk::ClearColorValue clearColor;
    float               flash = std::abs(std::sin(m_currentFrame / 120.f));
    clearColor = {0.f, 0.f, flash, 1.f};
    auto clearRange = vkStructsUtils::getImageSubresourceRange(vk::ImageAspectFlagBits::eColor);

    cmd.clearColorImage(acquiredSwapchainImage, vk::ImageLayout::eGeneral, &clearColor, 1, &clearRange);
    imageUtils::transitionImage(cmd, acquiredSwapchainImage, vk::ImageLayout::eGeneral,
                                vk::ImageLayout::ePresentSrcKHR);
    cmd.end();

    auto cmdInfo = vkStructsUtils::getCommandBufferSubmitInfo(cmd);
    auto waitInfo = vkStructsUtils::getSemaphoreSubmitInfo(vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                           currentFrameData.swapchainSemaphore);
    auto signalInfo =
        vkStructsUtils::getSemaphoreSubmitInfo(vk::PipelineStageFlagBits2::eAllGraphics, swapchainRenderSemaphore);
    auto submitInfo = vkStructsUtils::getSubmitInfo(&cmdInfo, &signalInfo, &waitInfo);
    m_graphicsQueue.submit2(submitInfo, currentFrameData.renderFence);

    vk::PresentInfoKHR presentInfo{
        .swapchainCount = 1,
        .pSwapchains = &*m_swapchain,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchainRenderSemaphore,
        .pImageIndices = &swapchainImageIndex,
    };
    VK_CHECK(m_graphicsQueue.presentKHR(presentInfo));
    m_currentFrame = (m_currentFrame + 1);
}

Engine::~Engine() { m_device.waitIdle(); }