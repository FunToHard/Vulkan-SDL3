#pragma once

// Standard library includes
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <memory>
#include <stdexcept>
#include <chrono>

// SDL3 includes
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// Vulkan includes
#include <vulkan/vulkan.h>

// GLM mathematics library
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Application constants
namespace VulkanGameEngine {
    // Window configuration
    constexpr uint32_t DEFAULT_WINDOW_WIDTH = 800;
    constexpr uint32_t DEFAULT_WINDOW_HEIGHT = 600;
    constexpr const char* APPLICATION_NAME = "Vulkan 3D Game Engine";
    constexpr const char* ENGINE_NAME = "VulkanGameEngine";
    
    // Vulkan configuration
    constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(1, 0, 0);
    constexpr uint32_t APPLICATION_VERSION = VK_MAKE_VERSION(1, 0, 0);
    constexpr uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_0;
    
    // Performance constants
    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    
    // Debug configuration
    #ifdef NDEBUG
        constexpr bool ENABLE_VALIDATION_LAYERS = false;
    #else
        constexpr bool ENABLE_VALIDATION_LAYERS = true;
    #endif
    
    // Validation layers
    const std::vector<const char*> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    // Required device extensions
    const std::vector<const char*> DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

// Common utility macros
#define VK_CHECK(result, operation) \
    VulkanGameEngine::VulkanUtils::checkVulkanResult(result, operation)

// 3D Vertex structure for pipeline input
namespace VulkanGameEngine {
    /**
     * @brief Vertex structure for 3D rendering
     * 
     * This structure defines the layout of vertex data that will be passed
     * to the vertex shader. Each vertex contains:
     * - Position: 3D coordinates in model space
     * - Color: RGB color values for per-vertex coloring
     * - TexCoord: 2D texture coordinates for texture mapping
     * 
     * The structure provides static methods to describe its layout to Vulkan,
     * which is essential for the graphics pipeline configuration.
     */
    struct Vertex {
        glm::vec3 position;   ///< 3D position (x, y, z)
        glm::vec3 color;      ///< RGB color (r, g, b)
        glm::vec2 texCoord;   ///< Texture coordinates (u, v)
        
        /**
         * @brief Get vertex binding description for Vulkan pipeline
         * 
         * The binding description tells Vulkan how vertex data is organized:
         * - Binding: Which vertex buffer binding this describes
         * - Stride: Size of each vertex in bytes
         * - Input Rate: Per-vertex or per-instance data
         * 
         * @return VkVertexInputBindingDescription for pipeline configuration
         */
        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;  // Vertex buffer binding index
            bindingDescription.stride = sizeof(Vertex);  // Size of each vertex
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // Per-vertex data
            
            return bindingDescription;
        }
        
        /**
         * @brief Get vertex attribute descriptions for Vulkan pipeline
         * 
         * Attribute descriptions tell Vulkan how to extract vertex attributes
         * from the vertex buffer data:
         * - Location: Shader input location (layout(location = X) in shader)
         * - Binding: Which vertex buffer binding to read from
         * - Format: Data type and component count
         * - Offset: Byte offset within the vertex structure
         * 
         * @return Array of attribute descriptions for position, color, and texCoord
         */
        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
            
            // Position attribute (location = 0 in vertex shader)
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 (3x float32)
            attributeDescriptions[0].offset = offsetof(Vertex, position);
            
            // Color attribute (location = 1 in vertex shader)
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 (3x float32)
            attributeDescriptions[1].offset = offsetof(Vertex, color);
            
            // Texture coordinate attribute (location = 2 in vertex shader)
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;     // vec2 (2x float32)
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
            
            return attributeDescriptions;
        }
    };
    
    /**
     * @brief Uniform Buffer Object for MVP matrices
     * 
     * This structure contains the transformation matrices that are passed
     * to shaders as uniform data. The alignas directives ensure proper
     * alignment for Vulkan uniform buffer requirements.
     */
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;       ///< Model transformation matrix
        alignas(16) glm::mat4 view;        ///< View (camera) transformation matrix
        alignas(16) glm::mat4 projection;  ///< Projection transformation matrix
    };
}

// Forward declarations for main engine classes
namespace VulkanGameEngine {
    class VulkanEngine;
    class VulkanInstance;
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanRenderPass;
    class VulkanPipeline;
    class VulkanBuffer;
    class VulkanCommandPool;
    class Logger;
    class MainCharacter;
}