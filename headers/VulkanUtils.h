#pragma once

#include "Common.h"
#include "Logger.h"

namespace VulkanGameEngine {
namespace VulkanUtils {

    /**
     * @brief Comprehensive Vulkan result checker with detailed error messages
     * 
     * This function provides educational error messages that help developers
     * understand what went wrong during Vulkan operations. It includes both
     * the Vulkan error code and a human-readable description.
     * 
     * @param result The VkResult to check
     * @param operation Description of the operation that was attempted
     * @throws std::runtime_error if result is not VK_SUCCESS
     */
    void checkVulkanResult(VkResult result, const std::string& operation);

    /**
     * @brief Convert VkResult enum to human-readable string
     * 
     * Provides educational descriptions of Vulkan error codes to help
     * developers understand what each error means and potential causes.
     * 
     * @param result The VkResult to convert
     * @return Human-readable string description of the error
     */
    std::string vulkanResultToString(VkResult result);

    /**
     * @brief Read binary file contents into a vector
     * 
     * Utility function for loading SPIR-V shader bytecode from compiled
     * shader files. Handles file I/O errors gracefully with meaningful
     * error messages.
     * 
     * @param filename Path to the file to read
     * @return Vector containing the file contents as bytes
     * @throws std::runtime_error if file cannot be read
     */
    std::vector<char> readFile(const std::string& filename);

    /**
     * @brief Check if a file exists and is readable
     * 
     * Utility function to verify shader files exist before attempting
     * to load them, providing better error messages.
     * 
     * @param filename Path to check
     * @return true if file exists and is readable, false otherwise
     */
    bool fileExists(const std::string& filename);

    /**
     * @brief Get file size in bytes
     * 
     * Helper function for pre-allocating buffers when reading files.
     * 
     * @param filename Path to the file
     * @return Size of the file in bytes
     * @throws std::runtime_error if file cannot be accessed
     */
    size_t getFileSize(const std::string& filename);

    /**
     * @brief Format Vulkan version number to readable string
     * 
     * Converts Vulkan API version numbers to human-readable format
     * for logging and debugging purposes.
     * 
     * @param version Vulkan version number (from VK_MAKE_VERSION)
     * @return Formatted version string (e.g., "1.2.3")
     */
    std::string formatVulkanVersion(uint32_t version);

    /**
     * @brief Log Vulkan object creation for debugging
     * 
     * Educational logging function that helps track Vulkan object
     * lifecycle during development and debugging.
     * 
     * @param objectType Type of Vulkan object being created
     * @param objectName Optional name for the object
     */
    void logObjectCreation(const std::string& objectType, const std::string& objectName = "");

    /**
     * @brief Log Vulkan object destruction for debugging
     * 
     * Educational logging function that helps track Vulkan object
     * cleanup during development and debugging.
     * 
     * @param objectType Type of Vulkan object being destroyed
     * @param objectName Optional name for the object
     */
    void logObjectDestruction(const std::string& objectType, const std::string& objectName = "");

} // namespace VulkanUtils
} // namespace VulkanGameEngine

// Enhanced error checking macro with more context
#define VK_CHECK_RESULT(result, operation) \
    VulkanGameEngine::VulkanUtils::checkVulkanResult(result, operation)

// Macro for checking Vulkan function calls directly
#define VK_CALL(call, operation) \
    VulkanGameEngine::VulkanUtils::checkVulkanResult(call, operation)