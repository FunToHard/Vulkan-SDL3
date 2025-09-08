#include "VulkanSwapchain.h"
#include "VulkanUtils.h"
#include <iostream>
#include <algorithm>
#include <limits>

namespace VulkanGameEngine {

VulkanSwapchain::VulkanSwapchain()
    : m_swapchain(VK_NULL_HANDLE)
    , m_imageFormat(VK_FORMAT_UNDEFINED)
    , m_extent({0, 0})
    , m_device(VK_NULL_HANDLE)
    , m_physicalDevice(VK_NULL_HANDLE)
{
}

VulkanSwapchain::~VulkanSwapchain() {
    cleanup();
}

void VulkanSwapchain::create(const VulkanDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    // Store device references
    m_device = device.getLogicalDevice();
    m_physicalDevice = device.getPhysicalDevice();
    
    // Query swapchain support details
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(m_physicalDevice, surface);
    
    if (!swapchainSupport.isAdequate()) {
        throw std::runtime_error("Swapchain support is not adequate for this device and surface");
    }
    
    // Choose optimal swapchain settings
    VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = choosePresentMode(swapchainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities, width, height);
    uint32_t imageCount = chooseImageCount(swapchainSupport.capabilities);
    
    // Store chosen properties
    m_imageFormat = surfaceFormat.format;
    m_extent = extent;
    
    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    
    // Image settings
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;  // Always 1 unless developing stereoscopic 3D application
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // Render directly to images
    
    // Queue family handling
    const auto& queueFamilyIndices = device.getQueueFamilyIndices();
    uint32_t queueFamilyIndicesArray[] = {
        queueFamilyIndices.graphicsFamily.value(),
        queueFamilyIndices.presentFamily.value()
    };
    
    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
        // Graphics and present queues are from different families
        // Images can be used across multiple queue families without explicit ownership transfers
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
        std::cout << "Using concurrent sharing mode for swapchain images (different queue families)" << std::endl;
    } else {
        // Graphics and present queues are from the same family
        // Images are owned by one queue family at a time (better performance)
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
        std::cout << "Using exclusive sharing mode for swapchain images (same queue family)" << std::endl;
    }
    
    // Transform and composition
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;  // No transformation
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;            // Ignore alpha channel
    
    // Present mode and clipping
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;  // Don't care about pixels obscured by other windows
    
    // Old swapchain (for recreation)
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    
    // Create the swapchain
    VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain),
             "Failed to create swapchain");
    
    std::cout << "Created swapchain with " << imageCount << " images, format " << m_imageFormat 
              << ", extent " << m_extent.width << "x" << m_extent.height << std::endl;
    
    // Retrieve swapchain images
    uint32_t actualImageCount;
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualImageCount, nullptr),
             "Failed to get swapchain image count");
    
    m_images.resize(actualImageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualImageCount, m_images.data()),
             "Failed to get swapchain images");
    
    std::cout << "Retrieved " << actualImageCount << " swapchain images" << std::endl;
    
    // Create image views
    createImageViews();
}

void VulkanSwapchain::recreate(const VulkanDevice& device, VkSurfaceKHR surface, uint32_t width, uint32_t height) {
    // Wait for device to be idle before recreating swapchain
    vkDeviceWaitIdle(m_device);
    
    std::cout << "Recreating swapchain for new dimensions: " << width << "x" << height << std::endl;
    
    // Clean up old swapchain resources
    cleanup();
    
    // Create new swapchain with updated dimensions
    create(device, surface, width, height);
}

void VulkanSwapchain::cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        // Destroy image views first
        destroyImageViews();
        
        // Destroy swapchain
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
    }
    
    // Clear image handles (these are owned by the swapchain, not us)
    m_images.clear();
    
    // Reset properties
    m_imageFormat = VK_FORMAT_UNDEFINED;
    m_extent = {0, 0};
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
}

VulkanSwapchain::SwapchainSupportDetails VulkanSwapchain::querySwapchainSupport(
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR surface) {
    
    SwapchainSupportDetails details;
    
    // Query basic surface capabilities
    // This includes min/max image count, current/min/max image extent, and supported transforms
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities),
             "Failed to get surface capabilities");
    
    // Query supported surface formats
    // Surface formats define the pixel format and color space of images
    uint32_t formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr),
             "Failed to get surface format count");
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data()),
                 "Failed to get surface formats");
    }
    
    // Query supported present modes
    // Present modes define how images are presented to the screen (immediate, FIFO, mailbox, etc.)
    uint32_t presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr),
             "Failed to get present mode count");
    
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data()),
                 "Failed to get present modes");
    }
    
    return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Surface format defines the pixel format and color space of swapchain images
    // We prefer sRGB color space for better color accuracy in most applications
    
    // First preference: B8G8R8A8_SRGB with sRGB color space
    // This is the most common format and provides good performance and color accuracy
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            std::cout << "Selected surface format: VK_FORMAT_B8G8R8A8_SRGB with sRGB color space" << std::endl;
            return availableFormat;
        }
    }
    
    // Second preference: R8G8B8A8_SRGB with sRGB color space
    // Alternative RGBA format that's also widely supported
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            std::cout << "Selected surface format: VK_FORMAT_R8G8B8A8_SRGB with sRGB color space" << std::endl;
            return availableFormat;
        }
    }
    
    // Fallback: Use the first available format
    // This ensures we can always create a swapchain, even if it's not optimal
    std::cout << "Using fallback surface format: " << availableFormats[0].format 
              << " with color space: " << availableFormats[0].colorSpace << std::endl;
    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Present mode determines how images are presented to the screen
    // Different modes offer different trade-offs between performance, power consumption, and visual quality
    
    // First preference: VK_PRESENT_MODE_MAILBOX_KHR (triple buffering)
    // This mode provides the best performance by allowing the application to render
    // as fast as possible while still syncing to the display refresh rate
    // It uses more memory but reduces input lag compared to FIFO
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            std::cout << "Selected present mode: VK_PRESENT_MODE_MAILBOX_KHR (triple buffering)" << std::endl;
            return availablePresentMode;
        }
    }
    
    // Second preference: VK_PRESENT_MODE_IMMEDIATE_KHR
    // This mode presents images immediately without waiting for vertical blank
    // It can cause tearing but provides the lowest latency
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            std::cout << "Selected present mode: VK_PRESENT_MODE_IMMEDIATE_KHR (immediate)" << std::endl;
            return availablePresentMode;
        }
    }
    
    // Fallback: VK_PRESENT_MODE_FIFO_KHR (V-Sync)
    // This mode is guaranteed to be available on all implementations
    // It waits for vertical blank before presenting, preventing tearing
    // but may cause stuttering if the application can't maintain the refresh rate
    std::cout << "Selected present mode: VK_PRESENT_MODE_FIFO_KHR (V-Sync, guaranteed available)" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, 
                                           uint32_t windowWidth, 
                                           uint32_t windowHeight) {
    // The swap extent is the resolution of the swapchain images
    // It's usually equal to the window resolution, but some window managers allow it to differ
    
    // Special case: If currentExtent.width is UINT32_MAX, it means the surface size
    // will be determined by the extent of the swapchain we create
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        // Most window managers set currentExtent to the window resolution
        // In this case, we must use this exact extent
        std::cout << "Using surface-defined extent: " << capabilities.currentExtent.width 
                  << "x" << capabilities.currentExtent.height << std::endl;
        return capabilities.currentExtent;
    } else {
        // The window manager allows us to choose the extent
        // We should pick the resolution that best matches the window within the allowed bounds
        
        VkExtent2D actualExtent = {windowWidth, windowHeight};
        
        // Clamp the extent to the supported range
        // minImageExtent and maxImageExtent define the allowed range
        actualExtent.width = std::clamp(actualExtent.width, 
                                      capabilities.minImageExtent.width, 
                                      capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, 
                                       capabilities.minImageExtent.height, 
                                       capabilities.maxImageExtent.height);
        
        std::cout << "Chose swap extent: " << actualExtent.width << "x" << actualExtent.height 
                  << " (clamped from window size " << windowWidth << "x" << windowHeight << ")" << std::endl;
        return actualExtent;
    }
}

uint32_t VulkanSwapchain::chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) {
    // Determine the optimal number of images in the swapchain
    // More images allow for better performance (triple buffering) but use more memory
    
    // Start with minimum + 1 for better performance
    // This allows the application to render to one image while another is being presented
    uint32_t imageCount = capabilities.minImageCount + 1;
    
    // Make sure we don't exceed the maximum (0 means no limit)
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
        std::cout << "Clamped image count to maximum: " << imageCount << std::endl;
    } else {
        std::cout << "Using image count: " << imageCount 
                  << " (minimum: " << capabilities.minImageCount 
                  << ", maximum: " << (capabilities.maxImageCount == 0 ? "unlimited" : std::to_string(capabilities.maxImageCount)) 
                  << ")" << std::endl;
    }
    
    return imageCount;
}

void VulkanSwapchain::createImageViews() {
    // Create image views for all swapchain images
    // Image views define how images are accessed by shaders and render passes
    
    m_imageViews.resize(m_images.size());
    
    for (size_t i = 0; i < m_images.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_images[i];
        
        // Specify how the image should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // Treat as 2D image
        createInfo.format = m_imageFormat;            // Use same format as swapchain
        
        // Component mapping allows swizzling color channels
        // We use identity mapping (no swizzling)
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        // Subresource range describes which part of the image the view accesses
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Color data
        createInfo.subresourceRange.baseMipLevel = 0;                        // Start at mip level 0
        createInfo.subresourceRange.levelCount = 1;                          // Only one mip level
        createInfo.subresourceRange.baseArrayLayer = 0;                      // Start at array layer 0
        createInfo.subresourceRange.layerCount = 1;                          // Only one array layer
        
        VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]),
                 "Failed to create image view " + std::to_string(i));
    }
    
    std::cout << "Created " << m_imageViews.size() << " image views for swapchain images" << std::endl;
}

void VulkanSwapchain::destroyImageViews() {
    // Destroy all image views
    for (auto imageView : m_imageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
    }
    m_imageViews.clear();
}

} // namespace VulkanGameEngine