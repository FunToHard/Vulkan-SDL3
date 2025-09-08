#pragma once

#include "Common.h"
#include "VulkanDevice.h"
#include <vector>
#include <algorithm>
#include <limits>

namespace VulkanGameEngine {

/**
 * VulkanSwapchain manages the swapchain and its associated resources.
 * 
 * A swapchain is a collection of images that are waiting to be presented to the screen.
 * It's essentially a queue of images where:
 * 1. We render to one image while another is being displayed
 * 2. When rendering is complete, we swap the images (hence "swapchain")
 * 3. This allows for smooth animation without tearing or flickering
 * 
 * The swapchain is tightly coupled with the window surface and must be recreated
 * when the window is resized or other surface properties change.
 * 
 * Key concepts:
 * - Surface Format: Defines the color space and pixel format (e.g., RGBA8, sRGB)
 * - Present Mode: Defines how images are presented (immediate, FIFO, mailbox)
 * - Extent: The resolution/dimensions of the swapchain images
 * - Image Count: How many images are in the swapchain (double/triple buffering)
 */
class VulkanSwapchain {
public:
    /**
     * Structure containing swapchain support details for a device.
     * 
     * Before creating a swapchain, we need to query what the device and surface
     * support. This includes available formats, present modes, and capabilities.
     */
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;        // Basic surface capabilities
        std::vector<VkSurfaceFormatKHR> formats;      // Available surface formats
        std::vector<VkPresentModeKHR> presentModes;   // Available presentation modes
        
        /**
         * Check if swapchain support is adequate.
         * We need at least one format and one present mode to create a swapchain.
         */
        bool isAdequate() const {
            return !formats.empty() && !presentModes.empty();
        }
    };

    /**
     * Constructor - initializes member variables to safe defaults
     */
    VulkanSwapchain();

    /**
     * Destructor - ensures proper cleanup of swapchain resources
     */
    ~VulkanSwapchain();

    /**
     * Creates the swapchain with optimal settings.
     * 
     * This function performs several steps:
     * 1. Queries swapchain support details
     * 2. Selects optimal surface format, present mode, and extent
     * 3. Creates the swapchain with chosen settings
     * 4. Retrieves swapchain images and creates image views
     * 
     * @param device The Vulkan device to create swapchain with
     * @param surface The window surface to present to
     * @param width Current window width
     * @param height Current window height
     */
    void create(const VulkanDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height);

    /**
     * Recreates the swapchain with new dimensions.
     * 
     * This is necessary when the window is resized or other surface
     * properties change. The old swapchain is destroyed and a new
     * one is created with updated settings.
     * 
     * @param device The Vulkan device
     * @param surface The window surface
     * @param width New window width
     * @param height New window height
     */
    void recreate(const VulkanDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height);

    /**
     * Cleans up all swapchain resources.
     * Image views and swapchain must be destroyed in the correct order.
     */
    void cleanup();

    // Getters for swapchain properties and resources
    VkSwapchainKHR getSwapchain() const { return m_swapchain; }
    const std::vector<VkImage>& getImages() const { return m_images; }
    const std::vector<VkImageView>& getImageViews() const { return m_imageViews; }
    VkFormat getImageFormat() const { return m_imageFormat; }
    VkExtent2D getExtent() const { return m_extent; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }

    /**
     * Queries swapchain support details for a device and surface.
     * 
     * This is a static utility function that can be used to check
     * swapchain support before creating a VulkanSwapchain instance.
     * 
     * @param physicalDevice Physical device to query
     * @param surface Surface to query support for
     * @return Structure containing swapchain support details
     */
    static SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

private:
    // Core swapchain resources
    VkSwapchainKHR m_swapchain;              // The swapchain handle
    std::vector<VkImage> m_images;           // Images in the swapchain
    std::vector<VkImageView> m_imageViews;   // Image views for accessing images
    
    // Swapchain properties
    VkFormat m_imageFormat;                  // Format of swapchain images
    VkExtent2D m_extent;                     // Dimensions of swapchain images
    
    // Device references (not owned by this class)
    VkDevice m_device;                       // Logical device handle
    VkPhysicalDevice m_physicalDevice;       // Physical device handle

    /**
     * Selects the best surface format from available options.
     * 
     * Surface format defines the color space and pixel format of images.
     * We prefer sRGB color space with 8-bit RGBA format for best compatibility
     * and color accuracy.
     * 
     * Preferred formats (in order):
     * 1. VK_FORMAT_B8G8R8A8_SRGB with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
     * 2. VK_FORMAT_R8G8B8A8_SRGB with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
     * 3. First available format (fallback)
     * 
     * @param availableFormats List of formats supported by the surface
     * @return Selected surface format
     */
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    /**
     * Selects the best present mode from available options.
     * 
     * Present mode determines how images are presented to the screen:
     * - VK_PRESENT_MODE_IMMEDIATE_KHR: Images presented immediately (may cause tearing)
     * - VK_PRESENT_MODE_FIFO_KHR: Images presented in FIFO order (V-Sync, always available)
     * - VK_PRESENT_MODE_FIFO_RELAXED_KHR: Like FIFO but allows late images to be presented immediately
     * - VK_PRESENT_MODE_MAILBOX_KHR: Triple buffering (best for performance if available)
     * 
     * Preferred modes (in order):
     * 1. VK_PRESENT_MODE_MAILBOX_KHR (triple buffering, best performance)
     * 2. VK_PRESENT_MODE_FIFO_KHR (V-Sync, guaranteed to be available)
     * 
     * @param availablePresentModes List of present modes supported by the surface
     * @return Selected present mode
     */
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    /**
     * Selects the swap extent (resolution) for swapchain images.
     * 
     * The swap extent is usually equal to the window resolution, but some
     * window managers allow it to differ. We need to clamp the extent to
     * the supported range.
     * 
     * Special case: If currentExtent.width is UINT32_MAX, it means the
     * surface size will be determined by the extent we specify.
     * 
     * @param capabilities Surface capabilities containing extent limits
     * @param windowWidth Current window width
     * @param windowHeight Current window height
     * @return Selected swap extent
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t windowWidth, uint32_t windowHeight);

    /**
     * Determines the optimal number of images for the swapchain.
     * 
     * More images allow for better performance (triple buffering) but use
     * more memory. We try to use one more than the minimum to avoid waiting
     * for the driver to complete internal operations.
     * 
     * Rules:
     * - Use at least minImageCount + 1 for better performance
     * - Don't exceed maxImageCount (0 means no limit)
     * - Fall back to minImageCount if minImageCount + 1 exceeds maximum
     * 
     * @param capabilities Surface capabilities containing image count limits
     * @return Optimal number of swapchain images
     */
    uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

    /**
     * Creates image views for all swapchain images.
     * 
     * Image views define how images are accessed by shaders. They specify
     * the format, aspect (color/depth), and which mip levels and array
     * layers to access.
     * 
     * For swapchain images, we create simple 2D color image views that
     * access the entire image.
     */
    void createImageViews();

    /**
     * Destroys all image views.
     * This must be called before destroying the swapchain.
     */
    void destroyImageViews();
};

} // namespace VulkanGameEngine