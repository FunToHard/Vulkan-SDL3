#pragma once

#include "Common.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanCommandPool.h"
#include "VulkanSynchronization.h"
#include "MainCharacter.h"

namespace VulkanGameEngine {

/**
 * VulkanEngine is the main orchestrator class that manages the entire Vulkan rendering pipeline.
 * 
 * This class brings together all the individual Vulkan components into a cohesive rendering
 * system. It handles the initialization sequence, manages the render loop, and ensures
 * proper cleanup of all resources.
 * 
 * Key responsibilities:
 * - Initialize all Vulkan components in the correct order
 * - Manage the render loop with proper synchronization
 * - Handle window resize and swapchain recreation
 * - Coordinate between different Vulkan subsystems
 * - Provide a high-level interface for 3D rendering
 * 
 * The engine follows a modular design where each major Vulkan concept is encapsulated
 * in its own class, making the codebase maintainable and educational.
 */
class VulkanEngine {
public:
    /**
     * Engine initialization state for error handling and debugging
     */
    enum class InitializationState {
        NOT_INITIALIZED,
        INSTANCE_CREATED,
        SURFACE_CREATED,
        DEVICE_CREATED,
        SWAPCHAIN_CREATED,
        RENDER_PASS_CREATED,
        PIPELINE_CREATED,
        DESCRIPTORS_CREATED,  // Added descriptor set state
        BUFFERS_CREATED,
        COMMAND_POOL_CREATED,
        SYNCHRONIZATION_CREATED,
        CHARACTER_LOADED,
        FULLY_INITIALIZED
    };

    /**
     * Constructor - initializes engine to safe defaults
     */
    VulkanEngine();

    /**
     * Destructor - ensures proper cleanup of all resources
     */
    ~VulkanEngine();

    /**
     * Initializes the entire Vulkan rendering system.
     * 
     * This function performs the complete Vulkan initialization sequence:
     * 1. Create Vulkan instance with validation layers
     * 2. Create window surface for rendering
     * 3. Select and create logical device
     * 4. Create swapchain for presentation
     * 5. Create render pass for rendering operations
     * 6. Create graphics pipeline
     * 7. Create vertex and uniform buffers
     * 8. Create command pool and allocate command buffers
     * 9. Create synchronization objects
     * 10. Load main character model
     * 
     * Each step is carefully ordered to respect Vulkan's dependency requirements.
     * 
     * @param window SDL window to render to
     * @param windowWidth Width of the rendering area
     * @param windowHeight Height of the rendering area
     */
    void initialize(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight);

    /**
     * Renders a single frame.
     * 
     * This function implements the standard Vulkan rendering loop:
     * 1. Wait for previous frame to complete
     * 2. Acquire next swapchain image
     * 3. Record command buffer with rendering commands
     * 4. Update uniform buffers with current transformation matrices
     * 5. Submit command buffer to graphics queue
     * 6. Present rendered image to screen
     * 
     * The function handles synchronization between CPU and GPU to ensure
     * smooth rendering without artifacts or crashes.
     */
    void render();

    /**
     * Handles window resize events.
     * 
     * When the window is resized, the swapchain must be recreated with new
     * dimensions. This function handles the recreation process:
     * 1. Wait for device to be idle
     * 2. Recreate swapchain with new dimensions
     * 3. Recreate render pass and pipeline if needed
     * 4. Update viewport and scissor rectangles
     * 
     * @param newWidth New window width
     * @param newHeight New window height
     */
    void handleResize(uint32_t newWidth, uint32_t newHeight);

    /**
     * Checks if the swapchain needs to be recreated.
     * 
     * This can happen due to window resize, minimization, or other
     * surface changes that make the current swapchain invalid.
     * 
     * @return true if swapchain needs recreation
     */
    bool needsSwapchainRecreation() const;

    /**
     * Gets current frame statistics.
     * 
     * @param fps Output parameter for frames per second
     * @param frameTime Output parameter for frame time in milliseconds
     */
    void getFrameStats(float& fps, float& frameTime) const;

    /**
     * Updates the 3D scene for the current frame.
     * 
     * This function updates transformation matrices and other per-frame
     * data. It's called before rendering each frame to animate the scene.
     * 
     * @param deltaTime Time elapsed since last frame (in seconds)
     */
    void updateScene(float deltaTime);

    /**
     * Moves the camera based on WASD input.
     * 
     * @param forward Move forward/backward (W/S keys)
     * @param right Move right/left (D/A keys)
     * @param deltaTime Time elapsed since last frame for frame-rate independent movement
     */
    void moveCamera(float forward, float right, float deltaTime);

    /**
     * Waits for all GPU operations to complete.
     * 
     * This function is typically called before cleanup or when the
     * application needs to ensure all GPU work is finished.
     */
    void waitIdle();

    /**
     * Cleans up all Vulkan resources.
     * 
     * This function destroys all Vulkan objects in the correct order
     * to avoid validation errors. It's safe to call multiple times.
     */
    void cleanup();

    /**
     * Gets a reference to the main character for external manipulation.
     * 
     * @return Reference to the main character
     */
    MainCharacter& getMainCharacter() { return m_mainCharacter; }
    const MainCharacter& getMainCharacter() const { return m_mainCharacter; }

    // Getters for engine state and components
    InitializationState getInitializationState() const { return m_initState; }
    bool isInitialized() const { return m_initState == InitializationState::FULLY_INITIALIZED; }
    
    // Component access (for advanced usage)
    const VulkanInstance& getInstance() const { return m_instance; }
    const VulkanDevice& getDevice() const { return m_device; }
    const VulkanSwapchain& getSwapchain() const { return m_swapchain; }
    const VulkanRenderPass& getRenderPass() const { return m_renderPass; }
    const VulkanPipeline& getPipeline() const { return m_pipeline; }
    const VulkanCommandPool& getCommandPool() const { return m_commandPool; }
    const VulkanSynchronization& getSynchronization() const { return m_synchronization; }

    // Frame statistics
    uint64_t getFrameCount() const { return m_frameCount; }
    uint32_t getCurrentFrameIndex() const { return m_currentFrame; }
    float getLastFrameTime() const { return m_lastFrameTime; }

private:
    // Vulkan components (in initialization order)
    VulkanInstance m_instance;              // Vulkan instance and validation layers
    VulkanDevice m_device;                  // Physical and logical device management
    VulkanSwapchain m_swapchain;            // Swapchain for presentation
    VulkanRenderPass m_renderPass;          // Render pass configuration
    VulkanPipeline m_pipeline;              // Graphics pipeline
    VulkanCommandPool m_commandPool;        // Command buffer management
    VulkanSynchronization m_synchronization; // Synchronization objects
    
    // Vulkan handles that need direct access
    VkSurfaceKHR m_surface;                 // Window surface for rendering
    
    // Buffers for 3D rendering (fallback cube)
    VulkanBuffer m_vertexBuffer;            // Vertex data buffer
    VulkanBuffer m_indexBuffer;             // Index data buffer
    std::vector<VulkanBuffer> m_uniformBuffers; // Uniform buffers (one per frame in flight)
    
    // Depth buffer for 3D rendering
    VkImage m_depthImage;                   // Depth buffer image
    VkDeviceMemory m_depthImageMemory;      // Depth buffer memory
    VkImageView m_depthImageView;           // Depth buffer image view
    
    // Descriptor sets for uniform buffer binding
    VkDescriptorPool m_descriptorPool;                // Pool for allocating descriptor sets
    std::vector<VkDescriptorSet> m_descriptorSets;    // Descriptor sets (one per frame in flight)
    
    // 3D Models
    MainCharacter m_mainCharacter;          // Main character model
    bool m_useMainCharacter;                // Whether to render main character or fallback cube
    
    // Command buffers for rendering
    std::vector<VkCommandBuffer> m_commandBuffers; // Command buffers (one per frame in flight)
    
    // Engine state
    InitializationState m_initState;        // Current initialization state
    SDL_Window* m_window;                   // SDL window handle
    uint32_t m_windowWidth;                 // Current window width
    uint32_t m_windowHeight;                // Current window height
    
    // Frame management
    uint32_t m_currentFrame;                // Current frame index (for frames in flight)
    uint64_t m_frameCount;                  // Total frames rendered
    float m_lastFrameTime;                  // Time taken for last frame (in seconds)
    
    // Scene data
    float m_time;                           // Total elapsed time
    glm::mat4 m_modelMatrix;                // Model transformation matrix
    glm::mat4 m_viewMatrix;                 // View (camera) transformation matrix
    glm::mat4 m_projectionMatrix;           // Projection transformation matrix
    
    // Camera data
    glm::vec3 m_cameraPosition;             // Camera position in 3D space
    glm::vec3 m_cameraTarget;               // Point the camera is looking at
    float m_cameraSpeed;                    // Camera movement speed

    /**
     * Creates the window surface for rendering.
     * 
     * The surface is the connection between Vulkan and the window system.
     * SDL provides a cross-platform way to create Vulkan surfaces.
     * 
     * @param window SDL window to create surface for
     */
    void createSurface(SDL_Window* window);

    /**
     * Creates vertex and index buffers with test geometry.
     * 
     * This function creates buffers containing a simple 3D shape
     * (like a triangle or cube) for testing the rendering pipeline.
     */
    void createBuffers();

    /**
     * Loads the main character model from OBJ file.
     * 
     * This function attempts to load the character model from
     * assets/maincharacter.obj. If loading fails, falls back to cube.
     */
    void loadMainCharacter();

    /**
     * Creates uniform buffers for transformation matrices.
     * 
     * Uniform buffers store data that's constant for all vertices
     * in a draw call, like transformation matrices.
     */
    void createUniformBuffers();

    /**
     * Allocates command buffers for rendering.
     * 
     * Command buffers record the GPU commands that will be executed
     * during rendering. We need one per frame in flight.
     */
    void createCommandBuffers();

    /**
     * Creates depth buffer and depth image view.
     * 
     * This function creates the depth buffer image, allocates memory for it,
     * and creates an image view for use in the render pass.
     */
    void createDepthBuffer();

    /**
     * Finds a suitable depth format supported by the device.
     * 
     * @return A depth format that supports depth buffering
     */
    VkFormat findDepthFormat();

    /**
     * Finds a format from candidates that supports the given features.
     * 
     * @param candidates List of candidate formats to check
     * @param tiling Image tiling mode required
     * @param features Format features required
     * @return A supported format, or throws if none found
     */
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                VkImageTiling tiling, VkFormatFeatureFlags features);

    /**
     * Records rendering commands into a command buffer.
     * 
     * This function records the sequence of GPU commands needed
     * to render a frame, including setting up the render pass,
     * binding resources, and issuing draw calls.
     * 
     * @param commandBuffer Command buffer to record into
     * @param imageIndex Index of the swapchain image being rendered to
     */
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    /**
     * Updates uniform buffer data for the current frame.
     * 
     * This function calculates and uploads the current transformation
     * matrices to the uniform buffer for the specified frame.
     * 
     * @param currentImage Index of the current frame
     */
    void updateUniformBuffer(uint32_t currentImage);

    /**
     * Recreates the swapchain and dependent resources.
     * 
     * This function is called when the swapchain becomes invalid
     * (e.g., due to window resize) and needs to be recreated.
     */
    void recreateSwapchain();

    /**
     * Sets up the initial 3D scene.
     * 
     * This function initializes the camera position, projection matrix,
     * and other scene parameters.
     */
    void setupScene();

    /**
     * Logs the current initialization state for debugging.
     * 
     * @param state The state to log
     * @param operation Description of the operation
     */
    void logInitializationState(InitializationState state, const std::string& operation);

    /**
     * Creates descriptor pool and allocates descriptor sets.
     * 
     * Descriptor pool manages memory for descriptor sets, and we allocate
     * one descriptor set per frame in flight.
     */
    void createDescriptorSets();

    /**
     * Updates descriptor sets to point to current uniform buffers.
     * 
     * This function tells Vulkan which buffer resources each descriptor
     * set should reference.
     */
    void updateDescriptorSets();
};

} // namespace VulkanGameEngine} // namespace VulkanGameEngine