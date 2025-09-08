#include "../headers/VulkanRenderPass.h"
#include "../headers/VulkanUtils.h"

namespace VulkanGameEngine {

VulkanRenderPass::~VulkanRenderPass() {
    cleanup();
}

void VulkanRenderPass::create(VkDevice device, VkFormat colorFormat, VkFormat depthFormat, 
                             VkSampleCountFlagBits msaaSamples) {
    this->device = device;
    createRenderPass(colorFormat, depthFormat, msaaSamples);
}

void VulkanRenderPass::createRenderPass(VkFormat colorFormat, VkFormat depthFormat, 
                                       VkSampleCountFlagBits msaaSamples) {
    /*
     * Render Pass Creation Overview:
     * 
     * A render pass in Vulkan describes the structure of rendering operations.
     * It defines what types of framebuffer attachments will be used, how many
     * samples each has, and how their contents should be handled throughout
     * the rendering operations.
     * 
     * Key concepts:
     * - Attachments: Describe the properties of images used during rendering
     * - Subpasses: Describe which attachments are used for rendering operations
     * - Dependencies: Define memory and execution dependencies between subpasses
     */

    // Define color attachment (where the final rendered image goes)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;           // Must match swapchain format
    colorAttachment.samples = msaaSamples;          // Number of samples for MSAA
    
    /*
     * Load and Store Operations:
     * - loadOp: What to do with attachment contents at the start of render pass
     *   - VK_ATTACHMENT_LOAD_OP_LOAD: Preserve existing contents
     *   - VK_ATTACHMENT_LOAD_OP_CLEAR: Clear to a constant value
     *   - VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents undefined
     * 
     * - storeOp: What to do with attachment contents at the end of render pass
     *   - VK_ATTACHMENT_STORE_OP_STORE: Store contents for later use
     *   - VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents will be undefined after rendering
     */
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;    // Clear at start
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // Store for presentation
    
    // Stencil operations (not used in this basic setup)
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    /*
     * Image Layouts:
     * Vulkan images can be in different layouts optimized for different operations:
     * - VK_IMAGE_LAYOUT_UNDEFINED: Don't care about previous contents
     * - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Optimal for color attachment
     * - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Optimal for presentation to swapchain
     */
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Define depth attachment (for 3D depth testing)
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;    // Clear depth buffer
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Don't need to store depth
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /*
     * Attachment References:
     * These reference the attachments defined above by their index in the
     * attachments array. They specify which attachment a subpass will use
     * and in what layout.
     */
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;  // Index 0 in attachments array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;  // Index 1 in attachments array
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /*
     * Subpass Description:
     * A subpass is a phase of rendering that reads and writes a subset of
     * attachments in a render pass. Most render passes have only one subpass.
     * 
     * The pipelineBindPoint specifies the type of pipeline that will be used:
     * - VK_PIPELINE_BIND_POINT_GRAPHICS: Graphics pipeline
     * - VK_PIPELINE_BIND_POINT_COMPUTE: Compute pipeline
     */
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    /*
     * Subpass Dependencies:
     * These specify memory and execution dependencies between subpasses.
     * Even with a single subpass, we need dependencies to handle the
     * transition from/to external operations (like presentation).
     * 
     * VK_SUBPASS_EXTERNAL refers to operations outside the render pass.
     */
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // Operations before render pass
    dependency.dstSubpass = 0;                    // Our subpass (index 0)
    
    /*
     * Pipeline Stages:
     * These specify which stages of the pipeline the dependency applies to:
     * - COLOR_ATTACHMENT_OUTPUT: When color attachments are written
     * - EARLY_FRAGMENT_TESTS: When depth testing happens
     */
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;  // No access in source
    
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | 
                              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Collect all attachments
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    // Create the render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    VK_CHECK(result, "Failed to create render pass");
}

void VulkanRenderPass::createFramebuffers(const std::vector<VkImageView>& swapchainImageViews,
                                         VkImageView depthImageView, VkExtent2D extent) {
    this->extent = extent;
    
    /*
     * Framebuffer Creation:
     * 
     * Framebuffers represent the actual memory attachments that a render pass
     * will render into. They must be compatible with the render pass, meaning:
     * - Same number of attachments
     * - Compatible formats for each attachment
     * - Same sample counts
     * 
     * We create one framebuffer for each swapchain image so we can render
     * to different images while others are being presented.
     */
    
    framebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        /*
         * Attachment Array:
         * The order must match the attachment indices used in the render pass:
         * - Index 0: Color attachment (swapchain image)
         * - Index 1: Depth attachment (shared depth buffer)
         */
        std::array<VkImageView, 2> attachments = {
            swapchainImageViews[i],  // Color attachment
            depthImageView           // Depth attachment (shared across all framebuffers)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;  // Must be compatible with this render pass
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;  // Number of layers for multiview rendering (1 for normal rendering)

        VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]);
        VK_CHECK(result, "Failed to create framebuffer " + std::to_string(i));
    }
}

void VulkanRenderPass::recreateFramebuffers(const std::vector<VkImageView>& swapchainImageViews,
                                           VkImageView depthImageView, VkExtent2D extent) {
    /*
     * Framebuffer Recreation:
     * 
     * When the window is resized, the swapchain is recreated with new dimensions.
     * This means we need to recreate all framebuffers to match the new size.
     * We must destroy the old framebuffers first to avoid resource leaks.
     */
    
    // Wait for device to be idle to ensure framebuffers aren't in use
    vkDeviceWaitIdle(device);
    
    // Destroy existing framebuffers
    destroyFramebuffers();
    
    // Create new framebuffers with updated dimensions
    createFramebuffers(swapchainImageViews, depthImageView, extent);
}

void VulkanRenderPass::destroyFramebuffers() {
    for (VkFramebuffer framebuffer : framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    framebuffers.clear();
}

void VulkanRenderPass::cleanup() {
    if (device != VK_NULL_HANDLE) {
        // Destroy framebuffers first
        destroyFramebuffers();
        
        // Destroy render pass
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
        
        device = VK_NULL_HANDLE;
    }
}

} // namespace VulkanGameEngine