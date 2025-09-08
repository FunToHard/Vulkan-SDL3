#pragma once

#include "Common.h"
#include <vector>
#include <optional>
#include <set>

namespace VulkanGameEngine {

/**
 * VulkanDevice manages physical and logical device selection and creation.
 * 
 * In Vulkan, there are two types of devices:
 * 1. Physical Device (VkPhysicalDevice): Represents an actual GPU or graphics hardware
 * 2. Logical Device (VkDevice): A software interface to interact with the physical device
 * 
 * The physical device provides information about capabilities, features, and properties
 * of the hardware. The logical device is what we use to create resources and submit
 * commands. This class handles the selection of the best physical device and creation
 * of an appropriate logical device with the required features and extensions.
 */
class VulkanDevice {
public:
    /**
     * Structure to hold queue family indices.
     * 
     * Queue families represent different types of operations that can be performed:
     * - Graphics: Rendering operations (vertex processing, fragment shading)
     * - Present: Presenting images to the screen/surface
     * - Compute: General purpose computing operations
     * - Transfer: Memory transfer operations
     * 
     * Some queue families support multiple operation types, while others are specialized.
     * We need at least graphics and present capabilities for our 3D engine.
     */
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;  // Queue family that supports graphics operations
        std::optional<uint32_t> presentFamily;   // Queue family that supports presentation
        std::optional<uint32_t> computeFamily;   // Queue family that supports compute operations (optional)
        std::optional<uint32_t> transferFamily;  // Queue family that supports transfer operations (optional)
        
        /**
         * Check if we have found all required queue families.
         * For basic 3D rendering, we need graphics and present capabilities.
         */
        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
        
        /**
         * Get unique queue families to avoid creating duplicate queues.
         * This is important because some devices have queue families that
         * support multiple operations (e.g., graphics + present).
         */
        std::set<uint32_t> getUniqueQueueFamilies() const {
            std::set<uint32_t> uniqueFamilies;
            if (graphicsFamily.has_value()) uniqueFamilies.insert(graphicsFamily.value());
            if (presentFamily.has_value()) uniqueFamilies.insert(presentFamily.value());
            if (computeFamily.has_value()) uniqueFamilies.insert(computeFamily.value());
            if (transferFamily.has_value()) uniqueFamilies.insert(transferFamily.value());
            return uniqueFamilies;
        }
    };

    /**
     * Structure to hold swapchain support details.
     * 
     * The swapchain is responsible for managing the images that get presented
     * to the screen. Different devices support different swapchain configurations,
     * so we need to query what's available before creating one.
     */
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;        // Basic surface capabilities (min/max images, dimensions)
        std::vector<VkSurfaceFormatKHR> formats;      // Available surface formats (color space, format)
        std::vector<VkPresentModeKHR> presentModes;   // Available presentation modes (immediate, FIFO, etc.)
        
        /**
         * Check if swapchain support is adequate for our needs.
         * We need at least one format and one present mode.
         */
        bool isAdequate() const {
            return !formats.empty() && !presentModes.empty();
        }
    };

    /**
     * Constructor - initializes member variables to safe defaults
     */
    VulkanDevice();

    /**
     * Destructor - ensures proper cleanup of device resources
     */
    ~VulkanDevice();

    /**
     * Creates the logical device after selecting the best physical device.
     * 
     * This function performs several important steps:
     * 1. Enumerates all available physical devices
     * 2. Scores each device based on suitability criteria
     * 3. Selects the best device for our needs
     * 4. Creates a logical device with required features and extensions
     * 5. Retrieves queue handles for graphics and presentation
     * 
     * @param instance The Vulkan instance to enumerate devices from
     * @param surface The window surface we need to present to
     */
    void create(VkInstance instance, VkSurfaceKHR surface);

    /**
     * Cleans up all device resources.
     * The logical device must be destroyed before the instance.
     */
    void cleanup();

    // Getters for device handles and properties
    VkDevice getLogicalDevice() const { return m_logicalDevice; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    
    // Queue handles for different operations
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkQueue getComputeQueue() const { return m_computeQueue; }
    VkQueue getTransferQueue() const { return m_transferQueue; }
    
    // Queue family information
    const QueueFamilyIndices& getQueueFamilyIndices() const { return m_queueFamilyIndices; }
    
    // Device properties and features
    const VkPhysicalDeviceProperties& getDeviceProperties() const { return m_deviceProperties; }
    const VkPhysicalDeviceFeatures& getDeviceFeatures() const { return m_deviceFeatures; }
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const { return m_memoryProperties; }
    
    // Swapchain support information
    SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface) const;

private:
    // Device handles
    VkPhysicalDevice m_physicalDevice;  // The selected physical device (GPU)
    VkDevice m_logicalDevice;           // The logical device interface
    
    // Queue handles - these are used to submit commands to the GPU
    VkQueue m_graphicsQueue;    // Queue for graphics operations (rendering)
    VkQueue m_presentQueue;     // Queue for presentation operations (displaying to screen)
    VkQueue m_computeQueue;     // Queue for compute operations (optional)
    VkQueue m_transferQueue;    // Queue for transfer operations (optional)
    
    // Queue family information
    QueueFamilyIndices m_queueFamilyIndices;
    
    // Device properties and capabilities
    VkPhysicalDeviceProperties m_deviceProperties;      // Basic device info (name, type, limits)
    VkPhysicalDeviceFeatures m_deviceFeatures;          // Optional features (geometry shaders, etc.)
    VkPhysicalDeviceMemoryProperties m_memoryProperties; // Memory types and heaps available
    
    // Required device extensions
    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME  // Required for presenting images to screen
    };

    /**
     * Enumerates and selects the best physical device.
     * 
     * This function queries all available physical devices and scores them
     * based on various criteria like device type, feature support, and
     * queue family availability.
     * 
     * @param instance Vulkan instance to query devices from
     * @param surface Surface to check presentation support against
     */
    void selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

    /**
     * Creates the logical device with required queues and extensions.
     * 
     * The logical device is our interface to the physical device. We specify
     * which queues we need, which extensions to enable, and which features
     * we want to use.
     * 
     * @param surface Surface for presentation queue creation
     */
    void createLogicalDevice(VkSurfaceKHR surface);

    /**
     * Scores a physical device based on suitability for our application.
     * 
     * Higher scores indicate better suitability. Devices that don't meet
     * minimum requirements receive a score of 0.
     * 
     * Scoring criteria:
     * - Device type (discrete GPU > integrated GPU > other)
     * - Maximum texture size and other limits
     * - Available memory
     * - Feature support
     * 
     * @param device Physical device to score
     * @param surface Surface to check compatibility against
     * @return Suitability score (0 = unsuitable, higher = better)
     */
    uint32_t scorePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface);

    /**
     * Checks if a physical device meets minimum requirements.
     * 
     * Minimum requirements:
     * - Supports required queue families (graphics + present)
     * - Supports required extensions (swapchain)
     * - Has adequate swapchain support
     * - Supports required features
     * 
     * @param device Physical device to check
     * @param surface Surface to check presentation support against
     * @return true if device is suitable, false otherwise
     */
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

    /**
     * Finds queue family indices for a physical device.
     * 
     * Different queue families support different types of operations.
     * We need to find families that support graphics operations and
     * presentation to our surface.
     * 
     * @param device Physical device to query
     * @param surface Surface to check presentation support against
     * @return Structure containing found queue family indices
     */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    /**
     * Checks if device supports all required extensions.
     * 
     * Extensions provide additional functionality beyond the core Vulkan API.
     * For rendering to a screen, we need the swapchain extension.
     * 
     * @param device Physical device to check
     * @return true if all required extensions are supported
     */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    /**
     * Queries swapchain support details for a physical device.
     * 
     * The swapchain manages the images that get presented to the screen.
     * We need to know what formats, present modes, and capabilities
     * are available.
     * 
     * @param device Physical device to query
     * @param surface Surface to query support for
     * @return Structure containing swapchain support details
     */
    SwapchainSupportDetails querySwapchainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const;

    /**
     * Retrieves queue handles after logical device creation.
     * 
     * After creating the logical device, we need to get handles to the
     * actual queues we can submit commands to.
     */
    void retrieveQueueHandles();

    /**
     * Queries and stores device properties, features, and memory information.
     * 
     * This information is useful for making decisions about resource
     * allocation and feature usage throughout the application.
     * 
     * @param device Physical device to query
     */
    void queryDeviceInfo(VkPhysicalDevice device);
};

} // namespace VulkanGameEngine