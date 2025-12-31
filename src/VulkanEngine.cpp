#include "../headers/VulkanEngine.h"
#include "../headers/VulkanUtils.h"
#include "../headers/VulkanBuffer.h"
#include "../headers/Logger.h"
#include <chrono>

namespace VulkanGameEngine {

VulkanEngine::VulkanEngine()
    : m_surface(VK_NULL_HANDLE)
    , m_initState(InitializationState::NOT_INITIALIZED)
    , m_window(nullptr)
    , m_windowWidth(0)
    , m_windowHeight(0)
    , m_currentFrame(0)
    , m_frameCount(0)
    , m_lastFrameTime(0.0f)
    , m_time(0.0f)
    , m_modelMatrix(1.0f)
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_useMainCharacter(false)
    , m_descriptorPool(VK_NULL_HANDLE)
    , m_depthImage(VK_NULL_HANDLE)
    , m_depthImageMemory(VK_NULL_HANDLE)
    , m_depthImageView(VK_NULL_HANDLE)
    , m_cameraPosition(10.0f, 5.0f, 10.0f)
    , m_cameraTarget(0.0f, 0.0f, 0.0f)
    , m_cameraSpeed(5.0f) {
    
    VulkanUtils::logObjectCreation("VulkanEngine", "Initialized");
}

VulkanEngine::~VulkanEngine() {
    cleanup();
}

void VulkanEngine::initialize(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight) {
    VulkanUtils::logObjectCreation("VulkanEngine", "Beginning initialization sequence");
    
    m_window = window;
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    
    try {
        // Step 1: Create Vulkan instance
        logInitializationState(InitializationState::INSTANCE_CREATED, "Creating Vulkan instance");
        
        // Get required extensions from SDL
        uint32_t extensionCount = 0;
        const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        if (!extensions) {
            throw std::runtime_error("Failed to get Vulkan extensions from SDL: " + std::string(SDL_GetError()));
        }
        
        std::vector<const char*> requiredExtensions(extensions, extensions + extensionCount);
        m_instance.create(requiredExtensions);
        m_initState = InitializationState::INSTANCE_CREATED;
        
        // Step 2: Create window surface
        logInitializationState(InitializationState::SURFACE_CREATED, "Creating window surface");
        createSurface(window);
        m_initState = InitializationState::SURFACE_CREATED;
        
        // Step 3: Create device (physical and logical)
        logInitializationState(InitializationState::DEVICE_CREATED, "Creating Vulkan device");
        m_device.create(m_instance.getInstance(), m_surface);
        m_initState = InitializationState::DEVICE_CREATED;
        
        // Step 4: Create swapchain
        logInitializationState(InitializationState::SWAPCHAIN_CREATED, "Creating swapchain");
        m_swapchain.create(m_device, m_surface, windowWidth, windowHeight);
        m_initState = InitializationState::SWAPCHAIN_CREATED;
        
        // Step 5: Create render pass
        logInitializationState(InitializationState::RENDER_PASS_CREATED, "Creating render pass");
        // Use appropriate depth format - you may need to query this from device
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // Common depth format
        m_renderPass.create(m_device.getLogicalDevice(), m_swapchain.getImageFormat(), depthFormat);
        m_initState = InitializationState::RENDER_PASS_CREATED;
        
        // Step 5.5: Create depth buffer and framebuffers
        logInitializationState(InitializationState::RENDER_PASS_CREATED, "Creating depth buffer and framebuffers");
        createDepthBuffer();
        m_renderPass.createFramebuffers(m_swapchain.getImageViews(), m_depthImageView, m_swapchain.getExtent());
        
        // Step 6: Create graphics pipeline
        logInitializationState(InitializationState::PIPELINE_CREATED, "Creating graphics pipeline");
        m_pipeline.createGraphicsPipeline(m_device.getLogicalDevice(), m_renderPass.getRenderPass(), 
                         "shaders/vertex.vert.spv", "shaders/fragment.frag.spv", m_swapchain.getExtent());
        m_initState = InitializationState::PIPELINE_CREATED;
        
        // Step 6.5: Create descriptor sets using the pipeline's layout
        logInitializationState(InitializationState::DESCRIPTORS_CREATED, "Creating descriptor sets");
        createDescriptorSets();
        m_initState = InitializationState::DESCRIPTORS_CREATED;
        
        // Step 7: Create command pool and command buffers (MOVED UP - needed for buffer creation)
        logInitializationState(InitializationState::COMMAND_POOL_CREATED, "Creating command pool and buffers");
        m_commandPool.create(m_device.getLogicalDevice(), 
                            m_device.getQueueFamilyIndices().graphicsFamily.value());
        createCommandBuffers();
        m_initState = InitializationState::COMMAND_POOL_CREATED;
        
        // Step 8: Create buffers (MOVED DOWN - now command pool is available)
        logInitializationState(InitializationState::BUFFERS_CREATED, "Creating vertex and uniform buffers");
        createBuffers();
        createUniformBuffers();
        updateDescriptorSets(); // Update descriptor sets after uniform buffers are created
        m_initState = InitializationState::BUFFERS_CREATED;
        
        // Step 9: Create synchronization objects
        logInitializationState(InitializationState::SYNCHRONIZATION_CREATED, "Creating synchronization objects");
        m_synchronization.create(m_device.getLogicalDevice(), MAX_FRAMES_IN_FLIGHT);
        m_initState = InitializationState::SYNCHRONIZATION_CREATED;
        
        // Step 10: Load main character
        logInitializationState(InitializationState::CHARACTER_LOADED, "Loading main character model");
        loadMainCharacter();
        m_initState = InitializationState::CHARACTER_LOADED;
        
        // Step 11: Setup initial scene
        setupScene();
        m_initState = InitializationState::FULLY_INITIALIZED;
        
        VulkanUtils::logObjectCreation("VulkanEngine", "Initialization completed successfully");
        LOG_INFO("Vulkan engine ready for rendering!", "Engine");
        LOG_INFO("  - Window size: " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight), "Engine");
        LOG_INFO("  - Max frames in flight: " + std::to_string(MAX_FRAMES_IN_FLIGHT), "Engine");
        LOG_INFO("  - Swapchain images: " + std::to_string(m_swapchain.getImageViews().size()), "Engine");
        
        if (m_useMainCharacter) {
            uint32_t vertexCount, triangleCount;
            m_mainCharacter.getModelStats(vertexCount, triangleCount);
            LOG_INFO("  - Main character loaded: " + std::to_string(vertexCount) + " vertices, " + 
                    std::to_string(triangleCount) + " triangles", "Engine");
        } else {
            LOG_INFO("  - Using fallback cube geometry", "Engine");
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Vulkan engine initialization failed at state " + 
                  std::to_string(static_cast<int>(m_initState)) + ": " + e.what(), "Engine");
        cleanup();
        throw;
    }
}

void VulkanEngine::render() {
    if (m_initState != InitializationState::FULLY_INITIALIZED) {
        throw std::runtime_error("Cannot render: engine not fully initialized");
    }
    
    // Skip rendering if window is minimized
    if (m_windowWidth == 0 || m_windowHeight == 0) {
        return;
    }
    
    auto frameStart = std::chrono::high_resolution_clock::now();
    
    try {
        // Wait for the previous frame to complete
        if (!m_synchronization.waitForFrame(m_currentFrame, UINT64_MAX)) { // Infinite timeout
            LOG_WARN("Failed to wait for frame " + std::to_string(m_currentFrame), "Engine");
            return;
        }
        
        // Acquire next image from swapchain
        uint32_t imageIndex;
        VkResult result = m_synchronization.acquireNextImage(
            m_device.getLogicalDevice(),
            m_swapchain.getSwapchain(),
            UINT64_MAX,
            m_synchronization.getImageAvailableSemaphore(m_currentFrame),
            VK_NULL_HANDLE,
            &imageIndex
        );
        
        // Handle swapchain recreation if needed
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapchain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swapchain image: " + 
                                   VulkanUtils::vulkanResultToString(result));
        }
        
        // Reset fence for this frame
        m_synchronization.resetFrameFence(m_currentFrame);
        
        // Update scene data for this frame
        updateScene(m_lastFrameTime);
        
        // Update uniform buffer for this frame
        updateUniformBuffer(m_currentFrame);
        
        // Record command buffer
        VkCommandBuffer commandBuffer = m_commandBuffers[m_currentFrame];
        VK_CHECK(vkResetCommandBuffer(commandBuffer, 0), "Failed to reset command buffer");
        recordCommandBuffer(commandBuffer, imageIndex);
        
        // Submit command buffer
        std::vector<VkSemaphore> waitSemaphores = {m_synchronization.getImageAvailableSemaphore(m_currentFrame)};
        std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        std::vector<VkSemaphore> signalSemaphores = {m_synchronization.getRenderFinishedSemaphore(m_currentFrame)};
        
        m_synchronization.submitCommandBuffers(
            m_device.getGraphicsQueue(),
            {commandBuffer},
            waitSemaphores,
            waitStages,
            signalSemaphores,
            m_synchronization.getInFlightFence(m_currentFrame)
        );
        
        // Present the image
        result = m_synchronization.presentImage(
            m_device.getPresentQueue(),
            m_swapchain.getSwapchain(),
            imageIndex,
            signalSemaphores
        );
        
        // Handle swapchain recreation if needed
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapchain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swapchain image: " + 
                                   VulkanUtils::vulkanResultToString(result));
        }
        
        // Move to next frame
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        m_frameCount++;
        
        // Calculate frame time
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float>(frameEnd - frameStart);
        m_lastFrameTime = frameDuration.count();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error during rendering: " + std::string(e.what()), "Engine");
        // Try to recover by recreating the swapchain
        try {
            recreateSwapchain();
        } catch (const std::exception& recreateError) {
            LOG_ERROR("Failed to recover from render error: " + std::string(recreateError.what()), "Engine");
            throw; // Re-throw if we can't recover
        }
    }
}

void VulkanEngine::handleResize(uint32_t newWidth, uint32_t newHeight) {
    if (newWidth == 0 || newHeight == 0) {
        return; // Skip zero-sized windows
    }
    
    VulkanUtils::logObjectCreation("VulkanEngine", 
        "Handling resize to " + std::to_string(newWidth) + "x" + std::to_string(newHeight));
    
    m_windowWidth = newWidth;
    m_windowHeight = newHeight;
    
    // Wait for device to be idle before recreating resources
    waitIdle();
    
    // Recreate swapchain and dependent resources
    recreateSwapchain();
    
    // Update projection matrix for new aspect ratio
    setupScene();
}

void VulkanEngine::updateScene(float deltaTime) {
    m_time += deltaTime;
    
    if (m_useMainCharacter) {
        // Position the main character at the origin with optional slow rotation for visibility
        glm::vec3 position(0.0f, 0.0f, 0.0f);  // Character at world origin
        glm::vec3 rotation(0.0f, m_time * glm::radians(15.0f), 0.0f); // Slow rotation around Y axis
        float scale = 1.0f;
        
        m_mainCharacter.setTransform(position, rotation, scale);
        m_modelMatrix = m_mainCharacter.getTransformMatrix();
    } else {
        // Position the fallback cube at origin as well
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), m_time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        
        m_modelMatrix = translation * rotationY;
    }
    
    /**
     * Camera Setup:
     * Use the dynamic camera position and target variables for camera control.
     */
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);     // Y is up
    
    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraTarget, upVector);
}

void VulkanEngine::moveCamera(float forward, float right, float deltaTime) {
    // Log camera movement for debugging
    if (forward != 0.0f || right != 0.0f) {
        std::cout << "[DEBUG][Camera] Movement input - Forward: " << forward 
                  << ", Right: " << right << ", DeltaTime: " << deltaTime << std::endl;
    }
    
    // Calculate camera forward and right vectors
    glm::vec3 forward_vector = glm::normalize(m_cameraTarget - m_cameraPosition);
    glm::vec3 right_vector = glm::normalize(glm::cross(forward_vector, glm::vec3(0.0f, 1.0f, 0.0f)));
    
    // Calculate movement based on input
    glm::vec3 movement = glm::vec3(0.0f);
    movement += forward_vector * forward * m_cameraSpeed * deltaTime;
    movement += right_vector * right * m_cameraSpeed * deltaTime;
    
    // Store old position for logging
    glm::vec3 oldPosition = m_cameraPosition;
    
    // Update camera position and target (move both to maintain look direction)
    m_cameraPosition += movement;
    m_cameraTarget += movement;
    
    // Log position change if moved
    if (forward != 0.0f || right != 0.0f) {
        std::cout << "[DEBUG][Camera] Position updated - From: (" 
                  << oldPosition.x << ", " << oldPosition.y << ", " << oldPosition.z 
                  << ") To: (" << m_cameraPosition.x << ", " << m_cameraPosition.y 
                  << ", " << m_cameraPosition.z << ")" << std::endl;
    }
    
    // Update the view matrix with new camera position
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraTarget, upVector);
}

void VulkanEngine::waitIdle() {
    if (m_device.getLogicalDevice() != VK_NULL_HANDLE) {
        VK_CHECK(vkDeviceWaitIdle(m_device.getLogicalDevice()), "Failed to wait for device idle");
    }
}

void VulkanEngine::cleanup() {
    VulkanUtils::logObjectDestruction("VulkanEngine", "Beginning cleanup sequence");
    
    // Wait for all operations to complete
    if (m_initState >= InitializationState::DEVICE_CREATED) {
        waitIdle();
    }
    
    // Clean up in reverse order of creation
    if (m_initState >= InitializationState::SYNCHRONIZATION_CREATED) {
        m_synchronization.cleanup();
    }
    
    if (m_initState >= InitializationState::CHARACTER_LOADED) {
        m_mainCharacter.cleanup();
    }
    
    if (m_initState >= InitializationState::BUFFERS_CREATED) {
        m_vertexBuffer.cleanup();
        m_indexBuffer.cleanup();
        for (auto& uniformBuffer : m_uniformBuffers) {
            uniformBuffer.cleanup();
        }
        m_uniformBuffers.clear();
    }
    
    if (m_initState >= InitializationState::COMMAND_POOL_CREATED) {
        m_commandPool.cleanup();
        m_commandBuffers.clear();
    }
    
    if (m_initState >= InitializationState::DESCRIPTORS_CREATED) {
        // Clean up descriptor pool (descriptor sets are automatically freed with pool)
        if (m_descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device.getLogicalDevice(), m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkDescriptorPool");
        }
        
        m_descriptorSets.clear();
    }
    
    if (m_initState >= InitializationState::PIPELINE_CREATED) {
        m_pipeline.cleanup();
    }
    
    if (m_initState >= InitializationState::RENDER_PASS_CREATED) {
        // Clean up depth buffer
        if (m_depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device.getLogicalDevice(), m_depthImageView, nullptr);
            m_depthImageView = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkImageView (depth)");
        }
        
        if (m_depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(m_device.getLogicalDevice(), m_depthImage, nullptr);
            m_depthImage = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkImage (depth)");
        }
        
        if (m_depthImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device.getLogicalDevice(), m_depthImageMemory, nullptr);
            m_depthImageMemory = VK_NULL_HANDLE;
            VulkanUtils::logObjectDestruction("VkDeviceMemory (depth)");
        }
        
        m_renderPass.cleanup();
    }
    
    if (m_initState >= InitializationState::SWAPCHAIN_CREATED) {
        m_swapchain.cleanup();
    }
    
    if (m_initState >= InitializationState::SURFACE_CREATED && m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance.getInstance(), m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
        VulkanUtils::logObjectDestruction("VkSurfaceKHR");
    }
    
    if (m_initState >= InitializationState::DEVICE_CREATED) {
        m_device.cleanup();
    }
    
    if (m_initState >= InitializationState::INSTANCE_CREATED) {
        m_instance.cleanup();
    }
    
    m_initState = InitializationState::NOT_INITIALIZED;
    m_window = nullptr;
    m_currentFrame = 0;
    m_frameCount = 0;
    
    VulkanUtils::logObjectDestruction("VulkanEngine", "Cleanup completed");
}

void VulkanEngine::createSurface(SDL_Window* window) {
    if (!SDL_Vulkan_CreateSurface(window, m_instance.getInstance(), nullptr, &m_surface)) {
        throw std::runtime_error("Failed to create Vulkan surface: " + std::string(SDL_GetError()));
    }
    
    VulkanUtils::logObjectCreation("VkSurfaceKHR", "Created SDL Vulkan surface");
}

void VulkanEngine::createBuffers() {
    // Define a colorful 3D cube for fallback rendering
    std::vector<Vertex> vertices = {
        // Front face (red)
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},  // Bottom left
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},  // Bottom right
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},  // Top right
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},  // Top left
        
        // Back face (green)
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},  // Bottom left
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // Bottom right
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // Top right
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // Top left
        
        // Left face (blue)
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Bottom left
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom right
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // Top right
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // Top left
        
        // Right face (yellow)
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},  // Bottom left
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // Bottom right
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // Top right
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // Top left
        
        // Top face (magenta)
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Bottom left
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom right
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // Top right
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // Top left
        
        // Bottom face (cyan)
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},  // Bottom left
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},  // Bottom right
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},  // Top right
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}   // Top left
    };
    
    // Define indices for the cube (2 triangles per face, 6 faces)
    std::vector<uint32_t> indices = {
        // Front face
        0, 1, 2,    2, 3, 0,
        // Back face
        4, 5, 6,    6, 7, 4,
        // Left face
        8, 9, 10,   10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Top face
        16, 17, 18, 18, 19, 16,
        // Bottom face
        20, 21, 22, 22, 23, 20
    };
    
    // Create vertex buffer
    m_vertexBuffer = BufferUtils::createVertexBuffer(
        m_device.getLogicalDevice(),
        m_device.getPhysicalDevice(),
        m_commandPool.getCommandPool(),
        m_device.getGraphicsQueue(),
        vertices
    );
    
    // Create index buffer
    m_indexBuffer = BufferUtils::createIndexBuffer(
        m_device.getLogicalDevice(),
        m_device.getPhysicalDevice(),
        m_commandPool.getCommandPool(),
        m_device.getGraphicsQueue(),
        indices
    );
    
    LOG_DEBUG("Fallback cube buffers created", "Engine");
}

void VulkanEngine::loadMainCharacter() {
    LOG_INFO("Attempting to load main character from assets/FinalBaseMesh.obj", "Engine");
    
    try {
        bool loaded = m_mainCharacter.loadFromOBJ(
            "assets/FinalBaseMesh.obj",
            m_device.getLogicalDevice(),
            m_device.getPhysicalDevice(),
            m_commandPool.getCommandPool(),
            m_device.getGraphicsQueue()
        );
        
        if (loaded) {
            m_useMainCharacter = true;
            LOG_INFO("Main character loaded successfully", "Engine");
        } else {
            m_useMainCharacter = false;
            LOG_WARN("Failed to load main character, falling back to cube", "Engine");
        }
        
    } catch (const std::exception& e) {
        m_useMainCharacter = false;
        LOG_WARN("Exception loading main character: " + std::string(e.what()) + ", falling back to cube", "Engine");
    }
}

void VulkanEngine::createUniformBuffers() {
    // Create one uniform buffer per frame in flight
    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_uniformBuffers[i] = BufferUtils::createUniformBuffer(
            m_device.getLogicalDevice(),
            m_device.getPhysicalDevice()
        );
    }
}

void VulkanEngine::createCommandBuffers() {
    m_commandBuffers = m_commandPool.allocateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // Begin recording
    m_commandPool.beginCommandBuffer(commandBuffer, VulkanCommandPool::Usage::SINGLE_USE);
    
    // Set up render area
    VkRect2D renderArea{};
    renderArea.offset = {0, 0};
    renderArea.extent = m_swapchain.getExtent();
    
    // Clear values
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};  // Clear color (black)
    clearValues[1].depthStencil = {1.0f, 0};             // Clear depth
    
    // Get the framebuffer for this image index
    const std::vector<VkFramebuffer>& framebuffers = m_renderPass.getFramebuffers();
    if (imageIndex >= framebuffers.size()) {
        throw std::runtime_error("Image index out of range for framebuffers");
    }
    
    // Determine which buffers to use for rendering
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    uint32_t indexCount;
    
    if (m_useMainCharacter && m_mainCharacter.isLoaded()) {
        vertexBuffer = m_mainCharacter.getVertexBuffer().getBuffer();
        indexBuffer = m_mainCharacter.getIndexBuffer().getBuffer();
        indexCount = m_mainCharacter.getIndexCount();
    } else {
        vertexBuffer = m_vertexBuffer.getBuffer();
        indexBuffer = m_indexBuffer.getBuffer();
        indexCount = 36; // 12 triangles * 3 indices for cube
    }
    
    // Use descriptor sets for uniform buffer binding
    std::vector<VkDescriptorSet> descriptorSets = {m_descriptorSets[m_currentFrame]};
    
    // Record frame commands
    m_commandPool.recordFrameCommands(
        commandBuffer,
        m_renderPass.getRenderPass(),
        framebuffers[imageIndex],       // Use correct framebuffer for this image
        m_pipeline.getPipeline(),
        m_pipeline.getPipelineLayout(),
        renderArea,
        clearValues,
        {vertexBuffer},                // Vertex buffers
        {0},                           // Vertex buffer offsets
        indexBuffer,                   // Index buffer
        0,                             // Index buffer offset
        descriptorSets,                // Descriptor sets for uniform buffers
        0,                             // Vertex count (using indexed drawing)
        indexCount,                    // Index count
        1                              // Instance count
    );
    
    // End recording
    m_commandPool.endCommandBuffer(commandBuffer);
}

void VulkanEngine::updateUniformBuffer(uint32_t currentImage) {
    UniformBufferObject ubo{};
    ubo.model = m_modelMatrix;
    ubo.view = m_viewMatrix;
    ubo.projection = m_projectionMatrix;
    
    m_uniformBuffers[currentImage].uploadData(&ubo, sizeof(ubo));
}

void VulkanEngine::recreateSwapchain() {
    VulkanUtils::logObjectCreation("VulkanEngine", "Recreating swapchain");
    
    // Wait for device to be idle
    waitIdle();
    
    // Clean up old swapchain-dependent resources
    m_renderPass.cleanup();
    m_pipeline.cleanup();
    
    // Clean up old depth buffer
    if (m_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device.getLogicalDevice(), m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device.getLogicalDevice(), m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }
    if (m_depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device.getLogicalDevice(), m_depthImageMemory, nullptr);
        m_depthImageMemory = VK_NULL_HANDLE;
    }
    
    m_swapchain.cleanup();
    
    // Recreate swapchain
    m_swapchain.create(m_device, m_surface, m_windowWidth, m_windowHeight);
    
    // Recreate render pass
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // Use same depth format as initialization
    m_renderPass.create(m_device.getLogicalDevice(), m_swapchain.getImageFormat(), depthFormat);
    
    // Recreate depth buffer and framebuffers
    createDepthBuffer();
    m_renderPass.createFramebuffers(m_swapchain.getImageViews(), m_depthImageView, m_swapchain.getExtent());
    
    // Recreate pipeline
    m_pipeline.createGraphicsPipeline(m_device.getLogicalDevice(), m_renderPass.getRenderPass(),
                     "shaders/vertex.vert.spv", "shaders/fragment.frag.spv", m_swapchain.getExtent());
    
    LOG_INFO("Swapchain recreated successfully", "Engine");
}

void VulkanEngine::setupScene() {
    /**
     * Projection Matrix Setup:
     * The projection matrix transforms vertices from view space to clip space.
     * We use a perspective projection to create the illusion of 3D depth.
     * 
     * Parameters:
     * - Field of View: 45 degrees (good balance between wide view and distortion)
     * - Aspect Ratio: width/height (maintains proper proportions)
     * - Near Plane: 0.1 units (closest visible distance)
     * - Far Plane: 50.0 units (increased to accommodate 10-unit camera offset)
     */
    float aspectRatio = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
    m_projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 50.0f);
    
    /**
     * Vulkan Coordinate System Adjustment:
     * GLM was designed for OpenGL, which has Y pointing up in clip space.
     * Vulkan has Y pointing down in clip space, so we flip the Y component
     * of the projection matrix to correct this difference.
     */
    m_projectionMatrix[1][1] *= -1;
    
    /**
     * Initial View Matrix:
     * Set up the camera at a fixed 10-unit offset from the origin.
     * This matches our updateScene() camera positioning.
     */
    m_viewMatrix = glm::lookAt(
        glm::vec3(10.0f, 5.0f, 10.0f),  // Camera position (10-unit offset)
        glm::vec3(0.0f, 0.0f, 0.0f),    // Look at origin (where character is positioned)
        glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector (Y is up in world space)
    );
    
    /**
     * Initial Model Matrix:
     * Start with identity matrix (no transformation).
     * Character/model will be positioned at origin.
     */
    m_modelMatrix = glm::mat4(1.0f);
    
    VulkanUtils::logObjectCreation("Scene", "3D scene setup completed");
    LOG_DEBUG("  - Field of view: 45 degrees", "Engine");
    LOG_DEBUG("  - Aspect ratio: " + std::to_string(aspectRatio), "Engine");
    LOG_DEBUG("  - Near plane: 0.1 units", "Engine");
    LOG_DEBUG("  - Far plane: 50.0 units", "Engine");
    LOG_DEBUG("  - Camera position: (10, 5, 10)", "Engine");
    LOG_DEBUG("  - Camera target: (0, 0, 0)", "Engine");
}

bool VulkanEngine::needsSwapchainRecreation() const {
    // Check if window is minimized (zero size)
    if (m_windowWidth == 0 || m_windowHeight == 0) {
        return false; // Don't recreate for minimized windows
    }
    
    // Check if current swapchain extent matches window size
    VkExtent2D currentExtent = m_swapchain.getExtent();
    return (currentExtent.width != m_windowWidth || currentExtent.height != m_windowHeight);
}

void VulkanEngine::getFrameStats(float& fps, float& frameTime) const {
    frameTime = m_lastFrameTime * 1000.0f; // Convert to milliseconds
    fps = (m_lastFrameTime > 0.0f) ? (1.0f / m_lastFrameTime) : 0.0f;
}

void VulkanEngine::logInitializationState(InitializationState state, const std::string& operation) {
    LOG_DEBUG("[VulkanEngine] " + operation + "...", "Engine");
}

void VulkanEngine::createDescriptorSets() {
    // Create descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    
    VK_CHECK(vkCreateDescriptorPool(m_device.getLogicalDevice(), &poolInfo, nullptr, &m_descriptorPool),
             "Failed to create descriptor pool");
    
    VulkanUtils::logObjectCreation("VkDescriptorPool", "Created for uniform buffers");
    
    // Allocate descriptor sets using the pipeline's descriptor set layout
    VkDescriptorSetLayout pipelineLayout = m_pipeline.getDescriptorSetLayout();
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipelineLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    
    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(m_device.getLogicalDevice(), &allocInfo, m_descriptorSets.data()),
             "Failed to allocate descriptor sets");
    
    VulkanUtils::logObjectCreation("DescriptorSets", 
        "Allocated " + std::to_string(MAX_FRAMES_IN_FLIGHT) + " descriptor sets");
}

void VulkanEngine::updateDescriptorSets() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(m_device.getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
    }
    
    VulkanUtils::logObjectCreation("DescriptorSets", "Updated with uniform buffer bindings");
}

void VulkanEngine::createDepthBuffer() {
    VkFormat depthFormat = findDepthFormat();
    
    // Create depth image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_swapchain.getExtent().width;
    imageInfo.extent.height = m_swapchain.getExtent().height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(m_device.getLogicalDevice(), &imageInfo, nullptr, &m_depthImage),
             "Failed to create depth image");

    // Allocate memory for depth image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device.getLogicalDevice(), m_depthImage, &memRequirements);

    // Find suitable memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device.getPhysicalDevice(), &memProperties);
    
    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memoryTypeIndex = i;
            break;
        }
    }
    
    if (memoryTypeIndex == UINT32_MAX) {
        throw std::runtime_error("Failed to find suitable memory type for depth buffer");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VK_CHECK(vkAllocateMemory(m_device.getLogicalDevice(), &allocInfo, nullptr, &m_depthImageMemory),
             "Failed to allocate depth image memory");

    vkBindImageMemory(m_device.getLogicalDevice(), m_depthImage, m_depthImageMemory, 0);

    // Create depth image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device.getLogicalDevice(), &viewInfo, nullptr, &m_depthImageView),
             "Failed to create depth image view");

    LOG_DEBUG("Depth buffer created successfully", "Engine");
}

VkFormat VulkanEngine::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat VulkanEngine::findSupportedFormat(const std::vector<VkFormat>& candidates,
                                          VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_device.getPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format");
}

} // namespace VulkanGameEngine