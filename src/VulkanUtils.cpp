#include "../headers/VulkanUtils.h"

namespace VulkanGameEngine {
namespace VulkanUtils {

    void checkVulkanResult(VkResult result, const std::string& operation) {
        if (result == VK_SUCCESS) {
            return;
        }

        std::string errorMessage = "Vulkan operation failed: " + operation;
        std::string errorDetails = "Error: " + vulkanResultToString(result) + " (" + std::to_string(result) + ")";
        
        // Log the error with details
        LOG_ERROR(errorMessage, "Vulkan");
        LOG_ERROR(errorDetails, "Vulkan");
        
        // Add educational context for common errors
        std::string suggestion;
        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                suggestion = "The system is out of host memory. Try closing other applications.";
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                suggestion = "The GPU is out of memory. Try reducing texture quality or buffer sizes.";
                break;
            case VK_ERROR_DEVICE_LOST:
                suggestion = "The GPU device was lost. This may be due to a driver crash or hardware issue.";
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                suggestion = "A required Vulkan extension is not available. Check your GPU driver version.";
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                suggestion = "A required validation layer is not installed. Install the Vulkan SDK.";
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                suggestion = "Your GPU driver doesn't support the requested Vulkan version. Update your drivers.";
                break;
            default:
                suggestion = "Check the Vulkan specification for details about this error code.";
                break;
        }
        
        LOG_WARN("Suggestion: " + suggestion, "Vulkan");
        
        throw std::runtime_error(errorMessage + "\n" + errorDetails + "\nSuggestion: " + suggestion);
    }

    std::string vulkanResultToString(VkResult result) {
        switch (result) {
            // Success codes
            case VK_SUCCESS: return "Success";
            case VK_NOT_READY: return "Not Ready";
            case VK_TIMEOUT: return "Timeout";
            case VK_EVENT_SET: return "Event Set";
            case VK_EVENT_RESET: return "Event Reset";
            case VK_INCOMPLETE: return "Incomplete";
            
            // Error codes
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "Out of Host Memory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "Out of Device Memory";
            case VK_ERROR_INITIALIZATION_FAILED: return "Initialization Failed";
            case VK_ERROR_DEVICE_LOST: return "Device Lost";
            case VK_ERROR_MEMORY_MAP_FAILED: return "Memory Map Failed";
            case VK_ERROR_LAYER_NOT_PRESENT: return "Layer Not Present";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "Extension Not Present";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "Feature Not Present";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "Incompatible Driver";
            case VK_ERROR_TOO_MANY_OBJECTS: return "Too Many Objects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "Format Not Supported";
            case VK_ERROR_FRAGMENTED_POOL: return "Fragmented Pool";
            case VK_ERROR_UNKNOWN: return "Unknown Error";
            
            // KHR extensions
            case VK_ERROR_OUT_OF_POOL_MEMORY: return "Out of Pool Memory";
            case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "Invalid External Handle";
            case VK_ERROR_FRAGMENTATION: return "Fragmentation";
            case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "Invalid Opaque Capture Address";
            
            // Surface-related errors
            case VK_ERROR_SURFACE_LOST_KHR: return "Surface Lost";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "Native Window In Use";
            case VK_SUBOPTIMAL_KHR: return "Suboptimal";
            case VK_ERROR_OUT_OF_DATE_KHR: return "Out of Date";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "Incompatible Display";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "Validation Failed";
            case VK_ERROR_INVALID_SHADER_NV: return "Invalid Shader";
            
            default: return "Unknown VkResult (" + std::to_string(result) + ")";
        }
    }

    std::vector<char> readFile(const std::string& filename) {
        // Check if file exists first for better error messages
        if (!fileExists(filename)) {
            throw std::runtime_error("Shader file not found: " + filename + 
                                   "\nMake sure the file path is correct and the file exists.");
        }

        // Open file at the end to get size immediately
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + filename + 
                                   "\nCheck file permissions and ensure the file is not locked by another process.");
        }

        // Get file size from current position (at end due to std::ios::ate)
        size_t fileSize = static_cast<size_t>(file.tellg());
        
        if (fileSize == 0) {
            throw std::runtime_error("Shader file is empty: " + filename + 
                                   "\nEnsure the shader has been compiled to SPIR-V bytecode.");
        }

        // Allocate buffer and read file
        std::vector<char> buffer(fileSize);
        file.seekg(0);  // Go back to beginning
        file.read(buffer.data(), fileSize);
        file.close();

        // Log successful shader loading for debugging
        LOG_DEBUG("Successfully loaded shader file: " + filename + " (" + std::to_string(fileSize) + " bytes)", "Shader");

        return buffer;
    }

    bool fileExists(const std::string& filename) {
        std::ifstream file(filename);
        return file.good();
    }

    size_t getFileSize(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        
        if (!file.is_open()) {
            throw std::runtime_error("Cannot access file to get size: " + filename);
        }
        
        return static_cast<size_t>(file.tellg());
    }

    std::string formatVulkanVersion(uint32_t version) {
        uint32_t major = VK_VERSION_MAJOR(version);
        uint32_t minor = VK_VERSION_MINOR(version);
        uint32_t patch = VK_VERSION_PATCH(version);
        
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    void logObjectCreation(const std::string& objectType, const std::string& objectName) {
        std::string message = "Creating " + objectType;
        if (!objectName.empty()) {
            message += " '" + objectName + "'";
        }
        LOG_DEBUG(message, "Object");
    }

    void logObjectDestruction(const std::string& objectType, const std::string& objectName) {
        std::string message = "Destroying " + objectType;
        if (!objectName.empty()) {
            message += " '" + objectName + "'";
        }
        LOG_DEBUG(message, "Object");
    }

} // namespace VulkanUtils
} // namespace VulkanGameEngine