#pragma once

#include "Common.h"
#include "VulkanBuffer.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace VulkanGameEngine {

/**
 * MainCharacter class handles loading and rendering of a 3D character model.
 * 
 * This class provides functionality to:
 * - Load OBJ files with vertex positions, normals, and texture coordinates
 * - Create Vulkan buffers for the loaded geometry
 * - Manage transformation matrices for positioning and animation
 * - Integrate with the existing Vulkan rendering pipeline
 * 
 * The class uses a simple OBJ parser that supports:
 * - Vertex positions (v)
 * - Vertex normals (vn) 
 * - Texture coordinates (vt)
 * - Face indices (f)
 */
class MainCharacter {
public:
    /**
     * Constructor - initializes character to default state
     */
    MainCharacter();

    /**
     * Destructor - ensures proper cleanup of resources
     */
    ~MainCharacter();

    /**
     * Loads a 3D model from an OBJ file.
     * 
     * This function parses an OBJ file and extracts vertex data including
     * positions, normals, and texture coordinates. The data is then used
     * to create Vulkan buffers for rendering.
     * 
     * @param filePath Path to the OBJ file to load
     * @param device Vulkan logical device for buffer creation
     * @param physicalDevice Vulkan physical device for memory allocation
     * @param commandPool Command pool for buffer operations
     * @param graphicsQueue Graphics queue for command submission
     * @return true if loading succeeded, false otherwise
     */
    bool loadFromOBJ(const std::string& filePath,
                     VkDevice device, 
                     VkPhysicalDevice physicalDevice,
                     VkCommandPool commandPool, 
                     VkQueue graphicsQueue);

    /**
     * Updates the character's transformation matrix.
     * 
     * @param position World position of the character
     * @param rotation Rotation angles (pitch, yaw, roll) in radians
     * @param scale Uniform scale factor
     */
    void setTransform(const glm::vec3& position, 
                      const glm::vec3& rotation = glm::vec3(0.0f), 
                      float scale = 1.0f);

    /**
     * Gets the character's current transformation matrix.
     * 
     * @return 4x4 transformation matrix
     */
    const glm::mat4& getTransformMatrix() const { return m_transformMatrix; }

    /**
     * Gets the vertex buffer for rendering.
     * 
     * @return Reference to the vertex buffer
     */
    const VulkanBuffer& getVertexBuffer() const { return m_vertexBuffer; }

    /**
     * Gets the index buffer for rendering.
     * 
     * @return Reference to the index buffer
     */
    const VulkanBuffer& getIndexBuffer() const { return m_indexBuffer; }

    /**
     * Gets the number of indices for drawing.
     * 
     * @return Number of indices in the index buffer
     */
    uint32_t getIndexCount() const { return m_indexCount; }

    /**
     * Gets the number of vertices in the model.
     * 
     * @return Number of vertices
     */
    uint32_t getVertexCount() const { return m_vertexCount; }

    /**
     * Checks if the model is loaded and ready for rendering.
     * 
     * @return true if model is loaded, false otherwise
     */
    bool isLoaded() const { return m_isLoaded; }

    /**
     * Cleans up all resources.
     * 
     * This function releases all Vulkan buffers and resets the character
     * to an unloaded state.
     */
    void cleanup();

    /**
     * Gets model statistics for debugging.
     * 
     * @param vertexCount Output parameter for vertex count
     * @param triangleCount Output parameter for triangle count
     */
    void getModelStats(uint32_t& vertexCount, uint32_t& triangleCount) const;

    /**
     * Color generation modes for visual variety
     */
    enum class ColorMode {
        RAINBOW,        // Current golden ratio HSV method
        GRADIENT,       // Smooth gradients based on position
        ANATOMICAL,     // Different colors per body part
        METALLIC,       // Metallic/shiny appearance
        PASTEL          // Soft pastel colors
    };

    /**
     * Sets the color generation mode for vertices
     * @param mode The color mode to use
     */
    void setColorMode(ColorMode mode) { m_colorMode = mode; }

private:
    // Model data
    std::vector<Vertex> m_vertices;     ///< Vertex data (positions, colors, tex coords)
    std::vector<uint32_t> m_indices;    ///< Index data for triangles
    
    // Vulkan resources
    VulkanBuffer m_vertexBuffer;        ///< Vertex buffer for GPU
    VulkanBuffer m_indexBuffer;         ///< Index buffer for GPU
    
    // Model state
    bool m_isLoaded;                    ///< Whether model is loaded
    uint32_t m_vertexCount;             ///< Number of vertices
    uint32_t m_indexCount;              ///< Number of indices
    glm::mat4 m_transformMatrix;        ///< Current transformation matrix
    
    // Model properties
    glm::vec3 m_position;               ///< World position
    glm::vec3 m_rotation;               ///< Rotation angles (pitch, yaw, roll)
    float m_scale;                      ///< Uniform scale factor
    ColorMode m_colorMode;              ///< Current color generation mode

    /**
     * OBJ file parsing structure to hold temporary data during loading.
     */
    struct OBJData {
        std::vector<glm::vec3> positions;    ///< Vertex positions from OBJ
        std::vector<glm::vec3> normals;      ///< Vertex normals from OBJ  
        std::vector<glm::vec2> texCoords;    ///< Texture coordinates from OBJ
        std::vector<uint32_t> indices;       ///< Face indices from OBJ
        
        void clear() {
            positions.clear();
            normals.clear();
            texCoords.clear();
            indices.clear();
        }
    };

    /**
     * Parses an OBJ file and extracts vertex data.
     * 
     * This function reads an OBJ file line by line and extracts:
     * - Vertex positions (v x y z)
     * - Vertex normals (vn x y z)
     * - Texture coordinates (vt u v)
     * - Face definitions (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3)
     * 
     * @param filePath Path to the OBJ file
     * @param objData Output structure to store parsed data
     * @return true if parsing succeeded, false otherwise
     */
    bool parseOBJFile(const std::string& filePath, OBJData& objData);

    /**
     * Converts OBJ data to Vulkan vertex format.
     * 
     * This function takes the raw OBJ data and converts it to the Vertex
     * structure format expected by the Vulkan pipeline. It handles index
     * mapping and creates a unified vertex buffer.
     * 
     * @param objData Raw OBJ data from file parsing
     */
    void convertOBJToVertices(const OBJData& objData);

    /**
     * Creates Vulkan buffers from vertex data.
     * 
     * This function creates the vertex and index buffers on the GPU
     * and uploads the mesh data for rendering.
     * 
     * @param device Vulkan logical device
     * @param physicalDevice Vulkan physical device
     * @param commandPool Command pool for buffer operations
     * @param graphicsQueue Graphics queue for command submission
     * @return true if buffer creation succeeded, false otherwise
     */
    bool createBuffers(VkDevice device, 
                       VkPhysicalDevice physicalDevice,
                       VkCommandPool commandPool, 
                       VkQueue graphicsQueue);

    /**
     * Updates the transformation matrix based on position, rotation, and scale.
     */
    void updateTransformMatrix();

    /**
     * Enhanced color generation with multiple modes
     */
    glm::vec3 generateDefaultColor(uint32_t vertexIndex, const glm::vec3& position) const;

    glm::vec3 generateRainbowColor(uint32_t vertexIndex) const;
    glm::vec3 generateGradientColor(const glm::vec3& position) const;
    glm::vec3 generateAnatomicalColor(const glm::vec3& position) const;
    glm::vec3 generateMetallicColor(uint32_t vertexIndex) const;
    glm::vec3 generatePastelColor(uint32_t vertexIndex) const;

    /**
     * Parses a face line from OBJ file.
     * 
     * Handles different face formats:
     * - f v1 v2 v3 (positions only)
     * - f v1/vt1 v2/vt2 v3/vt3 (positions and tex coords)
     * - f v1//vn1 v2//vn2 v3//vn3 (positions and normals)
     * - f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 (all attributes)
     * 
     * @param line Face definition line from OBJ file
     * @param objData Output structure to store face indices
     * @return true if parsing succeeded, false otherwise
     */
    bool parseFaceLine(const std::string& line, OBJData& objData);

    /**
     * Validates that the loaded model data is consistent.
     * 
     * @return true if data is valid, false otherwise
     */
    bool validateModelData() const;
};

} // namespace VulkanGameEngine} // namespace VulkanGameEngine