#include "../headers/VulkanSynchronization.h"
#include "../headers/VulkanUtils.h"

namespace VulkanGameEngine {

VulkanSynchronization::VulkanSynchronization()
    : m_device(VK_NULL_HANDLE)
    , m_maxFramesInFlight(0) {
    
    VulkanUtils::logObjectCreation("VulkanSynchronization", "Initialized");
}

VulkanSynchronization::~VulkanSynchronization() {
    cleanup();
}

void VulkanSynchronization::create(VkDevice device, uint32_t maxFramesInFlight) {
    m_device = device;
    m_maxFramesInFlight = maxFramesInFlight;
    
    VulkanUtils::logObjectCreation("VulkanSynchronization", 
        "Creating synchronization objects for " + std::to_string(maxFramesInFlight) + " frames in flight");
    
    // Resize the frame sync objects vector
    m_frameSyncObjects.resize(maxFramesInFlight);
    
    // Create synchronization objects for each frame
    for (uint32_t i = 0; i < maxFramesInFlight; i++) {
        std::string frameStr = "Frame " + std::to_string(i);
        
        // Create semaphores for this frame
        m_frameSyncObjects[i].imageAvailableSemaphore = 
            createSemaphoreInternal(device, frameStr + " Image Available");
        
        m_frameSyncObjects[i].renderFinishedSemaphore = 
            createSemaphoreInternal(device, frameStr + " Render Finished");
        
        // Create fence for this frame (start in signaled state so first frame doesn't wait)
        m_frameSyncObjects[i].inFlightFence = 
            createFenceInternal(device, true, frameStr + " In Flight");
    }
    
    std::cout << "Successfully created synchronization objects for " << maxFramesInFlight 
              << " frames in flight\n";
    std::cout << "  - Total semaphores: " << (maxFramesInFlight * 2) << "\n";
    std::cout << "  - Total fences: " << maxFramesInFlight << "\n";
}

bool VulkanSynchronization::waitForFrame(uint32_t frameIndex, uint64_t timeout) {
    if (frameIndex >= m_frameSyncObjects.size()) {
        throw std::runtime_error("Frame index out of range");
    }
    
    VkFence fence = m_frameSyncObjects[frameIndex].inFlightFence;
    VkResult result = vkWaitForFences(m_device, 1, &fence, VK_TRUE, timeout);
    
    if (result == VK_SUCCESS) {
        return true;
    } else if (result == VK_TIMEOUT) {
        std::cout << "Warning: Timeout waiting for frame " << frameIndex << "\n";
        return false;
    } else {
        VK_CHECK(result, "Failed to wait for frame fence");
        return false;
    }
}

void VulkanSynchronization::resetFrameFence(uint32_t frameIndex) {
    if (frameIndex >= m_frameSyncObjects.size()) {
        throw std::runtime_error("Frame index out of range");
    }
    
    VkFence fence = m_frameSyncObjects[frameIndex].inFlightFence;
    VK_CHECK(vkResetFences(m_device, 1, &fence), "Failed to reset frame fence");
}

bool VulkanSynchronization::waitForAllFrames(uint64_t timeout) {
    if (m_frameSyncObjects.empty()) {
        return true;
    }
    
    VulkanUtils::logObjectCreation("VulkanSynchronization", "Waiting for all frames to complete");
    
    // Collect all in-flight fences
    std::vector<VkFence> allFences;
    allFences.reserve(m_frameSyncObjects.size());
    
    for (const auto& frameSync : m_frameSyncObjects) {
        allFences.push_back(frameSync.inFlightFence);
    }
    
    VkResult result = vkWaitForFences(m_device, static_cast<uint32_t>(allFences.size()),
                                     allFences.data(), VK_TRUE, timeout);
    
    if (result == VK_SUCCESS) {
        std::cout << "All frames completed successfully\n";
        return true;
    } else if (result == VK_TIMEOUT) {
        std::cout << "Warning: Timeout waiting for all frames\n";
        return false;
    } else {
        VK_CHECK(result, "Failed to wait for all frame fences");
        return false;
    }
}

VkSemaphore VulkanSynchronization::createSemaphore(VkDevice device) {
    VkSemaphore semaphore = createSemaphoreInternal(device, "Manual Semaphore");
    m_semaphores.push_back(semaphore);
    return semaphore;
}

VkFence VulkanSynchronization::createFence(VkDevice device, bool signaled) {
    VkFence fence = createFenceInternal(device, signaled, "Manual Fence");
    m_fences.push_back(fence);
    return fence;
}

void VulkanSynchronization::destroySemaphore(VkDevice device, VkSemaphore semaphore) {
    if (semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, semaphore, nullptr);
        
        // Remove from tracking list
        auto it = std::find(m_semaphores.begin(), m_semaphores.end(), semaphore);
        if (it != m_semaphores.end()) {
            m_semaphores.erase(it);
        }
        
        VulkanUtils::logObjectDestruction("VkSemaphore");
    }
}

void VulkanSynchronization::destroyFence(VkDevice device, VkFence fence) {
    if (fence != VK_NULL_HANDLE) {
        vkDestroyFence(device, fence, nullptr);
        
        // Remove from tracking list
        auto it = std::find(m_fences.begin(), m_fences.end(), fence);
        if (it != m_fences.end()) {
            m_fences.erase(it);
        }
        
        VulkanUtils::logObjectDestruction("VkFence");
    }
}

void VulkanSynchronization::submitCommandBuffers(VkQueue queue,
                                                 const std::vector<VkCommandBuffer>& commandBuffers,
                                                 const std::vector<VkSemaphore>& waitSemaphores,
                                                 const std::vector<VkPipelineStageFlags>& waitStages,
                                                 const std::vector<VkSemaphore>& signalSemaphores,
                                                 VkFence fence) {
    
    if (waitSemaphores.size() != waitStages.size()) {
        throw std::runtime_error("Number of wait semaphores must match number of wait stages");
    }
    
    // Configure submit info
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    // Wait semaphores and stages
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    
    // Command buffers to execute
    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    
    // Signal semaphores
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.empty() ? nullptr : signalSemaphores.data();
    
    // Submit to queue
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence),
             "Failed to submit command buffers to queue");
}

VkResult VulkanSynchronization::presentImage(VkQueue presentQueue, VkSwapchainKHR swapchain,
                                            uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores) {
    
    // Configure present info
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    // Wait for these semaphores before presenting
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    presentInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    
    // Swapchain and image to present
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional per-swapchain results
    
    // Present the image
    return vkQueuePresentKHR(presentQueue, &presentInfo);
}

VkResult VulkanSynchronization::acquireNextImage(VkDevice device, VkSwapchainKHR swapchain,
                                                uint64_t timeout, VkSemaphore semaphore,
                                                VkFence fence, uint32_t* imageIndex) {
    
    return vkAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, imageIndex);
}

void VulkanSynchronization::cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        // Wait for all operations to complete before cleanup
        waitForAllFrames();
        
        // Clean up frame synchronization objects
        for (auto& frameSync : m_frameSyncObjects) {
            if (frameSync.imageAvailableSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_device, frameSync.imageAvailableSemaphore, nullptr);
                frameSync.imageAvailableSemaphore = VK_NULL_HANDLE;
            }
            
            if (frameSync.renderFinishedSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_device, frameSync.renderFinishedSemaphore, nullptr);
                frameSync.renderFinishedSemaphore = VK_NULL_HANDLE;
            }
            
            if (frameSync.inFlightFence != VK_NULL_HANDLE) {
                vkDestroyFence(m_device, frameSync.inFlightFence, nullptr);
                frameSync.inFlightFence = VK_NULL_HANDLE;
            }
        }
        
        if (!m_frameSyncObjects.empty()) {
            VulkanUtils::logObjectDestruction("FrameSyncObjects", 
                "Destroyed " + std::to_string(m_frameSyncObjects.size()) + " frame sync objects");
            m_frameSyncObjects.clear();
        }
        
        // Clean up manually created semaphores
        for (VkSemaphore semaphore : m_semaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_device, semaphore, nullptr);
            }
        }
        if (!m_semaphores.empty()) {
            VulkanUtils::logObjectDestruction("ManualSemaphores", 
                "Destroyed " + std::to_string(m_semaphores.size()) + " manual semaphores");
            m_semaphores.clear();
        }
        
        // Clean up manually created fences
        for (VkFence fence : m_fences) {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(m_device, fence, nullptr);
            }
        }
        if (!m_fences.empty()) {
            VulkanUtils::logObjectDestruction("ManualFences", 
                "Destroyed " + std::to_string(m_fences.size()) + " manual fences");
            m_fences.clear();
        }
        
        m_device = VK_NULL_HANDLE;
        m_maxFramesInFlight = 0;
    }
}

const VulkanSynchronization::FrameSyncObjects& VulkanSynchronization::getFrameSyncObjects(uint32_t frameIndex) const {
    if (frameIndex >= m_frameSyncObjects.size()) {
        throw std::runtime_error("Frame index out of range");
    }
    return m_frameSyncObjects[frameIndex];
}

VkSemaphore VulkanSynchronization::getImageAvailableSemaphore(uint32_t frameIndex) const {
    return getFrameSyncObjects(frameIndex).imageAvailableSemaphore;
}

VkSemaphore VulkanSynchronization::getRenderFinishedSemaphore(uint32_t frameIndex) const {
    return getFrameSyncObjects(frameIndex).renderFinishedSemaphore;
}

VkFence VulkanSynchronization::getInFlightFence(uint32_t frameIndex) const {
    return getFrameSyncObjects(frameIndex).inFlightFence;
}

VkSemaphore VulkanSynchronization::createSemaphoreInternal(VkDevice device, const std::string& name) {
    VulkanUtils::logObjectCreation("VkSemaphore", name);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // No flags needed for basic semaphores
    
    VkSemaphore semaphore;
    VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore),
             "Failed to create semaphore: " + name);
    
    return semaphore;
}

VkFence VulkanSynchronization::createFenceInternal(VkDevice device, bool signaled, const std::string& name) {
    VulkanUtils::logObjectCreation("VkFence", name + (signaled ? " (signaled)" : " (unsignaled)"));
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    
    // Create fence in signaled state if requested
    if (signaled) {
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }
    
    VkFence fence;
    VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence),
             "Failed to create fence: " + name);
    
    return fence;
}

// Utility functions implementation
namespace SynchronizationUtils {

VkSubmitInfo createSubmitInfo(VkCommandBuffer commandBuffer,
                             VkSemaphore waitSemaphore,
                             VkPipelineStageFlags waitStage,
                             VkSemaphore signalSemaphore) {
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    // Command buffer to submit
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    // Wait semaphore (if provided)
    if (waitSemaphore != VK_NULL_HANDLE) {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
    }
    
    // Signal semaphore (if provided)
    if (signalSemaphore != VK_NULL_HANDLE) {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;
    }
    
    return submitInfo;
}

VkPresentInfoKHR createPresentInfo(VkSwapchainKHR swapchain, uint32_t imageIndex,
                                  VkSemaphore waitSemaphore) {
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    
    // Wait semaphore (if provided)
    if (waitSemaphore != VK_NULL_HANDLE) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &waitSemaphore;
    }
    
    // Swapchain and image to present
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    return presentInfo;
}

VkResult waitForFences(VkDevice device, const std::vector<VkFence>& fences,
                      bool waitAll, uint64_t timeout) {
    
    if (fences.empty()) {
        return VK_SUCCESS;
    }
    
    return vkWaitForFences(device, static_cast<uint32_t>(fences.size()),
                          fences.data(), waitAll ? VK_TRUE : VK_FALSE, timeout);
}

void resetFences(VkDevice device, const std::vector<VkFence>& fences) {
    if (!fences.empty()) {
        VK_CHECK(vkResetFences(device, static_cast<uint32_t>(fences.size()), fences.data()),
                 "Failed to reset fences");
    }
}

} // namespace SynchronizationUtils

} // namespace VulkanGameEngine