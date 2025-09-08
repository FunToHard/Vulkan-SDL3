#include "../headers/VulkanBuffer.h"
#include "../headers/VulkanUtils.h"
#include <cstring>

namespace VulkanGameEngine {

VulkanBuffer::VulkanBuffer() 
    : m_device(VK_NULL_HANDLE)
    , m_buffer(VK_NULL_HANDLE)
    , m_memory(VK_NULL_HANDLE)
    , m_size(0)
    , m_usage(Usage::VERTEX_BUFFER)
    , m_memoryProperty(MemoryProperty::DEVICE_LOCAL)
    , m_mappedMemory(nullptr)
    , m_isCoherent(false) {
    
    VulkanUtils::logObjectCreation("VulkanBuffer", "Initialized");
}

VulkanBuffer::~VulkanBuffer() {
    cleanup();
}

// Move constructor
VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept 
    : m_device(other.m_device)
    , m_buffer(other.m_buffer)
    , m_memory(other.m_memory)
    , m_size(other.m_size)
    , m_usage(other.m_usage)
    , m_memoryProperty(other.m_memoryProperty)
    , m_mappedMemory(other.m_mappedMemory)
    , m_isCoherent(other.m_isCoherent) {
    
    // Reset the other object to prevent double cleanup
    other.m_device = VK_NULL_HANDLE;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
    other.m_mappedMemory = nullptr;
    other.m_isCoherent = false;
}

// Move assignment operator
VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        cleanup();
        
        // Move resources from other
        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_usage = other.m_usage;
        m_memoryProperty = other.m_memoryProperty;
        m_mappedMemory = other.m_mappedMemory;
        m_isCoherent = other.m_isCoherent;
        
        // Reset the other object to prevent double cleanup
        other.m_device = VK_NULL_HANDLE;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_size = 0;
        other.m_mappedMemory = nullptr;
        other.m_isCoherent = false;
    }
    return *this;
}

void VulkanBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, 
                         Usage usage, MemoryProperty memoryProperty) {
    
    // Store device handle for cleanup
    m_device = device;
    m_size = size;
    m_usage = usage;
    m_memoryProperty = memoryProperty;
    
    VulkanUtils::logObjectCreation("VulkanBuffer", 
        "Creating buffer of size " + std::to_string(size) + " bytes");
    
    // Step 1: Create the buffer object
    // The buffer is just a handle - it doesn't have memory allocated yet
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = getVulkanUsageFlags(usage);
    
    // Sharing mode determines how the buffer can be accessed by different queue families
    // EXCLUSIVE: Buffer is owned by one queue family at a time (better performance)
    // CONCURRENT: Buffer can be accessed by multiple queue families simultaneously
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer),
             "Failed to create buffer");
    
    // Step 2: Query memory requirements for the buffer
    // Vulkan tells us how much memory the buffer needs and what types are acceptable
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);
    
    std::cout << "Buffer memory requirements:\n"
              << "  Size: " << memRequirements.size << " bytes\n"
              << "  Alignment: " << memRequirements.alignment << " bytes\n"
              << "  Memory type bits: 0x" << std::hex << memRequirements.memoryTypeBits << std::dec << "\n";
    
    // Step 3: Find a suitable memory type
    VkMemoryPropertyFlags properties = getVulkanMemoryPropertyFlags(memoryProperty);
    uint32_t memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    
    // Check if the memory type is coherent (automatically synchronized between CPU/GPU)
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    m_isCoherent = (memProperties.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
    
    // Step 4: Allocate device memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    
    VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &m_memory),
             "Failed to allocate buffer memory");
    
    // Step 5: Bind the allocated memory to the buffer
    // The offset must be divisible by memRequirements.alignment
    VK_CHECK(vkBindBufferMemory(device, m_buffer, m_memory, 0),
             "Failed to bind buffer memory");
    
    std::cout << "Successfully created buffer with " 
              << (m_isCoherent ? "coherent" : "non-coherent") << " memory\n";
}

void VulkanBuffer::createWithData(VkDevice device, VkPhysicalDevice physicalDevice, 
                                 const void* data, VkDeviceSize size, Usage usage, MemoryProperty memoryProperty) {
    
    // First create the buffer
    create(device, physicalDevice, size, usage, memoryProperty);
    
    // Then upload the data
    if (data != nullptr && size > 0) {
        uploadData(data, size);
    }
}

void* VulkanBuffer::map(VkDeviceSize offset, VkDeviceSize size) {
    if (m_mappedMemory != nullptr) {
        std::cerr << "Warning: Buffer is already mapped\n";
        return m_mappedMemory;
    }
    
    // Memory mapping only works with HOST_VISIBLE memory
    VkMemoryPropertyFlags properties = getVulkanMemoryPropertyFlags(m_memoryProperty);
    if (!(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        throw std::runtime_error("Cannot map non-host-visible memory");
    }
    
    VK_CHECK(vkMapMemory(m_device, m_memory, offset, size, 0, &m_mappedMemory),
             "Failed to map buffer memory");
    
    return m_mappedMemory;
}

void VulkanBuffer::unmap() {
    if (m_mappedMemory != nullptr) {
        vkUnmapMemory(m_device, m_memory);
        m_mappedMemory = nullptr;
    }
}

void VulkanBuffer::uploadData(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (data == nullptr || size == 0) {
        return;
    }
    
    // Map memory, copy data, and unmap
    void* mappedData = map(offset, size);
    std::memcpy(mappedData, data, static_cast<size_t>(size));
    
    // For non-coherent memory, we need to flush to make CPU writes visible to GPU
    if (!m_isCoherent) {
        flush(offset, size);
    }
    
    unmap();
}

void VulkanBuffer::copyTo(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                         VulkanBuffer& dstBuffer, VkDeviceSize size, 
                         VkDeviceSize srcOffset, VkDeviceSize dstOffset) {
    
    // Create a temporary command buffer for the copy operation
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);
    
    // Record the copy command
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    
    vkCmdCopyBuffer(commandBuffer, m_buffer, dstBuffer.getBuffer(), 1, &copyRegion);
    
    // Submit and wait for completion
    endSingleTimeCommands(device, commandPool, commandBuffer, graphicsQueue);
}

void VulkanBuffer::flush(VkDeviceSize offset, VkDeviceSize size) {
    if (m_isCoherent) {
        return; // Coherent memory doesn't need explicit flushing
    }
    
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    
    VK_CHECK(vkFlushMappedMemoryRanges(m_device, 1, &mappedRange),
             "Failed to flush mapped memory range");
}

void VulkanBuffer::invalidate(VkDeviceSize offset, VkDeviceSize size) {
    if (m_isCoherent) {
        return; // Coherent memory doesn't need explicit invalidation
    }
    
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = m_memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    
    VK_CHECK(vkInvalidateMappedMemoryRanges(m_device, 1, &mappedRange),
             "Failed to invalidate mapped memory range");
}

void VulkanBuffer::cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        // Unmap memory if it's currently mapped
        unmap();
        
        // Destroy buffer and free memory
        if (m_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkBuffer");
        }
        
        if (m_memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkDeviceMemory");
        }
        
        m_device = VK_NULL_HANDLE;
    }
}

VkMemoryRequirements VulkanBuffer::getMemoryRequirements(VkDevice device, VkDeviceSize size, Usage usage) {
    // Create a temporary buffer to query requirements
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VulkanBuffer().getVulkanUsageFlags(usage);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer tempBuffer;
    VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &tempBuffer),
             "Failed to create temporary buffer for memory requirements query");
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, tempBuffer, &memRequirements);
    
    vkDestroyBuffer(device, tempBuffer, nullptr);
    
    return memRequirements;
}

VkBufferUsageFlags VulkanBuffer::getVulkanUsageFlags(Usage usage) const {
    switch (usage) {
        case Usage::VERTEX_BUFFER:
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        case Usage::INDEX_BUFFER:
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        case Usage::UNIFORM_BUFFER:
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        
        case Usage::STAGING_BUFFER:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        
        case Usage::STORAGE_BUFFER:
            return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        
        default:
            throw std::runtime_error("Unknown buffer usage type");
    }
}

VkMemoryPropertyFlags VulkanBuffer::getVulkanMemoryPropertyFlags(MemoryProperty property) const {
    switch (property) {
        case MemoryProperty::DEVICE_LOCAL:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        
        case MemoryProperty::HOST_VISIBLE:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        case MemoryProperty::HOST_COHERENT:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        case MemoryProperty::STAGING:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
        default:
            throw std::runtime_error("Unknown memory property type");
    }
}

uint32_t VulkanBuffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                                     VkMemoryPropertyFlags properties) const {
    
    // Query available memory types on this device
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    std::cout << "Searching for memory type with properties: 0x" << std::hex << properties << std::dec << "\n";
    std::cout << "Available memory types: " << memProperties.memoryTypeCount << "\n";
    
    // Find a memory type that satisfies both the buffer requirements and our property requirements
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        // Check if this memory type is supported by the buffer (bit i is set in typeFilter)
        bool typeSupported = (typeFilter & (1 << i)) != 0;
        
        // Check if this memory type has all the properties we need
        bool hasRequiredProperties = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;
        
        std::cout << "  Type " << i << ": supported=" << typeSupported 
                  << ", properties=0x" << std::hex << memProperties.memoryTypes[i].propertyFlags << std::dec
                  << ", heap=" << memProperties.memoryTypes[i].heapIndex << "\n";
        
        if (typeSupported && hasRequiredProperties) {
            std::cout << "Selected memory type " << i << "\n";
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type for buffer");
}

VkCommandBuffer VulkanBuffer::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) const {
    // Allocate a temporary command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer),
             "Failed to allocate single-time command buffer");
    
    // Begin recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // This command buffer will be submitted once
    
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo),
             "Failed to begin recording single-time command buffer");
    
    return commandBuffer;
}

void VulkanBuffer::endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, 
                                        VkCommandBuffer commandBuffer, VkQueue queue) const {
    
    // End recording
    VK_CHECK(vkEndCommandBuffer(commandBuffer),
             "Failed to end recording single-time command buffer");
    
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
    
    // Clean up the temporary command buffer
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Utility functions implementation
namespace BufferUtils {

VulkanBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                               VkCommandPool commandPool, VkQueue graphicsQueue,
                               const std::vector<Vertex>& vertices) {
    
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    // Create staging buffer (host-visible for data upload)
    VulkanBuffer stagingBuffer;
    stagingBuffer.createWithData(device, physicalDevice, vertices.data(), bufferSize,
                                VulkanBuffer::Usage::STAGING_BUFFER, 
                                VulkanBuffer::MemoryProperty::STAGING);
    
    // Create device-local vertex buffer (optimal for GPU access)
    VulkanBuffer vertexBuffer;
    vertexBuffer.create(device, physicalDevice, bufferSize,
                       VulkanBuffer::Usage::VERTEX_BUFFER,
                       VulkanBuffer::MemoryProperty::DEVICE_LOCAL);
    
    // Copy data from staging buffer to vertex buffer
    stagingBuffer.copyTo(device, commandPool, graphicsQueue, vertexBuffer, bufferSize);
    
    // Clean up staging buffer
    stagingBuffer.cleanup();
    
    VulkanUtils::logObjectCreation("VertexBuffer", 
        "Created with " + std::to_string(vertices.size()) + " vertices");
    
    return vertexBuffer;
}

VulkanBuffer createIndexBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                              VkCommandPool commandPool, VkQueue graphicsQueue,
                              const std::vector<uint32_t>& indices) {
    
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    // Create staging buffer
    VulkanBuffer stagingBuffer;
    stagingBuffer.createWithData(device, physicalDevice, indices.data(), bufferSize,
                                VulkanBuffer::Usage::STAGING_BUFFER,
                                VulkanBuffer::MemoryProperty::STAGING);
    
    // Create device-local index buffer
    VulkanBuffer indexBuffer;
    indexBuffer.create(device, physicalDevice, bufferSize,
                      VulkanBuffer::Usage::INDEX_BUFFER,
                      VulkanBuffer::MemoryProperty::DEVICE_LOCAL);
    
    // Copy data from staging buffer to index buffer
    stagingBuffer.copyTo(device, commandPool, graphicsQueue, indexBuffer, bufferSize);
    
    // Clean up staging buffer
    stagingBuffer.cleanup();
    
    VulkanUtils::logObjectCreation("IndexBuffer", 
        "Created with " + std::to_string(indices.size()) + " indices");
    
    return indexBuffer;
}

VulkanBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice) {
    VulkanBuffer uniformBuffer;
    uniformBuffer.create(device, physicalDevice, sizeof(UniformBufferObject),
                        VulkanBuffer::Usage::UNIFORM_BUFFER,
                        VulkanBuffer::MemoryProperty::HOST_VISIBLE);
    
    VulkanUtils::logObjectCreation("UniformBuffer", "Created for MVP matrices");
    
    return uniformBuffer;
}

VulkanBuffer createStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                VkDeviceSize size) {
    VulkanBuffer stagingBuffer;
    stagingBuffer.create(device, physicalDevice, size,
                        VulkanBuffer::Usage::STAGING_BUFFER,
                        VulkanBuffer::MemoryProperty::STAGING);
    
    VulkanUtils::logObjectCreation("StagingBuffer", 
        "Created with size " + std::to_string(size) + " bytes");
    
    return stagingBuffer;
}

} // namespace BufferUtils

/**
 * Additional buffer management utilities for vertex and uniform buffer operations
 */
namespace VertexBufferManager {
    
    /**
     * Creates multiple vertex buffers for different vertex attributes.
     * This is useful when you want to store different vertex attributes
     * in separate buffers (interleaved vs. separate attribute buffers).
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param commandPool Command pool for data transfer
     * @param graphicsQueue Queue for command submission
     * @param positions Vector of vertex positions
     * @param colors Vector of vertex colors
     * @param texCoords Vector of texture coordinates
     * @return Vector of VulkanBuffer objects for each attribute
     */
    std::vector<VulkanBuffer> createSeparateAttributeBuffers(
        VkDevice device, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue graphicsQueue,
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec3>& colors,
        const std::vector<glm::vec2>& texCoords) {
        
        std::vector<VulkanBuffer> attributeBuffers;
        attributeBuffers.reserve(3);
        
        // Create position buffer
        if (!positions.empty()) {
            VkDeviceSize positionSize = sizeof(glm::vec3) * positions.size();
            VulkanBuffer stagingBuffer;
            stagingBuffer.createWithData(device, physicalDevice, positions.data(), positionSize,
                                        VulkanBuffer::Usage::STAGING_BUFFER,
                                        VulkanBuffer::MemoryProperty::STAGING);
            
            VulkanBuffer positionBuffer;
            positionBuffer.create(device, physicalDevice, positionSize,
                                 VulkanBuffer::Usage::VERTEX_BUFFER,
                                 VulkanBuffer::MemoryProperty::DEVICE_LOCAL);
            
            stagingBuffer.copyTo(device, commandPool, graphicsQueue, positionBuffer, positionSize);
            stagingBuffer.cleanup();
            
            attributeBuffers.push_back(std::move(positionBuffer));
        }
        
        // Create color buffer
        if (!colors.empty()) {
            VkDeviceSize colorSize = sizeof(glm::vec3) * colors.size();
            VulkanBuffer stagingBuffer;
            stagingBuffer.createWithData(device, physicalDevice, colors.data(), colorSize,
                                        VulkanBuffer::Usage::STAGING_BUFFER,
                                        VulkanBuffer::MemoryProperty::STAGING);
            
            VulkanBuffer colorBuffer;
            colorBuffer.create(device, physicalDevice, colorSize,
                              VulkanBuffer::Usage::VERTEX_BUFFER,
                              VulkanBuffer::MemoryProperty::DEVICE_LOCAL);
            
            stagingBuffer.copyTo(device, commandPool, graphicsQueue, colorBuffer, colorSize);
            stagingBuffer.cleanup();
            
            attributeBuffers.push_back(std::move(colorBuffer));
        }
        
        // Create texture coordinate buffer
        if (!texCoords.empty()) {
            VkDeviceSize texCoordSize = sizeof(glm::vec2) * texCoords.size();
            VulkanBuffer stagingBuffer;
            stagingBuffer.createWithData(device, physicalDevice, texCoords.data(), texCoordSize,
                                        VulkanBuffer::Usage::STAGING_BUFFER,
                                        VulkanBuffer::MemoryProperty::STAGING);
            
            VulkanBuffer texCoordBuffer;
            texCoordBuffer.create(device, physicalDevice, texCoordSize,
                                 VulkanBuffer::Usage::VERTEX_BUFFER,
                                 VulkanBuffer::MemoryProperty::DEVICE_LOCAL);
            
            stagingBuffer.copyTo(device, commandPool, graphicsQueue, texCoordBuffer, texCoordSize);
            stagingBuffer.cleanup();
            
            attributeBuffers.push_back(std::move(texCoordBuffer));
        }
        
        VulkanUtils::logObjectCreation("SeparateAttributeBuffers", 
            "Created " + std::to_string(attributeBuffers.size()) + " attribute buffers");
        
        return attributeBuffers;
    }
    
    /**
     * Updates vertex buffer data efficiently.
     * This function handles both small updates (direct memory mapping) and
     * large updates (staging buffer for device-local memory).
     * 
     * @param vertexBuffer The vertex buffer to update
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param commandPool Command pool for staging operations
     * @param graphicsQueue Queue for command submission
     * @param newVertices New vertex data
     * @param offset Offset in the buffer to start updating
     */
    void updateVertexBuffer(VulkanBuffer& vertexBuffer,
                           VkDevice device, VkPhysicalDevice physicalDevice,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           const std::vector<Vertex>& newVertices,
                           VkDeviceSize offset) {
        
        VkDeviceSize dataSize = sizeof(Vertex) * newVertices.size();
        
        // Check if the update fits in the buffer
        if (offset + dataSize > vertexBuffer.getSize()) {
            throw std::runtime_error("Vertex buffer update exceeds buffer size");
        }
        
        // For device-local buffers, use staging buffer for updates
        if (vertexBuffer.getMemoryProperty() == VulkanBuffer::MemoryProperty::DEVICE_LOCAL) {
            VulkanBuffer stagingBuffer;
            stagingBuffer.createWithData(device, physicalDevice, newVertices.data(), dataSize,
                                        VulkanBuffer::Usage::STAGING_BUFFER,
                                        VulkanBuffer::MemoryProperty::STAGING);
            
            stagingBuffer.copyTo(device, commandPool, graphicsQueue, vertexBuffer, dataSize, 0, offset);
            stagingBuffer.cleanup();
        } else {
            // For host-visible buffers, direct memory mapping is more efficient
            vertexBuffer.uploadData(newVertices.data(), dataSize, offset);
        }
        
        VulkanUtils::logObjectCreation("VertexBufferUpdate", 
            "Updated " + std::to_string(newVertices.size()) + " vertices");
    }
}

/**
 * Uniform buffer management utilities for MVP matrices and other uniform data
 */
namespace UniformBufferManager {
    
    /**
     * Creates multiple uniform buffers for frames in flight.
     * This allows updating uniform data for one frame while another frame
     * is being rendered, improving performance.
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param framesInFlight Number of frames that can be processed simultaneously
     * @return Vector of uniform buffers, one for each frame in flight
     */
    std::vector<VulkanBuffer> createUniformBuffersForFramesInFlight(
        VkDevice device, VkPhysicalDevice physicalDevice, uint32_t framesInFlight) {
        
        std::vector<VulkanBuffer> uniformBuffers;
        uniformBuffers.reserve(framesInFlight);
        
        for (uint32_t i = 0; i < framesInFlight; i++) {
            VulkanBuffer uniformBuffer;
            uniformBuffer.create(device, physicalDevice, sizeof(UniformBufferObject),
                                VulkanBuffer::Usage::UNIFORM_BUFFER,
                                VulkanBuffer::MemoryProperty::HOST_VISIBLE);
            uniformBuffers.push_back(std::move(uniformBuffer));
        }
        
        VulkanUtils::logObjectCreation("UniformBuffersForFramesInFlight", 
            "Created " + std::to_string(framesInFlight) + " uniform buffers");
        
        return uniformBuffers;
    }
    
    /**
     * Updates uniform buffer with MVP matrices.
     * This function calculates and uploads transformation matrices
     * for 3D rendering.
     * 
     * @param uniformBuffer The uniform buffer to update
     * @param model Model transformation matrix
     * @param view View (camera) transformation matrix
     * @param projection Projection transformation matrix
     */
    void updateUniformBuffer(VulkanBuffer& uniformBuffer,
                            const glm::mat4& model,
                            const glm::mat4& view,
                            const glm::mat4& projection) {
        
        UniformBufferObject ubo{};
        ubo.model = model;
        ubo.view = view;
        ubo.projection = projection;
        
        uniformBuffer.uploadData(&ubo, sizeof(ubo));
    }
    
    /**
     * Creates a dynamic uniform buffer that can hold multiple objects' data.
     * This is useful for rendering multiple objects with different transformations
     * in a single draw call.
     * 
     * @param device Logical device
     * @param physicalDevice Physical device
     * @param objectCount Number of objects to support
     * @return VulkanBuffer configured for dynamic uniform data
     */
    VulkanBuffer createDynamicUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                           uint32_t objectCount) {
        
        // Query minimum uniform buffer offset alignment
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        
        size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
        size_t dynamicAlignment = sizeof(UniformBufferObject);
        
        // Align the UBO size to the minimum required alignment
        if (minUboAlignment > 0) {
            dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }
        
        VkDeviceSize bufferSize = objectCount * dynamicAlignment;
        
        VulkanBuffer dynamicUniformBuffer;
        dynamicUniformBuffer.create(device, physicalDevice, bufferSize,
                                   VulkanBuffer::Usage::UNIFORM_BUFFER,
                                   VulkanBuffer::MemoryProperty::HOST_VISIBLE);
        
        VulkanUtils::logObjectCreation("DynamicUniformBuffer", 
            "Created for " + std::to_string(objectCount) + " objects, alignment=" + std::to_string(dynamicAlignment));
        
        return dynamicUniformBuffer;
    }
    
    /**
     * Updates a specific object's data in a dynamic uniform buffer.
     * 
     * @param dynamicUniformBuffer The dynamic uniform buffer
     * @param physicalDevice Physical device for alignment queries
     * @param objectIndex Index of the object to update
     * @param model Model transformation matrix
     * @param view View transformation matrix
     * @param projection Projection transformation matrix
     */
    void updateDynamicUniformBuffer(VulkanBuffer& dynamicUniformBuffer,
                                   VkPhysicalDevice physicalDevice,
                                   uint32_t objectIndex,
                                   const glm::mat4& model,
                                   const glm::mat4& view,
                                   const glm::mat4& projection) {
        
        // Calculate aligned offset for this object
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        
        size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
        size_t dynamicAlignment = sizeof(UniformBufferObject);
        
        if (minUboAlignment > 0) {
            dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }
        
        VkDeviceSize offset = objectIndex * dynamicAlignment;
        
        UniformBufferObject ubo{};
        ubo.model = model;
        ubo.view = view;
        ubo.projection = projection;
        
        dynamicUniformBuffer.uploadData(&ubo, sizeof(ubo), offset);
    }
}

} // namespace VulkanGameEngine