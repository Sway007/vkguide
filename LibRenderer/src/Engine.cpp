#include "../include/Engine.hpp"

#include <math.h>

#include <array>
#include <iostream>

#include "../include/Loder.hpp"
#include "../include/PipelineBuilder.hpp"

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

Engine::~Engine() {
    m_device.waitIdle();
#ifndef VK_USE_PLATFORM_METAL_EXT
    ImGui_ImplVulkan_Shutdown();
#endif
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

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceVulkan13Features>
        featureChain{
            vk::PhysicalDeviceFeatures2{},
            {.bufferDeviceAddress = true},
            {.synchronization2 = true, .dynamicRendering = true},
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
    initDescriptors();
    initPipelines();
    initDefaultData();
}

void Engine::initDefaultData() {
    std::array<Vertex, 4> rectVertices;
    rectVertices[0].position = {0.5, -0.5, 0};
    rectVertices[1].position = {0.5, 0.5, 0};
    rectVertices[2].position = {-0.5, -0.5, 0};
    rectVertices[3].position = {-0.5, 0.5, 0};

    rectVertices[0].color = {0, 0, 0, 1};
    rectVertices[1].color = {0.5, 0.5, 0.5, 1};
    rectVertices[2].color = {1, 0, 0, 1};
    rectVertices[3].color = {0, 1, 0, 1};

    std::array<uint32_t, 6> rectIndices = {0, 1, 2, 2, 1, 3};

    m_rectangle = uploadMesh(rectIndices, rectVertices);

    testMeshes = loadGltfMeshes(this, std::filesystem::path(MODEL_DIR "/basicmesh.glb")).value();
}

void Engine::initPipelines() {
    initComputePipeline();
    // initTrianglePipeline();
    initMeshPipeline();
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
        if(surfaceFormat.format == vk::Format::eB8G8R8A8Unorm &&
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

    m_drawImage.format = vk::Format::eR16G16B16A16Sfloat,
    m_drawImage.imageExtent = {.width = m_swapchainExtent.width, .height = m_swapchainExtent.height, .depth = 1};

    auto imageCreateInfo = vkStructsUtils::makeImageCreateInfo(
        m_drawImage.format,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eStorage |
            vk::ImageUsageFlagBits::eColorAttachment,
        m_drawImage.imageExtent);
    m_drawImage.image = vk::raii::Image(m_device, imageCreateInfo);

    auto                   memRequirements = m_drawImage.image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = utils::findMemoryTypeIndex(m_chosenGPU, memRequirements.memoryTypeBits,
                                                      vk::MemoryPropertyFlagBits::eDeviceLocal),
    };
    m_drawImage.imageMemory = vk::raii::DeviceMemory(m_device, allocInfo);
    m_drawImage.image.bindMemory(m_drawImage.imageMemory, 0);

    auto imageViewCreateInfo =
        vkStructsUtils::makeImageViewCreateInfo(m_drawImage.format, m_drawImage.image, vk::ImageAspectFlagBits::eColor);
    m_drawImage.imageView = vk::raii::ImageView(m_device, imageViewCreateInfo);

    m_depthImage.format = vk::Format::eD32Sfloat;
    m_depthImage.imageExtent = m_drawImage.imageExtent;
    vk::ImageUsageFlags depthImageUseages{};
    depthImageUseages |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
    auto depthImageInfo =
        vkStructsUtils::makeImageCreateInfo(m_depthImage.format, depthImageUseages, m_depthImage.imageExtent);
    m_depthImage.image = vk::raii::Image(m_device, depthImageInfo);
    memRequirements = m_depthImage.image.getMemoryRequirements();
    vk::MemoryAllocateInfo depthAllocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = utils::findMemoryTypeIndex(m_chosenGPU, memRequirements.memoryTypeBits,
                                                      vk::MemoryPropertyFlagBits::eDeviceLocal),
    };
    m_depthImage.imageMemory = vk::raii::DeviceMemory(m_device, depthAllocInfo);
    m_depthImage.image.bindMemory(m_depthImage.imageMemory, 0);

    auto depthImageViewInfo = vkStructsUtils::makeImageViewCreateInfo(m_depthImage.format, m_depthImage.image,
                                                                      vk::ImageAspectFlagBits::eDepth);
    m_depthImage.imageView = vk::raii::ImageView(m_device, depthImageViewInfo);

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

    m_imCommandPool = vk::raii::CommandPool(m_device, poolInfo);
    allocInfo.commandPool = m_imCommandPool;
    allocInfo.commandBufferCount = 1;
    m_imCommandBuffer = std::move(m_device.allocateCommandBuffers(allocInfo).front());
    m_imFence = vk::raii::Fence(m_device, fenceInfo);
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

    imageUtils::transitionImage(cmd, m_drawImage.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    drawBackground(cmd, m_drawImage.image);

    imageUtils::transitionImage(cmd, m_drawImage.image, vk::ImageLayout::eGeneral,
                                vk::ImageLayout::eColorAttachmentOptimal);
    imageUtils::transitionImage(cmd, m_depthImage.image, vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eDepthAttachmentOptimal);

    drawGeometry(cmd);

    imageUtils::transitionImage(cmd, m_drawImage.image, vk::ImageLayout::eColorAttachmentOptimal,
                                vk::ImageLayout::eTransferSrcOptimal);
    imageUtils::transitionImage(cmd, acquiredSwapchainImage, vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eTransferDstOptimal);
    imageUtils::copyImageToImage(cmd, m_drawImage.image, acquiredSwapchainImage,
                                 {.width = m_drawImage.imageExtent.width, .height = m_drawImage.imageExtent.height},
                                 m_swapchainExtent);
#ifdef VK_USE_PLATFORM_METAL_EXT
    imageUtils::transitionImage(cmd, acquiredSwapchainImage, vk::ImageLayout::eTransferDstOptimal,
                                vk::ImageLayout::ePresentSrcKHR);
#else
    imageUtils::transitionImage(cmd, acquiredSwapchainImage, vk::ImageLayout::eTransferDstOptimal,
                                vk::ImageLayout::eColorAttachmentOptimal);
    drawImGui(cmd, m_swapchainImageViews[swapchainImageIndex]);
    imageUtils::transitionImage(cmd, acquiredSwapchainImage, vk::ImageLayout::eColorAttachmentOptimal,
                                vk::ImageLayout::ePresentSrcKHR);
#endif

    cmd.end();

    auto cmdInfo = vkStructsUtils::makeCommandBufferSubmitInfo(cmd);
    auto waitInfo = vkStructsUtils::makeSemaphoreSubmitInfo(vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                            currentFrameData.swapchainSemaphore);
    auto signalInfo =
        vkStructsUtils::makeSemaphoreSubmitInfo(vk::PipelineStageFlagBits2::eAllGraphics, swapchainRenderSemaphore);
    auto submitInfo = vkStructsUtils::makeSubmitInfo(&cmdInfo, &signalInfo, &waitInfo);
    m_graphicsQueue.submit2(submitInfo, currentFrameData.renderFence);

    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchainRenderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &*m_swapchain,
        .pImageIndices = &swapchainImageIndex,
    };
    VK_CHECK(m_graphicsQueue.presentKHR(presentInfo));
    m_frameNumber++;
}

void Engine::drawBackground(vk::CommandBuffer cmd, vk::Image image) {
    ComputeEffect& effect = m_backgroundEffects[m_currentBackgroundEffect];

    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, effect.pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_gradientPipelineLayout, 0, *m_drawImageDescriptorSet,
                           nullptr);
    cmd.pushConstants(m_gradientPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(ComputePushConstants),
                      &effect.data);

    cmd.dispatch(std::ceil(m_drawImage.imageExtent.width / 16.f), std::ceil(m_drawImage.imageExtent.height / 16.f), 1);
}

void Engine::initDescriptors() {
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes{
        {vk::DescriptorType::eStorageImage, 1},
    };
    m_globalDescriptorAllocator.initPool(m_device, 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, vk::DescriptorType::eStorageImage);
        m_drawImageDescriptorSetLayout = builder.build(m_device, vk::ShaderStageFlagBits::eCompute);
    }

    m_drawImageDescriptorSet = m_globalDescriptorAllocator.allocate(m_device, m_drawImageDescriptorSetLayout);
    vk::DescriptorImageInfo imageInfo{
        .imageView = m_drawImage.imageView,
        .imageLayout = vk::ImageLayout::eGeneral,
    };
    vk::WriteDescriptorSet drawImageWrite{
        .dstSet = m_drawImageDescriptorSet,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .pImageInfo = &imageInfo,
    };
    m_device.updateDescriptorSets(drawImageWrite, {});
}

void Engine::initComputePipeline() {
    vk::PushConstantRange pushConstant{
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(ComputePushConstants),
    };

    vk::PipelineLayoutCreateInfo computeLayoutInfo{
        .setLayoutCount = 1,
        .pSetLayouts = &*m_drawImageDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant,
    };
    m_gradientPipelineLayout = vk::raii::PipelineLayout(m_device, computeLayoutInfo);

    auto gradientShaderModule = utils::loadShaderModule(SHADER_DIR "/gradient_color.comp.spv", m_device);
    vk::PipelineShaderStageCreateInfo stageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = gradientShaderModule,
        .pName = "main",
    };
    auto skyShaderModule = utils::loadShaderModule(SHADER_DIR "/sky.comp.spv", m_device);

    vk::ComputePipelineCreateInfo computePipelineCreateInfo{
        .stage = stageInfo,
        .layout = m_gradientPipelineLayout,
    };

    ComputeEffect gradient{
        .name = "gradient",
        .layout = m_gradientPipelineLayout,
        .data =
            {
                .data1 = glm::vec4(1, 0, 0, 1),
                .data2 = glm::vec4(0, 1, 1, 1),
            },
    };
    gradient.pipeline = m_device.createComputePipeline(nullptr, computePipelineCreateInfo);

    computePipelineCreateInfo.stage.module = skyShaderModule;
    ComputeEffect sky{
        .name = "sky",
        .layout = m_gradientPipelineLayout,
        .data = {.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97)},
    };
    sky.pipeline = m_device.createComputePipeline(nullptr, computePipelineCreateInfo);

    m_backgroundEffects.push_back(std::move(gradient));
    m_backgroundEffects.push_back(std::move(sky));
}

#ifndef VK_USE_PLATFORM_METAL_EXT
void Engine::initImGUI(SDL_Window* pWindow) {
    vk::DescriptorPoolSize poolSize[] = {
        {.type = vk::DescriptorType::eSampler, .descriptorCount = 500},
        {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 500},
        {.type = vk::DescriptorType::eSampledImage, .descriptorCount = 500},
        {.type = vk::DescriptorType::eStorageImage, .descriptorCount = 500},
        {.type = vk::DescriptorType::eUniformTexelBuffer, .descriptorCount = 500},
        {.type = vk::DescriptorType::eStorageTexelBuffer, .descriptorCount = 500},
        {.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 500},
        {.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 500},
        {.type = vk::DescriptorType::eUniformBufferDynamic, .descriptorCount = 500},
        {.type = vk::DescriptorType::eInputAttachment, .descriptorCount = 500},
    };
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 500,
        .poolSizeCount = (uint32_t)std::size(poolSize),
        .pPoolSizes = poolSize,
    };
    m_imguiPool = vk::raii::DescriptorPool(m_device, poolInfo);

    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(pWindow);

    VkFormat                         format = static_cast<VkFormat>(m_swapchainImageFormat.format);
    VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance = *m_instance,
        .PhysicalDevice = *m_chosenGPU,
        .Device = *m_device,
        .Queue = *m_graphicsQueue,
        .DescriptorPool = *m_imguiPool,
        .MinImageCount = 3,
        .ImageCount = 3,
        .PipelineInfoMain =
            {
                .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
                .PipelineRenderingCreateInfo = pipelineRenderingCreateInfo,
            },
        .UseDynamicRendering = true,
    };
    ImGui_ImplVulkan_Init(&initInfo);
}

void Engine::drawImGui(vk::CommandBuffer cmd, vk::ImageView targetImageView) {
    auto colorAttachmentInfo =
        vkStructsUtils::makeColorAttachmentInfo(targetImageView, nullptr, vk::ImageLayout::eColorAttachmentOptimal);
    auto renderingInfo = vkStructsUtils::makeRenderingInfo(m_swapchainExtent, &colorAttachmentInfo, nullptr);
    cmd.beginRendering(renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    cmd.endRendering();
}

void Engine::setupGui() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if(ImGui::Begin("background")) {
        ComputeEffect& selected = m_backgroundEffects[m_currentBackgroundEffect];

        ImGui::Text("Selected effect: %s", selected.name);

        ImGui::SliderInt("Effect Index", &m_currentBackgroundEffect, 0, m_backgroundEffects.size() - 1);
        ImGui::InputFloat4("data1", (float*)&selected.data.data1);
        ImGui::InputFloat4("data2", (float*)&selected.data.data2);
        ImGui::InputFloat4("data3", (float*)&selected.data.data3);
        ImGui::InputFloat4("data4", (float*)&selected.data.data4);
    }
    ImGui::End();

    ImGui::Render();
}
#endif

void Engine::drawGeometry(vk::CommandBuffer cmd) {
    auto colorAttachment = vkStructsUtils::makeColorAttachmentInfo(m_drawImage.imageView, nullptr,
                                                                   vk::ImageLayout::eColorAttachmentOptimal);
    auto depthAttachment =
        vkStructsUtils::makeDepthAttachmentInfo(m_depthImage.imageView, vk::ImageLayout::eDepthAttachmentOptimal);
    auto renderingInfo = vkStructsUtils::makeRenderingInfo(
        {.width = m_drawImage.imageExtent.width, .height = m_drawImage.imageExtent.height}, &colorAttachment,
        &depthAttachment);

    cmd.beginRendering(renderingInfo);

    vk::Viewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = m_drawImage.imageExtent.width;
    viewport.height = m_drawImage.imageExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    cmd.setViewport(0, 1, &viewport);

    vk::Rect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = m_drawImage.imageExtent.width;
    scissor.extent.height = m_drawImage.imageExtent.height;
    cmd.setScissor(0, 1, &scissor);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_meshPipeline);
    GPUDrawPushConstants pushConstants{
        .worldMatrix = glm::mat4{1.f},
        .vertexBuffer = m_rectangle.vertexBufferAddress,
    };

    // mat view
    glm::mat4 view = glm::translate(glm::vec3{0, 0, -5});
    glm::mat4 projection =
        glm::perspective(glm::radians(70.f),
                         (float)m_drawImage.imageExtent.width / (float)m_drawImage.imageExtent.height, 0.1f, 10000.f);
    projection[1][1] *= -1;
    pushConstants.worldMatrix = projection * view;

    // mesh draw
    pushConstants.vertexBuffer = testMeshes[2]->meshBuffers.vertexBufferAddress;
    cmd.pushConstants(m_meshPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUDrawPushConstants),
                      &pushConstants);
    cmd.bindIndexBuffer(testMeshes[2]->meshBuffers.indexBuffer.buffer, 0, vk::IndexType::eUint32);
    cmd.drawIndexed(testMeshes[2]->surfaces[0].count, 1, testMeshes[2]->surfaces[0].startIndex, 0, 0);

    cmd.endRendering();
}

AllocatedBuffer Engine::createBuffer(size_t allocSize, vk::BufferUsageFlags usage,
                                     vk::MemoryPropertyFlags memoryProperty) {
    vk::BufferCreateInfo bufferInfo{
        .size = allocSize,
        .usage = usage,
    };
    AllocatedBuffer bufferStruct;
    bufferStruct.buffer = vk::raii::Buffer(m_device, bufferInfo);
    auto memoryRequirements = bufferStruct.buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = utils::findMemoryTypeIndex(m_chosenGPU, memoryRequirements.memoryTypeBits, memoryProperty),
    };

    vk::MemoryAllocateFlagsInfo allocFlagsInfo{
        .flags = vk::MemoryAllocateFlagBits::eDeviceAddress,
    };
    if(usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        allocInfo.pNext = &allocFlagsInfo;
    }

    bufferStruct.bufferMemory = m_device.allocateMemory(allocInfo);
    bufferStruct.buffer.bindMemory(bufferStruct.bufferMemory, 0);
    bufferStruct.usage = usage;
    return bufferStruct;
}

GPUMeshBuffers Engine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices) {
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;
    newSurface.vertexBuffer =
        createBuffer(vertexBufferSize,
                     vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                         vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::BufferDeviceAddressInfo deviceAddressInfo{.buffer = newSurface.vertexBuffer.buffer};
    newSurface.vertexBufferAddress = m_device.getBufferAddress(deviceAddressInfo);

    newSurface.indexBuffer =
        createBuffer(indexBufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
                     vk::MemoryPropertyFlagBits::eDeviceLocal);

    AllocatedBuffer stagingBuffer =
        createBuffer(vertexBufferSize + indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    void* data = stagingBuffer.bufferMemory.mapMemory(0, vertexBufferSize);
    memcpy(data, vertices.data(), vertexBufferSize);
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);
    immediateSubmit([&](vk::CommandBuffer cmd) {
        vk::BufferCopy vertexCopy{.size = vertexBufferSize};
        cmd.copyBuffer(stagingBuffer.buffer, newSurface.vertexBuffer.buffer, vertexCopy);

        vk::BufferCopy indexCopy{.srcOffset = vertexBufferSize, .size = indexBufferSize};
        cmd.copyBuffer(stagingBuffer.buffer, newSurface.indexBuffer.buffer, indexCopy);
    });
    return newSurface;
}

void Engine::immediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function) {
    m_device.resetFences(*m_imFence);
    m_imCommandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    m_imCommandBuffer.begin(beginInfo);

    function(m_imCommandBuffer);

    m_imCommandBuffer.end();
    vk::CommandBufferSubmitInfo cmdSubmitInfo{.commandBuffer = m_imCommandBuffer};

    const auto submitInfo = vkStructsUtils::makeSubmitInfo(&cmdSubmitInfo, nullptr, nullptr);
    m_graphicsQueue.submit2(submitInfo, m_imFence);
    m_device.waitForFences(*m_imFence, vk::True, UINT64_MAX);
}

void Engine::initMeshPipeline() {
    auto triangleFragShader = utils::loadShaderModule(SHADER_DIR "/colored_triangle.frag.spv", m_device);
    auto triangleVertShader = utils::loadShaderModule(SHADER_DIR "/colored_triangle_mesh.vert.spv", m_device);

    vk::PushConstantRange bufferRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(GPUDrawPushConstants),
    };
    auto pipelineLayoutInfo = vkStructsUtils::makePipelineLayoutCreateInfo();
    pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    m_meshPipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

    PipelineBuilder pipelineBuilder{};

    pipelineBuilder.m_pipelineLayout = m_meshPipelineLayout;
    pipelineBuilder.setShaders(triangleVertShader, triangleFragShader);
    pipelineBuilder.setInputTopology(vk::PrimitiveTopology::eTriangleList);
    pipelineBuilder.setPolygonMode(vk::PolygonMode::eFill);
    pipelineBuilder.setCullMode(vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise);
    pipelineBuilder.setMultiSamplingNone();
    pipelineBuilder.disableBlending();
    // pipelineBuilder.disableDepthTest();
    pipelineBuilder.enableDepthTest(true, vk::CompareOp::eLessOrEqual);
    pipelineBuilder.setColorAttachmentFormat(m_drawImage.format);
    pipelineBuilder.setDepthFormat(m_depthImage.format);
    m_meshPipeline = pipelineBuilder.build(m_device);
}