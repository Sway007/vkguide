#include "../include/PipelineBuilder.hpp"

#include <array>

#include "../include/Utils.hpp"

void PipelineBuilder::clear() {
    m_inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{};
    m_rasterizer = vk::PipelineRasterizationStateCreateInfo{};
    m_colorBlendAttachment = {};
    m_multisampling = vk::PipelineMultisampleStateCreateInfo{};
    m_pipelineLayout = nullptr;
    m_depthStencil = vk::PipelineDepthStencilStateCreateInfo{};
    m_renderInfo = vk::PipelineRenderingCreateInfo{};
    m_shaderStages.clear();
}

vk::raii::Pipeline PipelineBuilder::build(vk::raii::Device& device) {
    vk::PipelineViewportStateCreateInfo viewport{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &m_colorBlendAttachment,
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    std::array                         states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicInfo{
        .dynamicStateCount = static_cast<uint32_t>(states.size()),
        .pDynamicStates = states.data(),
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &m_renderInfo,
        .stageCount = static_cast<uint32_t>(m_shaderStages.size()),
        .pStages = m_shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &m_inputAssembly,
        .pViewportState = &viewport,
        .pRasterizationState = &m_rasterizer,
        .pMultisampleState = &m_multisampling,
        .pColorBlendState = &colorBlending,
        .pDepthStencilState = &m_depthStencil,
        .layout = m_pipelineLayout,
        .pDynamicState = &dynamicInfo,
    };

    return vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void PipelineBuilder::setShaders(vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) {
    m_shaderStages.clear();
    m_shaderStages.push_back(
        vkStructsUtils::makePipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, vertexShader));
    m_shaderStages.push_back(
        vkStructsUtils::makePipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, fragmentShader));
}

void PipelineBuilder::setInputTopology(vk::PrimitiveTopology topology) {
    m_inputAssembly.topology = topology;
    m_inputAssembly.primitiveRestartEnable = vk::False;
}

void PipelineBuilder::setPolygonMode(vk::PolygonMode mode) {
    m_rasterizer.polygonMode = mode;
    m_rasterizer.lineWidth = 1.0;
}

void PipelineBuilder::setCullMode(vk::CullModeFlagBits cullMode, vk::FrontFace frontFace) {
    m_rasterizer.cullMode = cullMode;
    m_rasterizer.frontFace = frontFace;
}

void PipelineBuilder::setMultiSamplingNone() {
    m_multisampling.sampleShadingEnable = vk::False;

    m_multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    m_multisampling.minSampleShading = 1.0f;
    m_multisampling.pSampleMask = nullptr;
}

void PipelineBuilder::disableBlending() {
    m_colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eA |
                                            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG;
    m_colorBlendAttachment.blendEnable = vk::False;
}

void PipelineBuilder::setColorAttachmentFormat(vk::Format format) {
    m_colorAttachmentFormat = format;
    m_renderInfo.colorAttachmentCount = 1;
    m_renderInfo.pColorAttachmentFormats = &m_colorAttachmentFormat;
}

void PipelineBuilder::setDepthFormat(vk::Format format) { m_renderInfo.depthAttachmentFormat = format; }

void PipelineBuilder::disableDepthTest() {
    m_depthStencil.depthTestEnable = vk::False;
    m_depthStencil.depthWriteEnable = vk::False;
}