#include "../headers/VulkanCommandPool.h"
#include "../headers/VulkanUtils.h"

namespace VulkanGameEngine {

VulkanCommandPool::VulkanCommandPool()
    : m_device(VK_NULL_HANDLE)
    , m_commandPool(VK_NULL_HANDLE)
    , m_queueFamilyIndex(0)
    , m_allowReset(true)
    , m_transient(false) {
    
    VulkanUtils::logObjectCreation("VulkanCommandPool", "Initialized");
}

VulkanCommandPool::~VulkanCommandPool() {
    cleanup();
}

void VulkanCommandPool::create(VkDevice device, uint32_t queueFamilyIndex, 
                              bool allowReset, bool transient) {
    
    // Store device handle and properties for later use
    m_device = device;
    m_queueFamilyIndex = queueFamilyIndex;
    m_allowReset = allowReset;
    m_transient = transient;
    
    VulkanUtils::logObjectCreation("VulkanCommandPool", 
        "Creating for queue family " + std::to_string(queueFamilyIndex));
    
    // Configure command pool creation flags
    VkCommandPoolCreateFlags flags = 0;
    
    // RESET_COMMAND_BUFFER_BIT allows individual command buffers to be reset
    // Without this flag, command buffers can only be reset by resetting the entire pool
    if (allowReset) {
        flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        std::cout << "  - Enabling individual command buffer reset\n";
    }
    
    // TRANSIENT_BIT hints that command buffers will be short-lived
    // This can help the driver optimize memory allocation
    if (transient) {
        flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        std::cout << "  - Enabling transient allocation optimization\n";
    }
    
    // Create the command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    
    VK_CHECK(vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool),
             "Failed to create command pool");
    
    std::cout << "Successfully created command pool for queue family " << queueFamilyIndex << "\n";
}

std::vector<VkCommandBuffer> VulkanCommandPool::allocateCommandBuffers(uint32_t count, Level level) {
    if (m_commandPool == VK_NULL_HANDLE) {
        throw std::runtime_error("Cannot allocate command buffers: command pool not created");
    }
    
    VulkanUtils::logObjectCreation("CommandBuffers", 
        "Allocating " + std::to_string(count) + " command buffers");
    
    // Configure command buffer allocation
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = getVulkanLevel(level);
    allocInfo.commandBufferCount = count;
    
    // Allocate the command buffers
    std::vector<VkCommandBuffer> commandBuffers(count);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, commandBuffers.data()),
             "Failed to allocate command buffers");
    
    // Track allocated buffers for cleanup
    m_allocatedBuffers.insert(m_allocatedBuffers.end(), commandBuffers.begin(), commandBuffers.end());
    
    std::cout << "Successfully allocated " << count << " " 
              << (level == Level::PRIMARY ? "primary" : "secondary") 
              << " command buffers\n";
    
    return commandBuffers;
}

VkCommandBuffer VulkanCommandPool::allocateCommandBuffer(Level level) {
    auto buffers = allocateCommandBuffers(1, level);
    return buffers[0];
}

void VulkanCommandPool::freeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers) {
    if (m_commandPool == VK_NULL_HANDLE || commandBuffers.empty()) {
        return;
    }
    
    VulkanUtils::logObjectDestruction("CommandBuffers", 
        "Freeing " + std::to_string(commandBuffers.size()) + " command buffers");
    
    // Free the command buffers
    vkFreeCommandBuffers(m_device, m_commandPool, 
                        static_cast<uint32_t>(commandBuffers.size()), 
                        commandBuffers.data());
    
    // Remove from tracking list
    for (const auto& buffer : commandBuffers) {
        auto it = std::find(m_allocatedBuffers.begin(), m_allocatedBuffers.end(), buffer);
        if (it != m_allocatedBuffers.end()) {
            m_allocatedBuffers.erase(it);
        }
    }
}

void VulkanCommandPool::freeCommandBuffer(VkCommandBuffer commandBuffer) {
    freeCommandBuffers({commandBuffer});
}

void VulkanCommandPool::reset(bool releaseResources) {
    if (m_commandPool == VK_NULL_HANDLE) {
        return;
    }
    
    VulkanUtils::logObjectCreation("CommandPool", "Resetting command pool");
    
    VkCommandPoolResetFlags flags = 0;
    if (releaseResources) {
        flags |= VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;
        std::cout << "  - Releasing resources back to system\n";
    }
    
    VK_CHECK(vkResetCommandPool(m_device, m_commandPool, flags),
             "Failed to reset command pool");
    
    std::cout << "Successfully reset command pool\n";
}

void VulkanCommandPool::beginCommandBuffer(VkCommandBuffer commandBuffer, Usage usage,
                                          const VkCommandBufferInheritanceInfo* inheritanceInfo) {
    
    // Configure command buffer begin info
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = getVulkanUsageFlags(usage);
    beginInfo.pInheritanceInfo = inheritanceInfo; // Only used for secondary command buffers
    
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo),
             "Failed to begin recording command buffer");
}

void VulkanCommandPool::endCommandBuffer(VkCommandBuffer commandBuffer) {
    VK_CHECK(vkEndCommandBuffer(commandBuffer),
             "Failed to end recording command buffer");
}

VkCommandBuffer VulkanCommandPool::beginSingleTimeCommands() {
    // Allocate a temporary command buffer
    VkCommandBuffer commandBuffer = allocateCommandBuffer(Level::PRIMARY);
    
    // Begin recording with single-use optimization
    beginCommandBuffer(commandBuffer, Usage::SINGLE_USE);
    
    return commandBuffer;
}

void VulkanCommandPool::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue) {
    // End recording
    endCommandBuffer(commandBuffer);
    
    // Submit to queue
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE),
             "Failed to submit single-time command buffer");
    
    // Wait for completion
    VK_CHECK(vkQueueWaitIdle(queue),
             "Failed to wait for queue idle after single-time command");
    
    // Free the temporary command buffer
    freeCommandBuffer(commandBuffer);
}

void VulkanCommandPool::beginRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass,
                                       VkFramebuffer framebuffer, VkRect2D renderArea,
                                       const std::vector<VkClearValue>& clearValues) {
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea = renderArea;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    // Begin the render pass
    // VK_SUBPASS_CONTENTS_INLINE means the render pass commands will be embedded
    // in the primary command buffer (as opposed to being in secondary command buffers)
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandPool::endRenderPass(VkCommandBuffer commandBuffer) {
    vkCmdEndRenderPass(commandBuffer);
}

void VulkanCommandPool::bindPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline, 
                                     VkPipelineBindPoint bindPoint) {
    vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
}

void VulkanCommandPool::bindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding,
                                         const std::vector<VkBuffer>& buffers,
                                         const std::vector<VkDeviceSize>& offsets) {
    
    if (buffers.size() != offsets.size()) {
        throw std::runtime_error("Number of vertex buffers must match number of offsets");
    }
    
    vkCmdBindVertexBuffers(commandBuffer, firstBinding, 
                          static_cast<uint32_t>(buffers.size()),
                          buffers.data(), offsets.data());
}

void VulkanCommandPool::bindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer,
                                       VkDeviceSize offset, VkIndexType indexType) {
    vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void VulkanCommandPool::bindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                                          uint32_t firstSet, const std::vector<VkDescriptorSet>& descriptorSets,
                                          const std::vector<uint32_t>& dynamicOffsets) {
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                           firstSet, static_cast<uint32_t>(descriptorSets.size()),
                           descriptorSets.data(), static_cast<uint32_t>(dynamicOffsets.size()),
                           dynamicOffsets.empty() ? nullptr : dynamicOffsets.data());
}

void VulkanCommandPool::draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                            uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandPool::drawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
                                   uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandPool::setViewport(VkCommandBuffer commandBuffer, float x, float y, float width, float height,
                                   float minDepth, float maxDepth) {
    VkViewport viewport{};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
}

void VulkanCommandPool::setScissor(VkCommandBuffer commandBuffer, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    VkRect2D scissor{};
    scissor.offset = {x, y};
    scissor.extent = {width, height};
    
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void VulkanCommandPool::copyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                  VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void VulkanCommandPool::pipelineBarrier(VkCommandBuffer commandBuffer,
                                       VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                       VkDependencyFlags dependencyFlags,
                                       const std::vector<VkMemoryBarrier>& memoryBarriers,
                                       const std::vector<VkBufferMemoryBarrier>& bufferBarriers,
                                       const std::vector<VkImageMemoryBarrier>& imageBarriers) {
    
    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                        static_cast<uint32_t>(memoryBarriers.size()),
                        memoryBarriers.empty() ? nullptr : memoryBarriers.data(),
                        static_cast<uint32_t>(bufferBarriers.size()),
                        bufferBarriers.empty() ? nullptr : bufferBarriers.data(),
                        static_cast<uint32_t>(imageBarriers.size()),
                        imageBarriers.empty() ? nullptr : imageBarriers.data());
}

void VulkanCommandPool::pushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                                     VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* data) {
    vkCmdPushConstants(commandBuffer, pipelineLayout, stageFlags, offset, size, data);
}

void VulkanCommandPool::recordFrameCommands(VkCommandBuffer commandBuffer,
                                           VkRenderPass renderPass, VkFramebuffer framebuffer,
                                           VkPipeline pipeline, VkPipelineLayout pipelineLayout,
                                           VkRect2D renderArea, const std::vector<VkClearValue>& clearValues,
                                           const std::vector<VkBuffer>& vertexBuffers,
                                           const std::vector<VkDeviceSize>& vertexOffsets,
                                           VkBuffer indexBuffer, VkDeviceSize indexOffset,
                                           const std::vector<VkDescriptorSet>& descriptorSets,
                                           uint32_t vertexCount, uint32_t indexCount,
                                           uint32_t instanceCount) {
    
    // Begin render pass
    beginRenderPass(commandBuffer, renderPass, framebuffer, renderArea, clearValues);
    
    // Bind graphics pipeline
    bindPipeline(commandBuffer, pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS);
    
    // Set viewport and scissor to match render area
    setViewport(commandBuffer, 
               static_cast<float>(renderArea.offset.x), 
               static_cast<float>(renderArea.offset.y),
               static_cast<float>(renderArea.extent.width), 
               static_cast<float>(renderArea.extent.height));
    
    setScissor(commandBuffer, renderArea.offset.x, renderArea.offset.y,
              renderArea.extent.width, renderArea.extent.height);
    
    // Bind vertex buffers if provided
    if (!vertexBuffers.empty()) {
        bindVertexBuffers(commandBuffer, 0, vertexBuffers, vertexOffsets);
    }
    
    // Bind index buffer if provided
    if (indexBuffer != VK_NULL_HANDLE) {
        bindIndexBuffer(commandBuffer, indexBuffer, indexOffset);
    }
    
    // Bind descriptor sets if provided
    if (!descriptorSets.empty()) {
        bindDescriptorSets(commandBuffer, pipelineLayout, 0, descriptorSets);
    }
    
    // Draw the geometry
    if (indexBuffer != VK_NULL_HANDLE && indexCount > 0) {
        // Indexed drawing
        drawIndexed(commandBuffer, indexCount, instanceCount);
    } else if (vertexCount > 0) {
        // Non-indexed drawing
        draw(commandBuffer, vertexCount, instanceCount);
    }
    
    // End render pass
    endRenderPass(commandBuffer);
}

void VulkanCommandPool::cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        // Free all tracked command buffers
        if (!m_allocatedBuffers.empty()) {
            VulkanUtils::logObjectDestruction("CommandBuffers", 
                "Freeing " + std::to_string(m_allocatedBuffers.size()) + " remaining command buffers");
            
            vkFreeCommandBuffers(m_device, m_commandPool, 
                               static_cast<uint32_t>(m_allocatedBuffers.size()),
                               m_allocatedBuffers.data());
            m_allocatedBuffers.clear();
        }
        
        // Destroy the command pool
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkCommandPool");
        }
        
        m_device = VK_NULL_HANDLE;
    }
}

VkCommandBufferUsageFlags VulkanCommandPool::getVulkanUsageFlags(Usage usage) const {
    switch (usage) {
        case Usage::SINGLE_USE:
            return VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        case Usage::REUSABLE:
            return 0; // No special flags for reusable command buffers
        
        case Usage::SIMULTANEOUS_USE:
            return VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        
        default:
            throw std::runtime_error("Unknown command buffer usage type");
    }
}

VkCommandBufferLevel VulkanCommandPool::getVulkanLevel(Level level) const {
    switch (level) {
        case Level::PRIMARY:
            return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        
        case Level::SECONDARY:
            return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        
        default:
            throw std::runtime_error("Unknown command buffer level");
    }
}

} // namespace VulkanGameEngine