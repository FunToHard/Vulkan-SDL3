#pragma once

#include "Common.h"
#include <vector>

namespace VulkanGameEngine {

/**
 * VulkanSynchronization manages Vulkan synchronization primitives.
 * 
 * Vulkan is designed for high-performance parallel execution, which means
 * operations can happen asynchronously. Synchronization primitives ensure
 * that operations happen in the correct order and that resources are not
 * accessed simultaneously in conflicting ways.
 * 
 * Key synchronization concepts:
 * - Semaphores: GPU-GPU synchronization (signal when GPU work is done)
 * - Fences: CPU-GPU synchronization (CPU can wait for GPU work to complete)
 * - Events: Fine-grained synchronization within command buffers
 * - Barriers: Memory and execution dependencies between pipeline stages
 * 
 * This class provides educational implementations of common synchronization
 * patterns used in Vulkan applications, particularly for rendering loops.
 */
class VulkanSynchronization {
public:
    /**
     * Structure to hold synchronization objects for a single frame.
     * 
     * In a typical rendering application, we want multiple frames "in flight"
     * simultaneously to maximize GPU utilization. Each frame needs its own
     * set of synchronization objects to avoid conflicts.
     */
    struct FrameSyncObjects {
        VkSemaphore imageAvailableSemaphore;  // Signaled when swapchain image is available
        VkSemaphore renderFinishedSemaphore;  // Signaled when rendering is complete
        VkFence inFlightFence;                // CPU can wait for this frame to complete
        
        FrameSyncObjects() 
            : imageAvailableSemaphore(VK_NULL_HANDLE)
            , renderFinishedSemaphore(VK_NULL_HANDLE)
            , inFlightFence(VK_NULL_HANDLE) {}
    };

    /**
     * Constructor - initializes synchronization objects to safe defaults
     */
    VulkanSynchronization();

    /**
     * Destructor - ensures proper cleanup of synchronization resources
     */
    ~VulkanSynchronization();

    /**
     * Creates synchronization objects for multiple frames in flight.
     * 
     * This function creates the semaphores and fences needed for a rendering
     * loop that processes multiple frames simultaneously. Having multiple
     * frames in flight improves performance by allowing the CPU to prepare
     * the next frame while the GPU is still working on the current frame.
     * 
     * @param device Logical device to create objects on
     * @param maxFramesInFlight Number of frames that can be processed simultaneously
     */
    void create(VkDevice device, uint32_t maxFramesInFlight = MAX_FRAMES_IN_FLIGHT);

    /**
     * Waits for the fence of a specific frame to be signaled.
     * 
     * This function blocks the CPU until the GPU has finished processing
     * the specified frame. This is typically called at the beginning of
     * the render loop to ensure the frame's resources are available.
     * 
     * @param frameIndex Index of the frame to wait for
     * @param timeout Timeout in nanoseconds (UINT64_MAX for infinite)
     * @return true if fence was signaled, false if timeout occurred
     */
    bool waitForFrame(uint32_t frameIndex, uint64_t timeout = UINT64_MAX);

    /**
     * Resets the fence for a specific frame.
     * 
     * Fences must be reset before they can be used again. This is typically
     * called after waiting for a frame and before submitting new work.
     * 
     * @param frameIndex Index of the frame fence to reset
     */
    void resetFrameFence(uint32_t frameIndex);

    /**
     * Waits for all frames to complete.
     * 
     * This function waits for all in-flight frames to finish. It's typically
     * used during shutdown to ensure all GPU work is complete before
     * destroying resources.
     * 
     * @param timeout Timeout in nanoseconds (UINT64_MAX for infinite)
     * @return true if all fences were signaled, false if timeout occurred
     */
    bool waitForAllFrames(uint64_t timeout = UINT64_MAX);

    /**
     * Creates a semaphore for GPU-GPU synchronization.
     * 
     * Semaphores are used to synchronize operations between different
     * queues or different submissions to the same queue. They are
     * signaled by GPU operations and waited on by other GPU operations.
     * 
     * @param device Logical device to create semaphore on
     * @return Handle to the created semaphore
     */
    VkSemaphore createSemaphore(VkDevice device);

    /**
     * Creates a fence for CPU-GPU synchronization.
     * 
     * Fences allow the CPU to wait for GPU operations to complete.
     * They can be created in signaled or unsignaled state.
     * 
     * @param device Logical device to create fence on
     * @param signaled Whether to create the fence in signaled state
     * @return Handle to the created fence
     */
    VkFence createFence(VkDevice device, bool signaled = true);

    /**
     * Destroys a semaphore.
     * 
     * @param device Logical device the semaphore was created on
     * @param semaphore Semaphore to destroy
     */
    void destroySemaphore(VkDevice device, VkSemaphore semaphore);

    /**
     * Destroys a fence.
     * 
     * @param device Logical device the fence was created on
     * @param fence Fence to destroy
     */
    void destroyFence(VkDevice device, VkFence fence);

    /**
     * Submits command buffers to a queue with synchronization.
     * 
     * This function submits command buffers to a queue with proper
     * synchronization using semaphores and fences. It handles the
     * common pattern of waiting for image availability, executing
     * rendering commands, and signaling completion.
     * 
     * @param queue Queue to submit to
     * @param commandBuffers Command buffers to submit
     * @param waitSemaphores Semaphores to wait for before execution
     * @param waitStages Pipeline stages to wait at for each semaphore
     * @param signalSemaphores Semaphores to signal after execution
     * @param fence Fence to signal when submission completes (optional)
     */
    void submitCommandBuffers(VkQueue queue,
                             const std::vector<VkCommandBuffer>& commandBuffers,
                             const std::vector<VkSemaphore>& waitSemaphores = {},
                             const std::vector<VkPipelineStageFlags>& waitStages = {},
                             const std::vector<VkSemaphore>& signalSemaphores = {},
                             VkFence fence = VK_NULL_HANDLE);

    /**
     * Presents a swapchain image with synchronization.
     * 
     * This function presents a rendered image to the swapchain, waiting
     * for rendering to complete before presentation.
     * 
     * @param presentQueue Queue to present on
     * @param swapchain Swapchain to present to
     * @param imageIndex Index of the image to present
     * @param waitSemaphores Semaphores to wait for before presentation
     * @return VkResult from the present operation
     */
    VkResult presentImage(VkQueue presentQueue, VkSwapchainKHR swapchain,
                         uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores = {});

    /**
     * Acquires the next image from the swapchain with synchronization.
     * 
     * This function gets the next available image from the swapchain,
     * signaling a semaphore when the image is ready for rendering.
     * 
     * @param device Logical device
     * @param swapchain Swapchain to acquire from
     * @param timeout Timeout in nanoseconds
     * @param semaphore Semaphore to signal when image is available
     * @param fence Fence to signal when image is available (optional)
     * @param imageIndex Output parameter for the acquired image index
     * @return VkResult from the acquire operation
     */
    VkResult acquireNextImage(VkDevice device, VkSwapchainKHR swapchain,
                             uint64_t timeout, VkSemaphore semaphore,
                             VkFence fence, uint32_t* imageIndex);

    /**
     * Cleans up all synchronization resources.
     * 
     * This function destroys all semaphores and fences created by this class.
     * It's safe to call multiple times.
     */
    void cleanup();

    // Getters for frame synchronization objects
    const FrameSyncObjects& getFrameSyncObjects(uint32_t frameIndex) const;
    uint32_t getMaxFramesInFlight() const { return m_maxFramesInFlight; }
    
    // Getters for individual synchronization objects
    VkSemaphore getImageAvailableSemaphore(uint32_t frameIndex) const;
    VkSemaphore getRenderFinishedSemaphore(uint32_t frameIndex) const;
    VkFence getInFlightFence(uint32_t frameIndex) const;

private:
    // Vulkan handles
    VkDevice m_device;              // Logical device (needed for cleanup)
    
    // Frame synchronization objects
    std::vector<FrameSyncObjects> m_frameSyncObjects;  // One set per frame in flight
    uint32_t m_maxFramesInFlight;                      // Number of frames in flight
    
    // Additional synchronization objects for manual management
    std::vector<VkSemaphore> m_semaphores;  // Manually created semaphores
    std::vector<VkFence> m_fences;          // Manually created fences

    /**
     * Creates a semaphore with detailed logging.
     * 
     * @param device Logical device
     * @param name Name for logging purposes
     * @return Created semaphore handle
     */
    VkSemaphore createSemaphoreInternal(VkDevice device, const std::string& name);

    /**
     * Creates a fence with detailed logging.
     * 
     * @param device Logical device
     * @param signaled Initial state of the fence
     * @param name Name for logging purposes
     * @return Created fence handle
     */
    VkFence createFenceInternal(VkDevice device, bool signaled, const std::string& name);
};

/**
 * Utility functions for common synchronization patterns
 */
namespace SynchronizationUtils {
    /**
     * Creates a simple submit info structure for command buffer submission.
     * 
     * @param commandBuffer Command buffer to submit
     * @param waitSemaphore Semaphore to wait for (optional)
     * @param waitStage Pipeline stage to wait at
     * @param signalSemaphore Semaphore to signal (optional)
     * @return Configured VkSubmitInfo structure
     */
    VkSubmitInfo createSubmitInfo(VkCommandBuffer commandBuffer,
                                 VkSemaphore waitSemaphore = VK_NULL_HANDLE,
                                 VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VkSemaphore signalSemaphore = VK_NULL_HANDLE);

    /**
     * Creates a present info structure for swapchain presentation.
     * 
     * @param swapchain Swapchain to present to
     * @param imageIndex Index of image to present
     * @param waitSemaphore Semaphore to wait for (optional)
     * @return Configured VkPresentInfoKHR structure
     */
    VkPresentInfoKHR createPresentInfo(VkSwapchainKHR swapchain, uint32_t imageIndex,
                                      VkSemaphore waitSemaphore = VK_NULL_HANDLE);

    /**
     * Waits for multiple fences with timeout.
     * 
     * @param device Logical device
     * @param fences Vector of fences to wait for
     * @param waitAll Whether to wait for all fences or just one
     * @param timeout Timeout in nanoseconds
     * @return VkResult from the wait operation
     */
    VkResult waitForFences(VkDevice device, const std::vector<VkFence>& fences,
                          bool waitAll = true, uint64_t timeout = UINT64_MAX);

    /**
     * Resets multiple fences.
     * 
     * @param device Logical device
     * @param fences Vector of fences to reset
     */
    void resetFences(VkDevice device, const std::vector<VkFence>& fences);
}

} // namespace VulkanGameEngine