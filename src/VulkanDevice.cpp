#include "../headers/VulkanDevice.h"
#include "../headers/VulkanUtils.h"
#include <map>
#include <iostream>

namespace VulkanGameEngine {

VulkanDevice::VulkanDevice() 
    : m_physicalDevice(VK_NULL_HANDLE)
    , m_logicalDevice(VK_NULL_HANDLE)
    , m_graphicsQueue(VK_NULL_HANDLE)
    , m_presentQueue(VK_NULL_HANDLE)
    , m_computeQueue(VK_NULL_HANDLE)
    , m_transferQueue(VK_NULL_HANDLE) {
    
    VulkanUtils::logObjectCreation("VulkanDevice", "Device Manager");
}

VulkanDevice::~VulkanDevice() {
    cleanup();
}

void VulkanDevice::create(VkInstance instance, VkSurfaceKHR surface) {
    std::cout << "\n=== VulkanDevice: Starting Device Selection and Creation ===\n";
    
    // Step 1: Select the best physical device
    // Physical devices represent actual GPUs or graphics hardware in the system
    selectPhysicalDevice(instance, surface);
    
    // Step 2: Create logical device with required queues and extensions
    // The logical device is our software interface to the physical device
    createLogicalDevice(surface);
    
    // Step 3: Retrieve queue handles for command submission
    // Queues are how we submit work to the GPU
    retrieveQueueHandles();
    
    std::cout << "VulkanDevice: Device creation completed successfully\n";
    std::cout << "=== Device Setup Complete ===\n\n";
}

void VulkanDevice::cleanup() {
    if (m_logicalDevice != VK_NULL_HANDLE) {
        std::cout << "VulkanDevice: Destroying logical device...\n";
        
        // Wait for all operations to complete before destroying the device
        // This ensures we don't destroy resources that are still in use
        vkDeviceWaitIdle(m_logicalDevice);
        
        // Destroy the logical device
        // Note: Queue handles are implicitly destroyed with the device
        vkDestroyDevice(m_logicalDevice, nullptr);
        m_logicalDevice = VK_NULL_HANDLE;
        
        // Reset queue handles
        m_graphicsQueue = VK_NULL_HANDLE;
        m_presentQueue = VK_NULL_HANDLE;
        m_computeQueue = VK_NULL_HANDLE;
        m_transferQueue = VK_NULL_HANDLE;
        
        VulkanUtils::logObjectDestruction("VulkanDevice", "Logical Device");
    }
    
    // Note: Physical device handles don't need to be destroyed
    // They are owned by the Vulkan instance
    m_physicalDevice = VK_NULL_HANDLE;
}

VulkanDevice::SwapchainSupportDetails VulkanDevice::querySwapchainSupport(VkSurfaceKHR surface) const {
    return querySwapchainSupportDetails(m_physicalDevice, surface);
}

void VulkanDevice::selectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    std::cout << "VulkanDevice: Enumerating physical devices...\n";
    
    // Get the number of available physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }
    
    std::cout << "VulkanDevice: Found " << deviceCount << " physical device(s)\n";
    
    // Get all available physical devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    
    // Score each device and select the best one
    std::multimap<uint32_t, VkPhysicalDevice> candidates;
    
    for (const auto& device : devices) {
        uint32_t score = scorePhysicalDevice(device, surface);
        candidates.insert(std::make_pair(score, device));
        
        // Log device information for educational purposes
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        
        std::cout << "VulkanDevice: Device '" << deviceProperties.deviceName 
                  << "' scored " << score << " points\n";
        std::cout << "  - Device Type: ";
        switch (deviceProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout << "Discrete GPU (dedicated graphics card)\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout << "Integrated GPU (built into CPU)\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout << "Virtual GPU (virtualized environment)\n";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout << "CPU (software rendering)\n";
                break;
            default:
                std::cout << "Other/Unknown\n";
                break;
        }
        std::cout << "  - API Version: " << VulkanUtils::formatVulkanVersion(deviceProperties.apiVersion) << "\n";
        std::cout << "  - Driver Version: " << deviceProperties.driverVersion << "\n";
    }
    
    // Select the device with the highest score
    // candidates is sorted by score (key), so rbegin() gives us the highest score
    if (candidates.rbegin()->first > 0) {
        m_physicalDevice = candidates.rbegin()->second;
        
        // Query and store device information
        queryDeviceInfo(m_physicalDevice);
        
        std::cout << "VulkanDevice: Selected device: " << m_deviceProperties.deviceName << "\n";
        std::cout << "VulkanDevice: Device selection completed successfully\n";
    } else {
        throw std::runtime_error("Failed to find a suitable GPU! No devices met minimum requirements.");
    }
}

void VulkanDevice::createLogicalDevice(VkSurfaceKHR surface) {
    std::cout << "VulkanDevice: Creating logical device...\n";
    
    // Find queue families for the selected physical device
    m_queueFamilyIndices = findQueueFamilies(m_physicalDevice, surface);
    
    // Create queue create info structures
    // We need to specify which queues we want to create and their priorities
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = m_queueFamilyIndices.getUniqueQueueFamilies();
    
    // Queue priority affects scheduling (0.0 to 1.0, where 1.0 is highest priority)
    float queuePriority = 1.0f;
    
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;  // We only need one queue per family for now
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
        
        std::cout << "VulkanDevice: Requesting queue from family " << queueFamily << "\n";
    }
    
    // Specify device features we want to use
    // For now, we'll use default features (all disabled)
    // In the future, we might enable features like:
    // - samplerAnisotropy for better texture filtering
    // - geometryShader for advanced rendering techniques
    // - tessellationShader for detailed surface subdivision
    VkPhysicalDeviceFeatures deviceFeatures{};
    
    // Create the logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    
    // Enable required device extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
    
    std::cout << "VulkanDevice: Enabling " << m_deviceExtensions.size() << " device extension(s):\n";
    for (const auto& extension : m_deviceExtensions) {
        std::cout << "  - " << extension << "\n";
    }
    
    // Enable validation layers for the device (if available)
    // Note: Device-specific validation layers are deprecated in newer Vulkan versions
    // but we include this for compatibility with older implementations
    if (ENABLE_VALIDATION_LAYERS) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    // Create the logical device
    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice);
    VK_CHECK(result, "Failed to create logical device");
    
    std::cout << "VulkanDevice: Logical device created successfully\n";
    VulkanUtils::logObjectCreation("VkDevice", "Logical Device");
}

uint32_t VulkanDevice::scorePhysicalDevice(VkPhysicalDevice device, VkSurfaceKHR surface) {
    // First check if device meets minimum requirements
    if (!isDeviceSuitable(device, surface)) {
        return 0;  // Device is not suitable at all
    }
    
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    uint32_t score = 0;
    
    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
        std::cout << "  - Discrete GPU bonus: +1000 points\n";
    } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        score += 500;
        std::cout << "  - Integrated GPU bonus: +500 points\n";
    }
    
    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D / 1000;
    
    // Geometry shaders enable advanced rendering techniques
    if (deviceFeatures.geometryShader) {
        score += 100;
        std::cout << "  - Geometry shader support: +100 points\n";
    }
    
    // Tessellation shaders allow for detailed surface subdivision
    if (deviceFeatures.tessellationShader) {
        score += 50;
        std::cout << "  - Tessellation shader support: +50 points\n";
    }
    
    // Anisotropic filtering improves texture quality
    if (deviceFeatures.samplerAnisotropy) {
        score += 25;
        std::cout << "  - Anisotropic filtering support: +25 points\n";
    }
    
    // More memory is generally better for complex scenes
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
    
    uint64_t totalMemory = 0;
    for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++) {
        if (memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            totalMemory += memProperties.memoryHeaps[i].size;
        }
    }
    
    // Add points based on available VRAM (in GB)
    uint32_t memoryGB = static_cast<uint32_t>(totalMemory / (1024 * 1024 * 1024));
    score += memoryGB * 10;
    std::cout << "  - Device memory (" << memoryGB << " GB): +" << (memoryGB * 10) << " points\n";
    
    return score;
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    // Check if device supports required queue families
    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if (!indices.isComplete()) {
        std::cout << "  - Missing required queue families\n";
        return false;
    }
    
    // Check if device supports required extensions
    if (!checkDeviceExtensionSupport(device)) {
        std::cout << "  - Missing required extensions\n";
        return false;
    }
    
    // Check if swapchain support is adequate
    SwapchainSupportDetails swapchainSupport = querySwapchainSupportDetails(device, surface);
    if (!swapchainSupport.isAdequate()) {
        std::cout << "  - Inadequate swapchain support\n";
        return false;
    }
    
    return true;
}

VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    
    // Get the number of queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    
    // Get queue family properties
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    // Find queue families that support the operations we need
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const auto& queueFamily = queueFamilies[i];
        
        // Check for graphics support
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        // Check for compute support
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }
        
        // Check for transfer support
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            indices.transferFamily = i;
        }
        
        // Check for presentation support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        
        // Early exit if we found all required families
        if (indices.isComplete()) {
            break;
        }
    }
    
    return indices;
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // Get available extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    
    // Check if all required extensions are available
    std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    // If requiredExtensions is empty, all extensions were found
    return requiredExtensions.empty();
}

VulkanDevice::SwapchainSupportDetails VulkanDevice::querySwapchainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    SwapchainSupportDetails details;
    
    // Get surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    // Get surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    
    // Get present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    
    return details;
}

void VulkanDevice::retrieveQueueHandles() {
    std::cout << "VulkanDevice: Retrieving queue handles...\n";
    
    // Get graphics queue handle
    // Queue index 0 because we only requested one queue per family
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    std::cout << "VulkanDevice: Graphics queue retrieved from family " 
              << m_queueFamilyIndices.graphicsFamily.value() << "\n";
    
    // Get present queue handle
    vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
    std::cout << "VulkanDevice: Present queue retrieved from family " 
              << m_queueFamilyIndices.presentFamily.value() << "\n";
    
    // Get compute queue handle (if available)
    if (m_queueFamilyIndices.computeFamily.has_value()) {
        vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue);
        std::cout << "VulkanDevice: Compute queue retrieved from family " 
                  << m_queueFamilyIndices.computeFamily.value() << "\n";
    }
    
    // Get transfer queue handle (if available and different from graphics)
    if (m_queueFamilyIndices.transferFamily.has_value() && 
        m_queueFamilyIndices.transferFamily.value() != m_queueFamilyIndices.graphicsFamily.value()) {
        vkGetDeviceQueue(m_logicalDevice, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
        std::cout << "VulkanDevice: Dedicated transfer queue retrieved from family " 
                  << m_queueFamilyIndices.transferFamily.value() << "\n";
    } else {
        // Use graphics queue for transfer operations if no dedicated transfer queue
        m_transferQueue = m_graphicsQueue;
        std::cout << "VulkanDevice: Using graphics queue for transfer operations\n";
    }
    
    std::cout << "VulkanDevice: All queue handles retrieved successfully\n";
}

void VulkanDevice::queryDeviceInfo(VkPhysicalDevice device) {
    // Get device properties (name, type, limits, etc.)
    vkGetPhysicalDeviceProperties(device, &m_deviceProperties);
    
    // Get device features (optional capabilities)
    vkGetPhysicalDeviceFeatures(device, &m_deviceFeatures);
    
    // Get memory properties (available memory types and heaps)
    vkGetPhysicalDeviceMemoryProperties(device, &m_memoryProperties);
    
    std::cout << "VulkanDevice: Device information retrieved:\n";
    std::cout << "  - Name: " << m_deviceProperties.deviceName << "\n";
    std::cout << "  - Vendor ID: 0x" << std::hex << m_deviceProperties.vendorID << std::dec << "\n";
    std::cout << "  - Device ID: 0x" << std::hex << m_deviceProperties.deviceID << std::dec << "\n";
    std::cout << "  - API Version: " << VulkanUtils::formatVulkanVersion(m_deviceProperties.apiVersion) << "\n";
    std::cout << "  - Driver Version: " << m_deviceProperties.driverVersion << "\n";
    
    // Log some important limits
    std::cout << "  - Max Texture Size: " << m_deviceProperties.limits.maxImageDimension2D << "x" 
              << m_deviceProperties.limits.maxImageDimension2D << "\n";
    std::cout << "  - Max Uniform Buffer Size: " << m_deviceProperties.limits.maxUniformBufferRange << " bytes\n";
    std::cout << "  - Max Push Constants Size: " << m_deviceProperties.limits.maxPushConstantsSize << " bytes\n";
    
    // Log memory information
    std::cout << "  - Memory Heaps: " << m_memoryProperties.memoryHeapCount << "\n";
    for (uint32_t i = 0; i < m_memoryProperties.memoryHeapCount; i++) {
        const auto& heap = m_memoryProperties.memoryHeaps[i];
        std::cout << "    Heap " << i << ": " << (heap.size / (1024 * 1024)) << " MB";
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            std::cout << " (Device Local)";
        }
        std::cout << "\n";
    }
}

} // namespace VulkanGameEngine