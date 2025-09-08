#pragma once

#include "Common.h"

namespace VulkanGameEngine {

/**
 * @brief VulkanPipeline manages the graphics pipeline creation and shader loading
 * 
 * The graphics pipeline in Vulkan defines how vertices are processed and how
 * fragments (pixels) are shaded. This class encapsulates the complex pipeline
 * creation process and provides educational comments about each stage.
 * 
 * Key concepts:
 * - Shader Modules: Compiled SPIR-V bytecode loaded into Vulkan
 * - Pipeline Stages: Vertex, Fragment, and other shader stages
 * - Pipeline Layout: Describes uniform buffers and push constants
 * - Render Pass Compatibility: Pipeline must match render pass format
 */
class VulkanPipeline {
public:
    /**
     * @brief Default constructor
     */
    VulkanPipeline() = default;

    /**
     * @brief Destructor - ensures proper cleanup of Vulkan resources
     */
    ~VulkanPipeline();

    /**
     * @brief Create the graphics pipeline with vertex and fragment shaders
     * 
     * This function orchestrates the entire graphics pipeline creation process:
     * 1. Load and create shader modules from SPIR-V files
     * 2. Configure pipeline stages (vertex input, assembly, viewport, etc.)
     * 3. Create pipeline layout for uniform buffers
     * 4. Create the final graphics pipeline object
     * 
     * @param device Logical Vulkan device
     * @param renderPass Compatible render pass for this pipeline
     * @param vertexShaderPath Path to compiled vertex shader (.spv file)
     * @param fragmentShaderPath Path to compiled fragment shader (.spv file)
     * @param extent Swapchain extent for viewport configuration
     */
    void createGraphicsPipeline(VkDevice device, 
                               VkRenderPass renderPass,
                               const std::string& vertexShaderPath,
                               const std::string& fragmentShaderPath,
                               VkExtent2D extent);

    /**
     * @brief Get the graphics pipeline object
     * @return VkPipeline handle for command buffer binding
     */
    VkPipeline getPipeline() const { return graphicsPipeline; }

    /**
     * @brief Get the pipeline layout
     * @return VkPipelineLayout handle for uniform buffer binding
     */
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

    /**
     * @brief Get the descriptor set layout
     * @return VkDescriptorSetLayout handle for descriptor set creation
     */
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    /**
     * @brief Check if the pipeline is valid and ready to use
     * @return true if pipeline is created and valid, false otherwise
     */
    bool isValid() const { 
        return device != VK_NULL_HANDLE && 
               graphicsPipeline != VK_NULL_HANDLE && 
               pipelineLayout != VK_NULL_HANDLE &&
               descriptorSetLayout != VK_NULL_HANDLE;
    }

    /**
     * @brief Clean up all Vulkan resources
     * 
     * Destroys the pipeline and layout in the correct order to avoid
     * validation layer warnings. Called automatically by destructor.
     */
    void cleanup();

private:
    // Vulkan handles
    VkDevice device = VK_NULL_HANDLE;           ///< Logical device reference
    VkPipeline graphicsPipeline = VK_NULL_HANDLE; ///< Graphics pipeline object
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; ///< Pipeline layout for uniforms
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; ///< Layout for uniform buffers

    /**
     * @brief Create a Vulkan shader module from SPIR-V bytecode
     * 
     * Shader modules are Vulkan objects that contain compiled shader code.
     * They are created from SPIR-V bytecode and used to define pipeline stages.
     * 
     * Educational note: SPIR-V is a binary intermediate representation that
     * allows shaders written in different languages (GLSL, HLSL) to be used
     * with Vulkan. This provides better performance and cross-platform compatibility.
     * 
     * @param shaderCode Vector containing SPIR-V bytecode
     * @return VkShaderModule handle for pipeline stage creation
     */
    VkShaderModule createShaderModule(const std::vector<char>& shaderCode);

    /**
     * @brief Load shader from file and create shader module
     * 
     * Convenience function that combines file loading and shader module creation.
     * Provides detailed error messages if shader loading fails.
     * 
     * @param shaderPath Path to the compiled SPIR-V shader file
     * @return VkShaderModule handle
     */
    VkShaderModule loadShader(const std::string& shaderPath);

    /**
     * @brief Create descriptor set layout for uniform buffers
     * 
     * Descriptor set layouts define the types and binding points of resources
     * that shaders can access (uniform buffers, textures, samplers, etc.).
     * This layout describes what resources the shaders expect to receive.
     */
    void createDescriptorSetLayout();

    /**
     * @brief Create pipeline layout for uniform buffers and push constants
     * 
     * The pipeline layout describes the interface between shaders and the
     * application. It defines:
     * - Descriptor set layouts (for uniform buffers, textures, etc.)
     * - Push constant ranges (for small, frequently updated data)
     * 
     * Educational note: This layout must be compatible with the descriptors
     * used when recording command buffers. Mismatches will cause validation errors.
     */
    void createPipelineLayout();

    /**
     * @brief Configure vertex input description
     * 
     * Defines how vertex data is laid out in memory and how it should be
     * interpreted by the vertex shader. This includes:
     * - Binding descriptions (stride, input rate)
     * - Attribute descriptions (location, format, offset)
     * 
     * @return VkPipelineVertexInputStateCreateInfo structure
     */
    VkPipelineVertexInputStateCreateInfo createVertexInputInfo();

    /**
     * @brief Configure input assembly state
     * 
     * Defines how vertices are assembled into primitives (triangles, lines, etc.).
     * For basic 3D rendering, we typically use triangle lists.
     * 
     * @return VkPipelineInputAssemblyStateCreateInfo structure
     */
    VkPipelineInputAssemblyStateCreateInfo createInputAssemblyInfo();

    /**
     * @brief Configure viewport and scissor state
     * 
     * The viewport transforms normalized device coordinates to framebuffer coordinates.
     * The scissor rectangle can be used to limit rendering to a specific region.
     * 
     * @param extent Swapchain extent for viewport dimensions
     * @return VkPipelineViewportStateCreateInfo structure
     */
    VkPipelineViewportStateCreateInfo createViewportInfo(VkExtent2D extent);

    /**
     * @brief Configure rasterization state
     * 
     * Controls how geometry is rasterized into fragments:
     * - Polygon mode (fill, line, point)
     * - Cull mode (back, front, none)
     * - Front face winding order
     * - Depth bias and clamp settings
     * 
     * @return VkPipelineRasterizationStateCreateInfo structure
     */
    VkPipelineRasterizationStateCreateInfo createRasterizationInfo();

    /**
     * @brief Configure multisampling state
     * 
     * Multisampling is used for anti-aliasing. For now, we disable it
     * for simplicity, but it can be enabled later for better visual quality.
     * 
     * @return VkPipelineMultisampleStateCreateInfo structure
     */
    VkPipelineMultisampleStateCreateInfo createMultisampleInfo();

    /**
     * @brief Configure color blending state
     * 
     * Defines how fragment shader output colors are combined with existing
     * framebuffer colors. This is essential for transparency and other effects.
     * 
     * @return VkPipelineColorBlendStateCreateInfo structure
     */
    VkPipelineColorBlendStateCreateInfo createColorBlendInfo();

    /**
     * @brief Configure depth and stencil testing state
     * 
     * Depth testing is essential for proper 3D rendering to ensure that
     * objects closer to the camera are rendered in front of objects that
     * are further away. This prevents z-fighting and ensures correct
     * depth ordering of 3D geometry.
     * 
     * @return VkPipelineDepthStencilStateCreateInfo structure
     */
    VkPipelineDepthStencilStateCreateInfo createDepthStencilInfo();

    // Static viewport and scissor for basic rendering
    VkViewport viewport{};
    VkRect2D scissor{};
    
    // Color blend attachment for single render target
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
};

} // namespace VulkanGameEngine