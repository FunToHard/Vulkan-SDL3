#include "VulkanPipeline.h"
#include "VulkanUtils.h"
#include <iostream>

namespace VulkanGameEngine {

VulkanPipeline::~VulkanPipeline() {
    cleanup();
}

void VulkanPipeline::createGraphicsPipeline(VkDevice device, 
                                           VkRenderPass renderPass,
                                           const std::string& vertexShaderPath,
                                           const std::string& fragmentShaderPath,
                                           VkExtent2D extent) {
    // Validate input parameters
    if (device == VK_NULL_HANDLE) {
        throw std::runtime_error("Invalid device handle provided to createGraphicsPipeline");
    }
    if (renderPass == VK_NULL_HANDLE) {
        throw std::runtime_error("Invalid render pass handle provided to createGraphicsPipeline");
    }
    if (extent.width == 0 || extent.height == 0) {
        throw std::runtime_error("Invalid extent provided to createGraphicsPipeline");
    }
    
    this->device = device;
    
    VulkanUtils::logObjectCreation("VulkanPipeline", "Graphics Pipeline");
    
    std::cout << "Creating graphics pipeline for 3D rendering..." << std::endl;
    std::cout << "Pipeline will support:" << std::endl;
    std::cout << "  - 3D vertex input (position, color, texture coordinates)" << std::endl;
    std::cout << "  - Depth testing for proper 3D object ordering" << std::endl;
    std::cout << "  - Uniform buffer support for MVP matrices" << std::endl;
    std::cout << "  - Triangle rasterization with back-face culling" << std::endl;
    
    // Step 1: Load and create shader modules
    // Shader modules contain the compiled SPIR-V bytecode that defines
    // the behavior of programmable pipeline stages
    std::cout << "Loading vertex shader: " << vertexShaderPath << std::endl;
    VkShaderModule vertexShaderModule = loadShader(vertexShaderPath);
    
    std::cout << "Loading fragment shader: " << fragmentShaderPath << std::endl;
    VkShaderModule fragmentShaderModule = loadShader(fragmentShaderPath);
    
    // Step 2: Create shader stage info structures
    // These structures tell Vulkan which shader modules to use for which stages
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;  // This is a vertex shader
    vertexShaderStageInfo.module = vertexShaderModule;
    vertexShaderStageInfo.pName = "main";  // Entry point function name in the shader
    
    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;  // This is a fragment shader
    fragmentShaderStageInfo.module = fragmentShaderModule;
    fragmentShaderStageInfo.pName = "main";  // Entry point function name in the shader
    
    // Array of shader stages for pipeline creation
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};
    
    // Step 3: Create descriptor set layout and pipeline layout for uniform buffers
    createDescriptorSetLayout();
    createPipelineLayout();
    
    // Step 4: Configure fixed-function pipeline stages
    // These stages define how vertices are processed and rasterized
    auto vertexInputInfo = createVertexInputInfo();
    auto inputAssemblyInfo = createInputAssemblyInfo();
    auto viewportInfo = createViewportInfo(extent);
    auto rasterizationInfo = createRasterizationInfo();
    auto multisampleInfo = createMultisampleInfo();
    auto depthStencilInfo = createDepthStencilInfo();
    auto colorBlendInfo = createColorBlendInfo();
    
    // Step 5: Create the graphics pipeline
    // The graphics pipeline is the heart of Vulkan rendering. It defines the complete
    // transformation from vertices to pixels, including all fixed-function and
    // programmable stages. This is a complex but powerful system that gives us
    // fine-grained control over the rendering process.
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;  // Vertex and fragment stages
    pipelineInfo.pStages = shaderStages;
    
    // Fixed-function stage configurations
    // These stages define how the GPU processes geometry and fragments
    pipelineInfo.pVertexInputState = &vertexInputInfo;      // How to read vertex data
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;  // How to assemble primitives
    pipelineInfo.pViewportState = &viewportInfo;            // Viewport transformation
    pipelineInfo.pRasterizationState = &rasterizationInfo;  // Rasterization settings
    pipelineInfo.pMultisampleState = &multisampleInfo;      // Anti-aliasing settings
    pipelineInfo.pDepthStencilState = &depthStencilInfo;    // Depth testing for 3D
    pipelineInfo.pColorBlendState = &colorBlendInfo;        // Color blending settings
    pipelineInfo.pDynamicState = nullptr;                   // No dynamic state for now
    
    // Pipeline layout and render pass compatibility
    // The pipeline must be compatible with the render pass it will be used with
    pipelineInfo.layout = pipelineLayout;  // Describes uniform buffers and push constants
    pipelineInfo.renderPass = renderPass;  // Must match the render pass format
    pipelineInfo.subpass = 0;              // Index of subpass in render pass
    
    // Pipeline derivation (for optimization - not used here)
    // Vulkan allows creating pipelines based on existing ones for better performance
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // No base pipeline
    pipelineInfo.basePipelineIndex = -1;               // No base pipeline index
    
    // Create the graphics pipeline
    // This is where all our configuration comes together into a single pipeline object
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, 
                                               nullptr, &graphicsPipeline);
    VK_CHECK_RESULT(result, "Failed to create graphics pipeline");
    
    std::cout << "Graphics pipeline created successfully!" << std::endl;
    std::cout << "Pipeline features:" << std::endl;
    std::cout << "  ✓ Vertex input configured for 3D vertices" << std::endl;
    std::cout << "  ✓ Triangle assembly with back-face culling" << std::endl;
    std::cout << "  ✓ Viewport transformation configured" << std::endl;
    std::cout << "  ✓ Depth testing enabled for 3D rendering" << std::endl;
    std::cout << "  ✓ Color blending configured (opaque rendering)" << std::endl;
    std::cout << "  ✓ Uniform buffer layout created for MVP matrices" << std::endl;
    
    // Step 6: Clean up shader modules
    // Shader modules are only needed during pipeline creation
    // Once the pipeline is created, the modules can be destroyed
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    
    std::cout << "Shader modules cleaned up after pipeline creation" << std::endl;
    
    // Educational note: The graphics pipeline is now complete and ready for rendering!
    // This pipeline object encapsulates the entire rendering state and can be bound
    // to command buffers for drawing operations. The pipeline is immutable once created,
    // which allows the driver to optimize it heavily for performance.
    std::cout << "\n=== Graphics Pipeline Creation Complete ===" << std::endl;
    std::cout << "The pipeline is now ready for 3D rendering operations." << std::endl;
    std::cout << "Key benefits of this Vulkan pipeline:" << std::endl;
    std::cout << "  • Immutable state allows driver optimizations" << std::endl;
    std::cout << "  • Explicit configuration gives precise control" << std::endl;
    std::cout << "  • Supports modern 3D rendering techniques" << std::endl;
    std::cout << "  • Educational comments explain each component" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

void VulkanPipeline::cleanup() {
    if (device != VK_NULL_HANDLE) {
        std::cout << "Cleaning up VulkanPipeline resources..." << std::endl;
        
        // Destroy pipeline first (depends on layout)
        if (graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkPipeline", "Graphics Pipeline");
        }
        
        // Destroy pipeline layout (depends on descriptor set layout)
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkPipelineLayout", "Pipeline Layout");
        }
        
        // Destroy descriptor set layout last
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkDescriptorSetLayout", "Descriptor Set Layout");
        }
        
        std::cout << "VulkanPipeline cleanup complete" << std::endl;
    }
}

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char>& shaderCode) {
    // Create shader module from SPIR-V bytecode
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    
    // SPIR-V bytecode must be aligned to uint32_t boundaries
    // reinterpret_cast is safe here because we know the data is properly aligned
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    
    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    VK_CHECK_RESULT(result, "Failed to create shader module");
    
    return shaderModule;
}

VkShaderModule VulkanPipeline::loadShader(const std::string& shaderPath) {
    // Load SPIR-V bytecode from file
    std::vector<char> shaderCode = VulkanUtils::readFile(shaderPath);
    
    std::cout << "Loaded shader file: " << shaderPath 
              << " (" << shaderCode.size() << " bytes)" << std::endl;
    
    // Create and return shader module
    return createShaderModule(shaderCode);
}

void VulkanPipeline::createDescriptorSetLayout() {
    // Create descriptor set layout for uniform buffer object (MVP matrices)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;  // Binding point in shader (layout(binding = 0))
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;  // Single uniform buffer
    
    // This uniform buffer is accessible from the vertex shader
    // It contains the Model-View-Projection matrices for 3D transformations
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    // No immutable samplers (used for texture sampling)
    uboLayoutBinding.pImmutableSamplers = nullptr;
    
    // Create the descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    
    VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
    VK_CHECK_RESULT(result, "Failed to create descriptor set layout");
    
    VulkanUtils::logObjectCreation("VkDescriptorSetLayout", "Uniform Buffer Layout");
    std::cout << "Created descriptor set layout for uniform buffers (MVP matrices)" << std::endl;
}

void VulkanPipeline::createPipelineLayout() {
    // Create pipeline layout with descriptor set layout for uniform buffers
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    // Include descriptor set layout for uniform buffers
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    
    // No push constants for now (could be added later for small, frequent updates)
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    
    VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
    VK_CHECK_RESULT(result, "Failed to create pipeline layout");
    
    VulkanUtils::logObjectCreation("VkPipelineLayout", "Pipeline Layout");
    std::cout << "Created pipeline layout with uniform buffer support" << std::endl;
}

VkPipelineVertexInputStateCreateInfo VulkanPipeline::createVertexInputInfo() {
    // Configure vertex input for 3D vertices with position, color, and texture coordinates
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    // Get vertex binding and attribute descriptions from Vertex structure
    // This describes how vertex data is laid out in memory and how to interpret it
    static auto bindingDescription = Vertex::getBindingDescription();
    static auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    // Configure vertex input state with our vertex format
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    std::cout << "Configured vertex input for 3D vertices (position, color, texCoord)" << std::endl;
    
    return vertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo VulkanPipeline::createInputAssemblyInfo() {
    // Configure how vertices are assembled into primitives
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    
    // Use triangle lists - most common for 3D rendering
    // Each group of 3 vertices forms a triangle
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    // Don't restart primitives - useful for strip topologies
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
    
    return inputAssemblyInfo;
}

VkPipelineViewportStateCreateInfo VulkanPipeline::createViewportInfo(VkExtent2D extent) {
    // Configure viewport transformation
    // Viewport transforms normalized device coordinates [-1,1] to framebuffer coordinates
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;  // Minimum depth value (near plane)
    viewport.maxDepth = 1.0f;  // Maximum depth value (far plane)
    
    // Configure scissor rectangle
    // Scissor can be used to limit rendering to a specific region
    scissor.offset = {0, 0};
    scissor.extent = extent;
    
    VkPipelineViewportStateCreateInfo viewportStateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;
    
    return viewportStateInfo;
}

VkPipelineRasterizationStateCreateInfo VulkanPipeline::createRasterizationInfo() {
    // Configure rasterization behavior
    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    
    // Don't clamp fragments beyond near/far planes (requires GPU feature)
    rasterizationInfo.depthClampEnable = VK_FALSE;
    
    // Don't discard all fragments (would disable rasterization)
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    
    // Fill polygons with fragments (alternatives: line, point)
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    
    // Line width for wireframe rendering (1.0f for normal rendering)
    rasterizationInfo.lineWidth = 1.0f;
    
    // Cull back faces for performance (front faces are counter-clockwise)
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    
    // Disable depth bias (used for shadow mapping)
    rasterizationInfo.depthBiasEnable = VK_FALSE;
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;
    
    return rasterizationInfo;
}

VkPipelineMultisampleStateCreateInfo VulkanPipeline::createMultisampleInfo() {
    // Configure multisampling for anti-aliasing
    VkPipelineMultisampleStateCreateInfo multisampleInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    
    // Disable multisampling for now (can be enabled later for better quality)
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.minSampleShading = 1.0f;
    multisampleInfo.pSampleMask = nullptr;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.alphaToOneEnable = VK_FALSE;
    
    return multisampleInfo;
}

VkPipelineColorBlendStateCreateInfo VulkanPipeline::createColorBlendInfo() {
    // Configure color blending for transparency effects
    
    // Per-attachment blending configuration
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                         VK_COLOR_COMPONENT_G_BIT | 
                                         VK_COLOR_COMPONENT_B_BIT | 
                                         VK_COLOR_COMPONENT_A_BIT;
    
    // Disable blending for now (opaque rendering)
    // When enabled, this would allow for transparency effects
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    // Global color blending configuration
    VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;  // Disable logical operations
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    
    // Blend constants (used with certain blend factors)
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;
    
    return colorBlendInfo;
}

VkPipelineDepthStencilStateCreateInfo VulkanPipeline::createDepthStencilInfo() {
    // Configure depth and stencil testing for proper 3D rendering
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    
    // Enable depth testing to ensure proper 3D object ordering
    // Objects closer to the camera will be rendered in front of farther objects
    depthStencilInfo.depthTestEnable = VK_TRUE;
    
    // Enable depth writing so that depth values are updated in the depth buffer
    // This is necessary for depth testing to work correctly
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    
    // Use "less than" comparison for depth testing
    // Fragments with smaller depth values (closer to camera) pass the test
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    
    // Disable depth bounds testing (advanced feature for optimization)
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    
    // Disable stencil testing for now (used for advanced effects like shadows)
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};  // Optional stencil operations
    depthStencilInfo.back = {};   // Optional stencil operations
    
    std::cout << "Configured depth testing for proper 3D rendering" << std::endl;
    
    return depthStencilInfo;
}

} // namespace VulkanGameEngine