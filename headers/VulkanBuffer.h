#pragma once

#include "Common.h"

namespace VulkanGameEngine {

/**
 * VulkanBuffer manages Vulkan buffer creation, memory allocation, and data transfer.
 * 
 * In Vulkan, buffers are generic memory objects that can store various types of data:
 * - Vertex data (positions, colors, texture coordinates)
 * - Index data (triangle indices for indexed drawing)
 * - Uniform data (transformation matrices, lighting parameters)
 * - Storage data (compute shader input/output)
 * 
 * Unlike other graphics APIs, Vulkan requires explicit memory management. When you
 * create a buffer, you must also:
 * 1. Query memory requirements for the buffer
 * 2. Find a suitable memory type that meets those requirements
 * 3. Allocate device memory
 * 4. Bind the memory to the buffer
 * 
 * This class abstracts these complex operations into a simple interface while
 * providing educational comments about Vulkan's memory model.
 */
class VulkanBuffer {
public:
    /**
     * Buffer usage types for different rendering scenarios.
     * These correspond to VkBufferUsageFlags but provide a more
     * user-friendly interface for common use cases.
     */
    enum class Usage {
        VERTEX_BUFFER,      // Stores vertex data (positions, colors, etc.)
        INDEX_BUFFER,       // Stores index data for indexed drawing
        UNIFORM_BUFFER,     // Stores uniform data (matrices, parameters)
        STAGING_BUFFER,     // Temporary buffer for data transfer
        STORAGE_BUFFER      // General storage for compute shaders
    };

    /**
     * Memory property requirements for different performance scenarios.
     * 
     * Vulkan exposes different types of memory with different characteristics:
     * - DEVICE_LOCAL: Fast GPU memory, not accessible by CPU
     * - HOST_VISIBLE: CPU-accessible memory, may be slower
     * - HOST_COHERENT: CPU changes are automatically visible to GPU
     * - HOST_CACHED: CPU-cached memory for better CPU performance
     */
    enum class MemoryProperty {
        DEVICE_LOCAL,       // Fast GPU memory (best for frequently accessed data)
        HOST_VISIBLE,       // CPU-accessible memory (for data upload)
        HOST_COHERENT,      // Automatically synchronized between CPU/GPU
        STAGING             // Optimal for temporary data transfer
    };

    /**
     * Constructor - initializes buffer to safe defaults
     */
    VulkanBuffer();

    /**
     * Destructor - ensures proper cleanup of buffer resources
     */
    ~VulkanBuffer();

    // Delete copy constructor and copy assignment to prevent accidental copying
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    // Move constructor
    VulkanBuffer(VulkanBuffer&& other) noexcept;

    // Move assignment operator
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    /**
     * Creates a buffer with specified usage and memory properties.
     * 
     * This function performs the complete buffer creation process:
     * 1. Creates the VkBuffer object with appropriate usage flags
     * 2. Queries memory requirements (size, alignment, memory type bits)
     * 3. Finds a suitable memory type that meets requirements and properties
     * 4. Allocates device memory
     * 5. Binds the allocated memory to the buffer
     * 
     * @param device Logical device to create buffer on
     * @param physicalDevice Physical device for memory type queries
     * @param size Size of the buffer in bytes
     * @param usage Intended usage of the buffer (vertex, uniform, etc.)
     * @param memoryProperty Memory property requirements (device local, host visible, etc.)
     */
    void create(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                Usage usage, MemoryProperty memoryProperty);

    /**
     * Creates a buffer and immediately uploads data to it.
     * 
     * This is a convenience function that creates a buffer and copies data
     * to it in one operation. For host-visible memory, it maps the memory,
     * copies the data, and unmaps. For device-local memory, it uses a
     * staging buffer for efficient transfer.
     * 
     * @param device Logical device to create buffer on
     * @param physicalDevice Physical device for memory type queries
     * @param data Pointer to data to upload
     * @param size Size of data in bytes
     * @param usage Intended usage of the buffer
     * @param memoryProperty Memory property requirements
     */
    void createWithData(VkDevice device, VkPhysicalDevice physicalDevice, 
                       const void* data, VkDeviceSize size, Usage usage, MemoryProperty memoryProperty);

    /**
     * Maps buffer memory for CPU access.
     * 
     * Memory mapping allows the CPU to directly access GPU memory. This is only
     * possible with HOST_VISIBLE memory types. The returned pointer can be used
     * to read from or write to the buffer.
     * 
     * Important notes:
     * - Only works with HOST_VISIBLE memory
     * - Must call unmap() when finished
     * - For non-coherent memory, may need to flush/invalidate
     * 
     * @param offset Byte offset into the buffer to start mapping
     * @param size Number of bytes to map (VK_WHOLE_SIZE for entire buffer)
     * @return Pointer to mapped memory, or nullptr on failure
     */
    void* map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    /**
     * Unmaps previously mapped buffer memory.
     * 
     * This must be called after map() when you're finished accessing
     * the buffer memory from the CPU.
     */
    void unmap();

    /**
     * Copies data to the buffer using memory mapping.
     * 
     * This function maps the buffer memory, copies the provided data,
     * and unmaps the memory. It handles the mapping/unmapping automatically
     * and includes proper error checking.
     * 
     * @param data Pointer to source data
     * @param size Number of bytes to copy
     * @param offset Byte offset into buffer to start copying
     */
    void uploadData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    /**
     * Copies data from this buffer to another buffer.
     * 
     * This function records and submits a copy command to transfer data
     * between buffers on the GPU. This is more efficient than CPU-based
     * copying for large amounts of data.
     * 
     * @param device Logical device for command submission
     * @param commandPool Command pool for temporary command buffer allocation
     * @param graphicsQueue Queue to submit copy command to
     * @param dstBuffer Destination buffer
     * @param size Number of bytes to copy
     * @param srcOffset Source offset in bytes
     * @param dstOffset Destination offset in bytes
     */
    void copyTo(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                VulkanBuffer& dstBuffer, VkDeviceSize size, 
                VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

    /**
     * Flushes mapped memory to make CPU writes visible to GPU.
     * 
     * For non-coherent memory types, CPU writes may not be immediately
     * visible to the GPU. This function ensures that CPU writes are
     * flushed and become visible to GPU operations.
     * 
     * @param offset Byte offset to start flushing
     * @param size Number of bytes to flush
     */
    void flush(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    /**
     * Invalidates mapped memory to make GPU writes visible to CPU.
     * 
     * For non-coherent memory types, GPU writes may not be immediately
     * visible to the CPU. This function invalidates the CPU cache to
     * ensure GPU writes become visible to CPU reads.
     * 
     * @param offset Byte offset to start invalidating
     * @param size Number of bytes to invalidate
     */
    void invalidate(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    /**
     * Cleans up all buffer resources.
     * 
     * This function destroys the buffer and frees the associated device memory.
     * It's safe to call multiple times and handles null resources gracefully.
     */
    void cleanup();

    // Getters for buffer properties
    VkBuffer getBuffer() const { return m_buffer; }
    VkDeviceMemory getMemory() const { return m_memory; }
    VkDeviceSize getSize() const { return m_size; }
    Usage getUsage() const { return m_usage; }
    MemoryProperty getMemoryProperty() const { return m_memoryProperty; }
    bool isMapped() const { return m_mappedMemory != nullptr; }

    /**
     * Gets the memory requirements for a buffer with given parameters.
     * 
     * This static utility function can be used to query memory requirements
     * before actually creating a buffer, which is useful for memory planning.
     * 
     * @param device Logical device
     * @param size Buffer size in bytes
     * @param usage Buffer usage flags
     * @return Memory requirements structure
     */
    static VkMemoryRequirements getMemoryRequirements(VkDevice device, VkDeviceSize size, Usage usage);

private:
    // Vulkan handles
    VkDevice m_device;              // Logical device (needed for cleanup)
    VkBuffer m_buffer;              // The Vulkan buffer object
    VkDeviceMemory m_memory;        // Device memory bound to the buffer
    
    // Buffer properties
    VkDeviceSize m_size;            // Size of the buffer in bytes
    Usage m_usage;                  // Buffer usage type
    MemoryProperty m_memoryProperty; // Memory property requirements
    
    // Memory mapping state
    void* m_mappedMemory;           // Pointer to mapped memory (null if not mapped)
    bool m_isCoherent;              // Whether memory is coherent (auto-synchronized)

    /**
     * Converts our Usage enum to Vulkan buffer usage flags.
     * 
     * This function translates our simplified usage types into the
     * appropriate VkBufferUsageFlags that Vulkan expects.
     * 
     * @param usage Our buffer usage type
     * @return Corresponding Vulkan buffer usage flags
     */
    VkBufferUsageFlags getVulkanUsageFlags(Usage usage) const;

    /**
     * Converts our MemoryProperty enum to Vulkan memory property flags.
     * 
     * This function translates our simplified memory property types into
     * the appropriate VkMemoryPropertyFlags that Vulkan expects.
     * 
     * @param property Our memory property type
     * @return Corresponding Vulkan memory property flags
     */
    VkMemoryPropertyFlags getVulkanMemoryPropertyFlags(MemoryProperty property) const;

    /**
     * Finds a suitable memory type for the buffer.
     * 
     * Vulkan devices expose multiple memory types with different properties.
     * This function finds a memory type that satisfies both the buffer's
     * requirements and our desired properties.
     * 
     * The algorithm:
     * 1. Get memory requirements from the buffer
     * 2. Iterate through available memory types
     * 3. Check if type is supported (type filter bit is set)
     * 4. Check if type has required properties
     * 5. Return the first matching type
     * 
     * @param physicalDevice Physical device to query memory types from
     * @param typeFilter Bit field of acceptable memory types (from buffer requirements)
     * @param properties Required memory properties
     * @return Index of suitable memory type
     * @throws std::runtime_error if no suitable memory type is found
     */
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                           VkMemoryPropertyFlags properties) const;

    /**
     * Creates a single-use command buffer for operations like buffer copying.
     * 
     * Some operations (like copying between buffers) require command buffers.
     * This function creates a temporary command buffer, begins recording,
     * and returns it ready for use.
     * 
     * @param device Logical device
     * @param commandPool Command pool to allocate from
     * @return Command buffer ready for recording
     */
    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) const;

    /**
     * Ends recording and submits a single-use command buffer.
     * 
     * This function ends command buffer recording, submits it to the queue,
     * waits for completion, and cleans up the temporary command buffer.
     * 
     * @param device Logical device
     * @param commandPool Command pool the buffer was allocated from
     * @param commandBuffer Command buffer to submit
     * @param queue Queue to submit to
     */
    void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, 
                              VkCommandBuffer commandBuffer, VkQueue queue) const;
};

/**
 * Utility functions for common buffer operations
 */
namespace BufferUtils {
    /**
     * Creates a vertex buffer with the provided vertex data.
     * 
     * This convenience function creates a device-local vertex buffer and
     * uploads the provided vertex data using a staging buffer for optimal
     * performance.
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param commandPool Command pool for data transfer
     * @param graphicsQueue Queue for command submission
     * @param vertices Vector of vertex data
     * @return VulkanBuffer configured as a vertex buffer
     */
    VulkanBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                   VkCommandPool commandPool, VkQueue graphicsQueue,
                                   const std::vector<Vertex>& vertices);

    /**
     * Creates an index buffer with the provided index data.
     * 
     * This convenience function creates a device-local index buffer and
     * uploads the provided index data using a staging buffer.
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param commandPool Command pool for data transfer
     * @param graphicsQueue Queue for command submission
     * @param indices Vector of index data
     * @return VulkanBuffer configured as an index buffer
     */
    VulkanBuffer createIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                  VkCommandPool commandPool, VkQueue graphicsQueue,
                                  const std::vector<uint32_t>& indices);

    /**
     * Creates a uniform buffer for transformation matrices.
     * 
     * This convenience function creates a host-visible uniform buffer
     * suitable for frequently updated data like transformation matrices.
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @return VulkanBuffer configured as a uniform buffer
     */
    VulkanBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice);

    /**
     * Creates a staging buffer for data transfer operations.
     * 
     * Staging buffers are temporary host-visible buffers used to transfer
     * data to device-local buffers efficiently.
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param size Size of the staging buffer
     * @return VulkanBuffer configured as a staging buffer
     */
    VulkanBuffer createStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                    VkDeviceSize size);
}

/**
 * Advanced vertex buffer management utilities
 */
namespace VertexBufferManager {
    /**
     * Creates separate buffers for different vertex attributes.
     * Useful for advanced rendering techniques where attributes
     * are stored in separate buffers rather than interleaved.
     */
    std::vector<VulkanBuffer> createSeparateAttributeBuffers(
        VkDevice device, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue graphicsQueue,
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec3>& colors,
        const std::vector<glm::vec2>& texCoords);

    /**
     * Updates vertex buffer data efficiently based on buffer type.
     */
    void updateVertexBuffer(VulkanBuffer& vertexBuffer,
                           VkDevice device, VkPhysicalDevice physicalDevice,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           const std::vector<Vertex>& newVertices,
                           VkDeviceSize offset = 0);
}

/**
 * Uniform buffer management utilities for MVP matrices
 */
namespace UniformBufferManager {
    /**
     * Creates uniform buffers for multiple frames in flight.
     */
    std::vector<VulkanBuffer> createUniformBuffersForFramesInFlight(
        VkDevice device, VkPhysicalDevice physicalDevice, uint32_t framesInFlight);

    /**
     * Updates uniform buffer with transformation matrices.
     */
    void updateUniformBuffer(VulkanBuffer& uniformBuffer,
                            const glm::mat4& model,
                            const glm::mat4& view,
                            const glm::mat4& projection);

    /**
     * Creates a dynamic uniform buffer for multiple objects.
     */
    VulkanBuffer createDynamicUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                           uint32_t objectCount);

    /**
     * Updates specific object data in dynamic uniform buffer.
     */
    void updateDynamicUniformBuffer(VulkanBuffer& dynamicUniformBuffer,
                                   VkPhysicalDevice physicalDevice,
                                   uint32_t objectIndex,
                                   const glm::mat4& model,
                                   const glm::mat4& view,
                                   const glm::mat4& projection);
}

} // namespace VulkanGameEngine