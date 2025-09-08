#pragma once

#include "Common.h"
#include <vector>
#include <string>

namespace VulkanGameEngine {

/**
 * VulkanInstance manages the Vulkan instance creation and validation layers.
 * 
 * The Vulkan instance is the connection between your application and the Vulkan library.
 * It's the first object you need to create when using Vulkan, and it serves as the
 * entry point to the Vulkan API. The instance stores application-level state and
 * enables validation layers for debugging.
 */
class VulkanInstance {
public:
    /**
     * Constructor - initializes member variables to safe defaults
     */
    VulkanInstance();

    /**
     * Destructor - ensures proper cleanup of Vulkan resources
     */
    ~VulkanInstance();

    /**
     * Creates the Vulkan instance with the specified extensions and validation layers.
     * 
     * @param requiredExtensions Vector of extension names required by the application
     *                          (typically includes surface extensions for window system integration)
     * @param enableValidationLayers Whether to enable validation layers for debugging
     * 
     * The Vulkan instance acts as the bridge between your application and the Vulkan
     * implementation. It's responsible for:
     * - Loading the Vulkan library
     * - Enabling instance-level extensions (like surface support)
     * - Setting up validation layers for debugging and error checking
     */
    void create(const std::vector<const char*>& requiredExtensions, bool enableValidationLayers = true);

    /**
     * Cleans up all Vulkan resources managed by this instance.
     * This includes the debug messenger and the Vulkan instance itself.
     */
    void cleanup();

    /**
     * Returns the underlying VkInstance handle.
     * This is needed by other Vulkan objects that require an instance.
     */
    VkInstance getInstance() const { return m_instance; }

    /**
     * Checks if validation layers are currently enabled.
     */
    bool areValidationLayersEnabled() const { return m_validationLayersEnabled; }

private:
    // Core Vulkan instance handle
    VkInstance m_instance;

    // Debug messenger for validation layer output
    // The debug messenger allows us to receive detailed error messages,
    // warnings, and performance hints from the validation layers
    VkDebugUtilsMessengerEXT m_debugMessenger;

    // Flag to track if validation layers are enabled
    bool m_validationLayersEnabled;

    // Validation layers to request
    // These layers provide extensive debugging and validation of Vulkan usage
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    /**
     * Sets up the debug messenger for validation layer output.
     * 
     * The debug messenger is a Vulkan extension that allows validation layers
     * to send detailed messages about API usage, errors, and performance warnings
     * directly to our application through a callback function.
     */
    void setupDebugMessenger();

    /**
     * Checks if all requested validation layers are available on this system.
     * 
     * @return true if all validation layers are supported, false otherwise
     * 
     * Validation layers are optional components that can be enabled during
     * development to catch errors and provide debugging information. They
     * should typically be disabled in release builds for performance.
     */
    bool checkValidationLayerSupport();

    /**
     * Populates the debug messenger create info structure.
     * This is used both for instance creation and debug messenger setup.
     * 
     * @param createInfo Reference to the create info structure to populate
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    /**
     * Debug callback function for validation layer messages.
     * 
     * This static function receives all debug messages from validation layers
     * and formats them in an educational way to help understand what's happening.
     * 
     * @param messageSeverity Severity level of the message (verbose, info, warning, error)
     * @param messageType Type of message (general, validation, performance)
     * @param pCallbackData Detailed information about the message
     * @param pUserData User-defined data (unused in our case)
     * @return VK_FALSE to continue execution (VK_TRUE would abort)
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

} // namespace VulkanGameEngine