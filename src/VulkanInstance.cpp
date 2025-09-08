#include "../headers/VulkanInstance.h"
#include "../headers/VulkanUtils.h"
#include <iostream>
#include <set>
#include <cstring>

namespace VulkanGameEngine {

VulkanInstance::VulkanInstance() 
    : m_instance(VK_NULL_HANDLE)
    , m_debugMessenger(VK_NULL_HANDLE)
    , m_validationLayersEnabled(false) {
}

VulkanInstance::~VulkanInstance() {
    cleanup();
}

void VulkanInstance::create(const std::vector<const char*>& requiredExtensions, bool enableValidationLayers) {
    m_validationLayersEnabled = enableValidationLayers;

    // Check if validation layers are requested but not available
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    /*
     * VkApplicationInfo provides information about our application to the Vulkan driver.
     * While most of this information is optional, it can help drivers optimize for
     * specific applications or engines. The API version is particularly important
     * as it tells the driver which version of Vulkan we're targeting.
     */
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan 3D Game";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Educational Vulkan Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;  // Using Vulkan 1.0 for maximum compatibility

    /*
     * VkInstanceCreateInfo specifies the parameters for creating a Vulkan instance.
     * This includes which extensions and validation layers to enable.
     */
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Set up extensions
    // Extensions add functionality to Vulkan that's not part of the core API
    std::vector<const char*> extensions = requiredExtensions;
    
    if (enableValidationLayers) {
        // Add the debug utils extension so we can receive validation layer messages
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Set up validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();

        // Set up debug messenger for instance creation/destruction messages
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    /*
     * Create the Vulkan instance
     * This is where we actually connect to the Vulkan implementation.
     * The instance creation can fail for various reasons:
     * - Vulkan not supported on this system
     * - Requested extensions not available
     * - Validation layers not found
     */
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    VK_CHECK(result, "Failed to create Vulkan instance");

    std::cout << "✓ Vulkan instance created successfully" << std::endl;
    std::cout << "  - API Version: " << VK_VERSION_MAJOR(appInfo.apiVersion) 
              << "." << VK_VERSION_MINOR(appInfo.apiVersion) 
              << "." << VK_VERSION_PATCH(appInfo.apiVersion) << std::endl;
    std::cout << "  - Extensions enabled: " << extensions.size() << std::endl;
    
    if (enableValidationLayers) {
        std::cout << "  - Validation layers: ENABLED" << std::endl;
        setupDebugMessenger();
    } else {
        std::cout << "  - Validation layers: DISABLED" << std::endl;
    }
}

void VulkanInstance::cleanup() {
    /*
     * Cleanup must happen in reverse order of creation.
     * Debug messenger depends on the instance, so it must be destroyed first.
     */
    if (m_debugMessenger != VK_NULL_HANDLE) {
        // We need to load the function pointer since this is an extension function
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
        std::cout << "✓ Debug messenger destroyed" << std::endl;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
        std::cout << "✓ Vulkan instance destroyed" << std::endl;
    }
}

void VulkanInstance::setupDebugMessenger() {
    if (!m_validationLayersEnabled) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    /*
     * Create the debug messenger
     * Since this is an extension function, we need to load it manually
     */
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_instance, "vkCreateDebugUtilsMessengerEXT");
    
    if (func != nullptr) {
        VkResult result = func(m_instance, &createInfo, nullptr, &m_debugMessenger);
        VK_CHECK(result, "Failed to set up debug messenger");
        std::cout << "✓ Debug messenger set up successfully" << std::endl;
    } else {
        throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT function!");
    }
}

bool VulkanInstance::checkValidationLayerSupport() {
    /*
     * Query available validation layers
     * Validation layers are implemented as separate libraries that can be
     * loaded at runtime. We need to check if the ones we want are available.
     */
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::cout << "Available validation layers:" << std::endl;
    for (const auto& layer : availableLayers) {
        std::cout << "  - " << layer.layerName << ": " << layer.description << std::endl;
    }

    /*
     * Check if all requested validation layers are available
     */
    for (const char* layerName : m_validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            std::cerr << "Validation layer not found: " << layerName << std::endl;
            return false;
        }
    }

    return true;
}

void VulkanInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    
    /*
     * Specify which types of messages we want to receive
     * - VERBOSE: Diagnostic messages (very detailed)
     * - INFO: Informational messages
     * - WARNING: Potential problems that might cause issues
     * - ERROR: Invalid usage that will likely cause crashes or undefined behavior
     */
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    /*
     * Specify which types of messages we want to receive
     * - GENERAL: Unrelated to specification or performance
     * - VALIDATION: Violation of specification or possible mistake
     * - PERFORMANCE: Potential non-optimal use of Vulkan
     */
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional user data passed to callback
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    /*
     * Format the debug message in an educational way
     * This helps developers understand what's happening and why
     */
    std::string severityStr;
    std::string prefix;
    
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severityStr = "VERBOSE";
            prefix = "🔍 ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severityStr = "INFO";
            prefix = "ℹ️  ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severityStr = "WARNING";
            prefix = "⚠️  ";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severityStr = "ERROR";
            prefix = "❌ ";
            break;
        default:
            severityStr = "UNKNOWN";
            prefix = "❓ ";
            break;
    }

    std::string typeStr;
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        typeStr += "GENERAL ";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        typeStr += "VALIDATION ";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        typeStr += "PERFORMANCE ";
    }

    /*
     * Print the formatted message
     * We use std::cerr for errors and warnings, std::cout for info and verbose
     */
    auto& stream = (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? std::cerr : std::cout;
    
    stream << prefix << "[" << severityStr << "|" << typeStr << "] " 
           << pCallbackData->pMessage << std::endl;

    /*
     * Additional educational information for common issues
     */
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        stream << "  💡 This message indicates a potential issue with Vulkan API usage." << std::endl;
        stream << "     Review the validation layer documentation for more details." << std::endl;
    }

    /*
     * Return VK_FALSE to continue execution
     * Returning VK_TRUE would abort the Vulkan call that triggered this callback
     */
    return VK_FALSE;
}

} // namespace VulkanGameEngine