#pragma once

#include "Common.h"

namespace VulkanGameEngine {

/**
 * VulkanRenderPass manages Vulkan render pass and framebuffer objects.
 * 
 * A render pass in Vulkan defines the structure of rendering operations:
 * - What types of attachments (color, depth) will be used
 * - How many samples each attachment has
 * - How the contents should be handled (load/store operations)
 * - Dependencies between subpasses
 * 
 * Framebuffers represent the actual memory attachments that the render pass
 * will render into. They must be compatible with the render pass structure.
 */
class VulkanRenderPass {
public:
    VulkanRenderPass() = default;
    ~VulkanRenderPass();

    // Disable copy constructor and assignment operator
    VulkanRenderPass(const VulkanRenderPass&) = delete;
    VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

    /**
     * Creates a render pass with color and depth attachments.
     * 
     * @param device The logical Vulkan device
     * @param colorFormat The format of the color attachment (usually swapchain format)
     * @param depthFormat The format of the depth attachment (e.g., VK_FORMAT_D32_SFLOAT)
     * @param msaaSamples Number of MSAA samples (VK_SAMPLE_COUNT_1_BIT for no MSAA)
     */
    void create(VkDevice device, VkFormat colorFormat, VkFormat depthFormat, 
                VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

    /**
     * Creates framebuffers for the render pass.
     * Must be called after create() and after swapchain image views are available.
     * 
     * @param swapchainImageViews The image views from the swapchain
     * @param depthImageView The depth buffer image view
     * @param extent The dimensions of the framebuffers
     */
    void createFramebuffers(const std::vector<VkImageView>& swapchainImageViews,
                           VkImageView depthImageView, VkExtent2D extent);

    /**
     * Recreates framebuffers with new dimensions.
     * Used when the window is resized and swapchain is recreated.
     */
    void recreateFramebuffers(const std::vector<VkImageView>& swapchainImageViews,
                             VkImageView depthImageView, VkExtent2D extent);

    /**
     * Cleans up all Vulkan resources.
     * Called automatically in destructor, but can be called manually for explicit cleanup.
     */
    void cleanup();

    // Getters
    VkRenderPass getRenderPass() const { return renderPass; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return framebuffers; }
    VkExtent2D getExtent() const { return extent; }

private:
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkExtent2D extent{};

    /**
     * Creates the render pass object with specified attachment formats.
     * 
     * The render pass defines:
     * - Color attachment: Where the final rendered image is stored
     * - Depth attachment: For depth testing and 3D rendering
     * - Load/store operations: How to handle attachment contents
     * - Subpass dependencies: Synchronization between rendering operations
     */
    void createRenderPass(VkFormat colorFormat, VkFormat depthFormat, 
                         VkSampleCountFlagBits msaaSamples);

    /**
     * Destroys all framebuffer objects.
     * Called when recreating framebuffers or during cleanup.
     */
    void destroyFramebuffers();
};

} // namespace VulkanGameEngine