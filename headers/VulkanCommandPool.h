#pragma once

#include "Common.h"
#include <vector>

namespace VulkanGameEngine {

/**
 * VulkanCommandPool manages command buffer allocation and command recording.
 * 
 * In Vulkan, command buffers are used to record GPU commands that will be
 * executed later. Command buffers must be allocated from command pools, which
 * are associated with specific queue families.
 * 
 * Key concepts:
 * - Command Pool: Memory pool for allocating command buffers
 * - Command Buffer: Records a sequence of GPU commands
 * - Queue Family: Determines what types of operations can be recorded
 * - Primary vs Secondary: Primary can be submitted to queues, secondary are called from primary
 * 
 * This class provides a high-level interface for command buffer management while
 * maintaining educational comments about Vulkan's command recording system.
 */
class VulkanCommandPool {
public:
    /**
     * Command buffer usage patterns for optimization hints.
     * These correspond to VkCommandBufferUsageFlags but provide
     * a more user-friendly interface.
     */
    enum class Usage {
        SINGLE_USE,         // Command buffer will be recorded once and submitted once
        REUSABLE,          // Command buffer will be recorded multiple times
        SIMULTANEOUS_USE   // Command buffer can be submitted while still being executed
    };

    /**
     * Command buffer level determines submission capabilities.
     */
    enum class Level {
        PRIMARY,    // Can be submitted directly to queues
        SECONDARY   // Must be called from primary command buffers
    };

    /**
     * Constructor - initializes command pool to safe defaults
     */
    VulkanCommandPool();

    /**
     * Destructor - ensures proper cleanup of command pool resources
     */
    ~VulkanCommandPool();

    /**
     * Creates a command pool for the specified queue family.
     * 
     * Command pools are associated with specific queue families and can only
     * allocate command buffers that will be submitted to queues of that family.
     * 
     * The pool can be configured with different flags:
     * - TRANSIENT: Command buffers are short-lived (good for single-use)
     * - RESET_COMMAND_BUFFER: Individual command buffers can be reset
     * - PROTECTED: For protected memory operations (advanced feature)
     * 
     * @param device Logical device to create the pool on
     * @param queueFamilyIndex Index of the queue family this pool will serve
     * @param allowReset Whether individual command buffers can be reset
     * @param transient Whether command buffers will be short-lived
     */
    void create(VkDevice device, uint32_t queueFamilyIndex, 
                bool allowReset = true, bool transient = false);

    /**
     * Allocates command buffers from the pool.
     * 
     * This function allocates the specified number of command buffers from
     * the pool. The buffers are ready for recording but are in the initial
     * state and must be begun before recording commands.
     * 
     * @param count Number of command buffers to allocate
     * @param level Primary or secondary command buffer level
     * @return Vector of allocated command buffer handles
     */
    std::vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count, Level level = Level::PRIMARY);

    /**
     * Allocates a single command buffer from the pool.
     * 
     * Convenience function for allocating a single command buffer.
     * 
     * @param level Primary or secondary command buffer level
     * @return Handle to the allocated command buffer
     */
    VkCommandBuffer allocateCommandBuffer(Level level = Level::PRIMARY);

    /**
     * Frees command buffers back to the pool.
     * 
     * Command buffers can be freed individually to reclaim memory.
     * Freed command buffers become invalid and must not be used.
     * 
     * @param commandBuffers Vector of command buffers to free
     */
    void freeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers);

    /**
     * Frees a single command buffer back to the pool.
     * 
     * @param commandBuffer Command buffer to free
     */
    void freeCommandBuffer(VkCommandBuffer commandBuffer);

    /**
     * Resets the entire command pool.
     * 
     * This operation resets all command buffers allocated from the pool
     * back to the initial state. This is more efficient than resetting
     * individual command buffers when you want to reset many at once.
     * 
     * @param releaseResources Whether to release resources back to the system
     */
    void reset(bool releaseResources = false);

    /**
     * Begins recording commands into a command buffer.
     * 
     * This function transitions the command buffer from the initial state
     * to the recording state. After calling this, you can record commands
     * into the buffer.
     * 
     * @param commandBuffer Command buffer to begin recording
     * @param usage Usage pattern for optimization hints
     * @param inheritanceInfo Inheritance info for secondary command buffers (optional)
     */
    void beginCommandBuffer(VkCommandBuffer commandBuffer, Usage usage = Usage::REUSABLE,
                           const VkCommandBufferInheritanceInfo* inheritanceInfo = nullptr);

    /**
     * Ends recording commands into a command buffer.
     * 
     * This function transitions the command buffer from the recording state
     * to the executable state. After calling this, the command buffer can
     * be submitted to a queue for execution.
     * 
     * @param commandBuffer Command buffer to end recording
     */
    void endCommandBuffer(VkCommandBuffer commandBuffer);

    /**
     * Creates and begins a single-use command buffer.
     * 
     * This is a convenience function for operations that need a temporary
     * command buffer (like copying data between buffers). The command buffer
     * is allocated, begun, and ready for recording.
     * 
     * @return Command buffer ready for single-use recording
     */
    VkCommandBuffer beginSingleTimeCommands();

    /**
     * Ends and submits a single-use command buffer.
     * 
     * This function ends recording, submits the command buffer to the queue,
     * waits for completion, and frees the command buffer. This is the
     * counterpart to beginSingleTimeCommands().
     * 
     * @param commandBuffer Command buffer to end and submit
     * @param queue Queue to submit the command buffer to
     */
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue);

    /**
     * Records a render pass into a command buffer.
     * 
     * This function records the commands to begin a render pass, which
     * includes setting up the framebuffer and clearing values.
     * 
     * @param commandBuffer Command buffer to record into
     * @param renderPass Render pass to begin
     * @param framebuffer Framebuffer to render into
     * @param renderArea Area of the framebuffer to render to
     * @param clearValues Clear values for attachments
     */
    void beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass,
                        VkFramebuffer framebuffer, VkRect2D renderArea,
                        const std::vector<VkClearValue>& clearValues);

    /**
     * Records the end of a render pass.
     * 
     * @param commandBuffer Command buffer to record into
     */
    void endRenderPass(VkCommandBuffer commandBuffer);

    /**
     * Records a pipeline bind command.
     * 
     * @param commandBuffer Command buffer to record into
     * @param pipeline Graphics or compute pipeline to bind
     * @param bindPoint Pipeline bind point (graphics or compute)
     */
    void bindPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline, 
                     VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

    /**
     * Records vertex buffer binding commands.
     * 
     * @param commandBuffer Command buffer to record into
     * @param firstBinding First vertex input binding
     * @param buffers Vector of vertex buffers to bind
     * @param offsets Vector of byte offsets into each buffer
     */
    void bindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding,
                          const std::vector<VkBuffer>& buffers,
                          const std::vector<VkDeviceSize>& offsets);

    /**
     * Records an index buffer binding command.
     * 
     * @param commandBuffer Command buffer to record into
     * @param buffer Index buffer to bind
     * @param offset Byte offset into the buffer
     * @param indexType Type of indices (16-bit or 32-bit)
     */
    void bindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer,
                        VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);

    /**
     * Records descriptor set binding commands.
     * 
     * @param commandBuffer Command buffer to record into
     * @param pipelineLayout Pipeline layout that defines the descriptor sets
     * @param firstSet Index of the first descriptor set
     * @param descriptorSets Vector of descriptor sets to bind
     * @param dynamicOffsets Vector of dynamic offsets for dynamic descriptors
     */
    void bindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                           uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets,
                           const std::vector<uint32_t>& dynamicOffsets = {});

    /**
     * Records a draw command for non-indexed geometry.
     * 
     * @param commandBuffer Command buffer to record into
     * @param vertexCount Number of vertices to draw
     * @param instanceCount Number of instances to draw
     * @param firstVertex Index of the first vertex
     * @param firstInstance Index of the first instance
     */
    void draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount = 1,
             uint32_t firstVertex = 0, uint32_t firstInstance = 0);

    /**
     * Records a draw command for indexed geometry.
     * 
     * @param commandBuffer Command buffer to record into
     * @param indexCount Number of indices to draw
     * @param instanceCount Number of instances to draw
     * @param firstIndex Index of the first index
     * @param vertexOffset Offset added to vertex indices
     * @param firstInstance Index of the first instance
     */
    void drawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount = 1,
                    uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);

    /**
     * Records a viewport setting command.
     * 
     * The viewport transformation maps normalized device coordinates to framebuffer coordinates.
     * 
     * @param commandBuffer Command buffer to record into
     * @param x X coordinate of the viewport origin
     * @param y Y coordinate of the viewport origin
     * @param width Width of the viewport
     * @param height Height of the viewport
     * @param minDepth Minimum depth value (typically 0.0f)
     * @param maxDepth Maximum depth value (typically 1.0f)
     */
    void setViewport(VkCommandBuffer commandBuffer, float x, float y, float width, float height,
                    float minDepth = 0.0f, float maxDepth = 1.0f);

    /**
     * Records a scissor rectangle setting command.
     * 
     * The scissor test discards fragments outside the scissor rectangle.
     * 
     * @param commandBuffer Command buffer to record into
     * @param x X coordinate of the scissor rectangle origin
     * @param y Y coordinate of the scissor rectangle origin
     * @param width Width of the scissor rectangle
     * @param height Height of the scissor rectangle
     */
    void setScissor(VkCommandBuffer commandBuffer, int32_t x, int32_t y, uint32_t width, uint32_t height);

    /**
     * Records a buffer copy command.
     * 
     * This is useful for copying data between buffers on the GPU.
     * 
     * @param commandBuffer Command buffer to record into
     * @param srcBuffer Source buffer
     * @param dstBuffer Destination buffer
     * @param size Number of bytes to copy
     * @param srcOffset Offset in source buffer
     * @param dstOffset Offset in destination buffer
     */
    void copyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                   VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

    /**
     * Records a memory barrier command.
     * 
     * Memory barriers ensure proper ordering of memory operations.
     * This is important for synchronization between different pipeline stages.
     * 
     * @param commandBuffer Command buffer to record into
     * @param srcStageMask Source pipeline stage mask
     * @param dstStageMask Destination pipeline stage mask
     * @param dependencyFlags Dependency flags
     * @param memoryBarriers Memory barriers
     * @param bufferBarriers Buffer memory barriers
     * @param imageBarriers Image memory barriers
     */
    void pipelineBarrier(VkCommandBuffer commandBuffer,
                        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                        VkDependencyFlags dependencyFlags = 0,
                        const std::vector<VkMemoryBarrier>& memoryBarriers = {},
                        const std::vector<VkBufferMemoryBarrier>& bufferBarriers = {},
                        const std::vector<VkImageMemoryBarrier>& imageBarriers = {});

    /**
     * Records push constants update command.
     * 
     * Push constants are a way to quickly provide a small amount of uniform data
     * to shaders without using descriptor sets.
     * 
     * @param commandBuffer Command buffer to record into
     * @param pipelineLayout Pipeline layout that defines the push constants
     * @param stageFlags Shader stages that will access the push constants
     * @param offset Byte offset into the push constant range
     * @param size Size of the data in bytes
     * @param data Pointer to the data
     */
    void pushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                      VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data);

    /**
     * Records a complete frame rendering sequence.
     * 
     * This is a high-level function that records a typical frame rendering sequence:
     * 1. Begin render pass
     * 2. Bind pipeline
     * 3. Bind vertex/index buffers
     * 4. Bind descriptor sets
     * 5. Draw
     * 6. End render pass
     * 
     * @param commandBuffer Command buffer to record into
     * @param renderPass Render pass to use
     * @param framebuffer Framebuffer to render into
     * @param pipeline Graphics pipeline to use
     * @param pipelineLayout Pipeline layout for descriptor sets
     * @param renderArea Area to render to
     * @param clearValues Clear values for attachments
     * @param vertexBuffers Vertex buffers to bind
     * @param vertexOffsets Offsets for vertex buffers
     * @param indexBuffer Index buffer (optional)
     * @param indexOffset Index buffer offset
     * @param descriptorSets Descriptor sets to bind
     * @param vertexCount Number of vertices (for non-indexed drawing)
     * @param indexCount Number of indices (for indexed drawing)
     * @param instanceCount Number of instances to draw
     */
    void recordFrameCommands(VkCommandBuffer commandBuffer,
                            VkRenderPass renderPass, VkFramebuffer framebuffer,
                            VkPipeline pipeline, VkPipelineLayout pipelineLayout,
                            VkRect2D renderArea, const std::vector<VkClearValue>& clearValues,
                            const std::vector<VkBuffer>& vertexBuffers,
                            const std::vector<VkDeviceSize>& vertexOffsets,
                            VkBuffer indexBuffer = VK_NULL_HANDLE, VkDeviceSize indexOffset = 0,
                            const std::vector<VkDescriptorSet>& descriptorSets = {},
                            uint32_t vertexCount = 0, uint32_t indexCount = 0,
                            uint32_t instanceCount = 1);

    /**
     * Cleans up all command pool resources.
     * 
     * This function destroys the command pool and all command buffers
     * allocated from it. It's safe to call multiple times.
     */
    void cleanup();

    // Getters for command pool properties
    VkCommandPool getCommandPool() const { return m_commandPool; }
    uint32_t getQueueFamilyIndex() const { return m_queueFamilyIndex; }
    bool canReset() const { return m_allowReset; }
    bool isTransient() const { return m_transient; }

private:
    // Vulkan handles
    VkDevice m_device;              // Logical device (needed for cleanup)
    VkCommandPool m_commandPool;    // The Vulkan command pool object
    
    // Command pool properties
    uint32_t m_queueFamilyIndex;    // Queue family this pool serves
    bool m_allowReset;              // Whether individual command buffers can be reset
    bool m_transient;               // Whether command buffers are short-lived
    
    // Tracking allocated command buffers for cleanup
    std::vector<VkCommandBuffer> m_allocatedBuffers;

    /**
     * Converts our Usage enum to Vulkan command buffer usage flags.
     * 
     * @param usage Our usage type
     * @return Corresponding Vulkan command buffer usage flags
     */
    VkCommandBufferUsageFlags getVulkanUsageFlags(Usage usage) const;

    /**
     * Converts our Level enum to Vulkan command buffer level.
     * 
     * @param level Our level type
     * @return Corresponding Vulkan command buffer level
     */
    VkCommandBufferLevel getVulkanLevel(Level level) const;
};

} // namespace VulkanGameEngine