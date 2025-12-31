#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

class PipelineBuilder {
   public:
    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;

    vk::PipelineInputAssemblyStateCreateInfo m_inputAssembly;
    vk::PipelineRasterizationStateCreateInfo m_rasterizer;
    vk::PipelineColorBlendAttachmentState    m_colorBlendAttachment;
    vk::PipelineMultisampleStateCreateInfo   m_multisampling;
    vk::PipelineLayout                       m_pipelineLayout;
    vk::PipelineDepthStencilStateCreateInfo  m_depthStencil;
    vk::PipelineRenderingCreateInfo          m_renderInfo;
    vk::Format                               m_colorAttachmentFormat;

   public:
    PipelineBuilder() { clear(); };

    void               clear();
    vk::raii::Pipeline build(vk::raii::Device& device);
    void               setShaders(vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader);
    void               setInputTopology(vk::PrimitiveTopology topology);
    void               setPolygonMode(vk::PolygonMode mode);
    void               setCullMode(vk::CullModeFlagBits cullMode, vk::FrontFace frontFace);
    void               setMultiSamplingNone();
    void               disableBlending();
    void               setColorAttachmentFormat(vk::Format format);
    void               setDepthFormat(vk::Format format);
    void               disableDepthTest();
};
