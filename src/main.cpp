#include "Common.h"
#include "VulkanEngine.h"
#include "VulkanUtils.h"
#include "Logger.h"
#include <chrono>
#include <thread>

using namespace VulkanGameEngine;

/**
 * Application class that manages the main game loop and SDL integration.
 * 
 * This class handles:
 * - SDL initialization and window creation
 * - Event handling (keyboard, mouse, window events)
 * - Main game loop with proper timing
 * - Integration with the VulkanEngine for rendering
 * - Graceful shutdown and cleanup
 */
class Application {
public:
    Application() 
        : m_window(nullptr)
        , m_running(false)
        , m_windowWidth(DEFAULT_WINDOW_WIDTH)
        , m_windowHeight(DEFAULT_WINDOW_HEIGHT) {
    }

    ~Application() {
        cleanup();
    }

    /**
     * Initializes SDL and creates the application window.
     * 
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize() {
        // Initialize logging system first
        Logger::getInstance().setLogLevel(Logger::Level::DEBUG);
        Logger::getInstance().setColorEnabled(true);
        Logger::getInstance().setTimestampEnabled(true);
        
        LOG_INFO("=== Vulkan 3D Game Engine ===", "App");
        LOG_INFO("Initializing application...", "App");
        
        // Initialize SDL
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            LOG_ERROR("Failed to initialize SDL: " + std::string(SDL_GetError()), "SDL");
            return false;
        }
        
        LOG_INFO("SDL initialized successfully", "SDL");
        
        // Check if Vulkan is supported
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            LOG_ERROR("Failed to load Vulkan library: " + std::string(SDL_GetError()), "SDL");
            return false;
        }
        
        LOG_INFO("Vulkan library loaded successfully", "SDL");
        
        // Create window
        m_window = SDL_CreateWindow(
            APPLICATION_NAME,
            m_windowWidth, m_windowHeight,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
        );
        
        if (!m_window) {
            LOG_ERROR("Failed to create SDL window: " + std::string(SDL_GetError()), "SDL");
            return false;
        }
        
        LOG_INFO("Window created: " + std::to_string(m_windowWidth) + "x" + std::to_string(m_windowHeight), "SDL");
        
        // Initialize Vulkan engine
        try {
            LOG_PERF_START(VulkanEngineInit);
            m_engine.initialize(m_window, m_windowWidth, m_windowHeight);
            LOG_PERF_END(VulkanEngineInit);
            LOG_INFO("Vulkan engine initialized successfully!", "Engine");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize Vulkan engine: " + std::string(e.what()), "Engine");
            return false;
        }
        
        m_running = true;
        return true;
    }

    /**
     * Runs the main application loop.
     * 
     * The main loop handles:
     * 1. Event processing (input, window events)
     * 2. Frame timing calculations
     * 3. Rendering via VulkanEngine
     * 4. Frame rate limiting (optional)
     */
    void run() {
        if (!m_running) {
            std::cerr << "Cannot run: application not initialized" << std::endl;
            return;
        }
        
        LOG_INFO("=== Starting Main Loop ===", "App");
        LOG_INFO("Controls:", "App");
        LOG_INFO("  - WASD: Move camera around the scene", "App");
        LOG_INFO("  - ESC: Exit application", "App");
        LOG_INFO("  - F11: Toggle fullscreen (not implemented)", "App");
        LOG_INFO("  - Resize window to test swapchain recreation", "App");
        LOG_INFO("  - Close window with X button to exit", "App");
        
        // Check if we're rendering character or fallback cube
        if (m_engine.getMainCharacter().isLoaded()) {
            LOG_INFO("Rendering 3D character model at origin (0,0,0)...", "App");
            LOG_INFO("Camera positioned at (10,5,10) looking at character", "App");
        } else {
            LOG_INFO("Rendering fallback cube at origin (0,0,0)...", "App");
            LOG_INFO("Camera positioned at (10,5,10) looking at cube", "App");
        }
        
        auto lastTime = std::chrono::high_resolution_clock::now();
        uint64_t frameCount = 0;
        float fpsTimer = 0.0f;
        uint32_t consecutiveErrors = 0;
        const uint32_t maxConsecutiveErrors = 5;
        
        LOG_INFO("Entering render loop - window should now be visible!", "App");
        
        while (m_running) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            // Process events first - this is critical for keeping window responsive
            processEvents();
            
            // Only exit if user explicitly requested it
            if (!m_running) {
                LOG_INFO("Exit requested by user", "App");
                break;
            }
            
            // Handle camera movement with WASD keys
            handleCameraMovement(deltaTime);
            
            // Render frame with error handling
            try {
                m_engine.render();
                frameCount++;
                consecutiveErrors = 0; // Reset error counter on successful render
                
                // Update FPS counter every second
                fpsTimer += deltaTime;
                if (fpsTimer >= 1.0f) {
                    float fps, frameTime;
                    m_engine.getFrameStats(fps, frameTime);
                    
                    LOG_INFO("FPS: " + std::to_string(static_cast<int>(fps)) + 
                            " | Frame Time: " + std::to_string(frameTime) + "ms" +
                            " | Total Frames: " + std::to_string(frameCount), "Performance");
                    
                    fpsTimer = 0.0f;
                }
                
            } catch (const std::exception& e) {
                consecutiveErrors++;
                LOG_ERROR("Render error #" + std::to_string(consecutiveErrors) + ": " + std::string(e.what()), "Render");
                
                // Only exit after multiple consecutive errors to avoid crashes on temporary issues
                if (consecutiveErrors >= maxConsecutiveErrors) {
                    LOG_ERROR("Too many consecutive render errors. Exiting to prevent infinite loop.", "App");
                    m_running = false;
                } else {
                    LOG_WARN("Continuing despite render error. Attempt " + std::to_string(consecutiveErrors) + " of " + std::to_string(maxConsecutiveErrors), "App");
                    // Small delay to prevent tight error loop - ONLY on errors
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            // REMOVED: Artificial frame rate limiting that was causing ~1 FPS
            // Let Vulkan handle its own frame synchronization via VSync and frame timing
            // The VSync mechanism will naturally limit framerate to monitor refresh rate
        }
        
        LOG_INFO("Main loop ended. Total frames rendered: " + std::to_string(frameCount), "App");
    }

    /**
     * Cleans up all resources and shuts down SDL.
     */
    void cleanup() {
        LOG_INFO("Cleaning up application...", "App");
        
        // Clean up Vulkan engine first
        m_engine.cleanup();
        
        // Clean up SDL resources
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            VulkanUtils::logObjectDestruction("SDL_Window");
        }
        
        SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        
        LOG_INFO("Application cleanup completed", "App");
    }

private:
    SDL_Window* m_window;
    VulkanEngine m_engine;
    bool m_running;
    uint32_t m_windowWidth;
    uint32_t m_windowHeight;

    /**
     * Processes SDL events (keyboard, mouse, window events).
     */
    void processEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    LOG_INFO("Quit event received - user closed window", "Input");
                    m_running = false;
                    break;
                
                case SDL_EVENT_KEY_DOWN:
                    handleKeyDown(event.key);
                    break;
                
                case SDL_EVENT_WINDOW_RESIZED:
                    handleWindowResize(event.window);
                    break;
                
                case SDL_EVENT_WINDOW_MINIMIZED:
                    LOG_DEBUG("Window minimized", "Window");
                    break;
                
                case SDL_EVENT_WINDOW_RESTORED:
                    LOG_DEBUG("Window restored", "Window");
                    break;
                
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    LOG_INFO("Window close requested", "Input");
                    m_running = false;
                    break;
                
                default:
                    // Don't log every event to avoid spam, but count them
                    break;
            }
        }
    }

    /**
     * Handles WASD camera movement based on current keyboard state.
     */
    void handleCameraMovement(float deltaTime) {
        const bool* keyboardState = SDL_GetKeyboardState(nullptr);
        
        float forward = 0.0f;
        float right = 0.0f;
        
        // W/S for forward/backward movement
        if (keyboardState[SDL_SCANCODE_W]) {
            forward += 1.0f;
        }
        if (keyboardState[SDL_SCANCODE_S]) {
            forward -= 1.0f;
        }
        
        // A/D for left/right movement
        if (keyboardState[SDL_SCANCODE_A]) {
            right -= 1.0f;
        }
        if (keyboardState[SDL_SCANCODE_D]) {
            right += 1.0f;
        }
        
        // Apply camera movement if any keys are pressed
        if (forward != 0.0f || right != 0.0f) {
            m_engine.moveCamera(forward, right, deltaTime);
        }
    }

    /**
     * Handles keyboard input events.
     */
    void handleKeyDown(const SDL_KeyboardEvent& keyEvent) {
        switch (keyEvent.key) {
            case SDLK_ESCAPE:
                LOG_INFO("ESC pressed - exiting application", "Input");
                m_running = false;
                break;
            
            case SDLK_F11:
                LOG_DEBUG("F11 pressed - fullscreen toggle not implemented", "Input");
                break;
            
            default:
                // Ignore other keys
                break;
        }
    }

    /**
     * Handles window resize events.
     */
    void handleWindowResize(const SDL_WindowEvent& windowEvent) {
        if (windowEvent.type == SDL_EVENT_WINDOW_RESIZED) {
            m_windowWidth = static_cast<uint32_t>(windowEvent.data1);
            m_windowHeight = static_cast<uint32_t>(windowEvent.data2);
            
            LOG_INFO("Window resized to " + std::to_string(m_windowWidth) + "x" + std::to_string(m_windowHeight), "Window");
            
            // Handle resize in Vulkan engine
            try {
                LOG_PERF_START(SwapchainRecreation);
                m_engine.handleResize(m_windowWidth, m_windowHeight);
                LOG_PERF_END(SwapchainRecreation);
                LOG_DEBUG("Swapchain recreated for new window size", "Vulkan");
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to handle window resize: " + std::string(e.what()), "Window");
                m_running = false;
            }
        }
    }
};

/**
 * Main entry point for the Vulkan 3D Game Engine.
 * 
 * This function:
 * 1. Creates and initializes the application
 * 2. Runs the main game loop
 * 3. Handles any top-level exceptions
 * 4. Ensures proper cleanup on exit
 */
int main() {
    Application app;
    
    try {
        // Initialize the application
        if (!app.initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }
        
        // Run the main loop
        app.run();
        
        LOG_INFO("Application exited normally", "App");
        return 0;
        
    } catch (const std::exception& e) {
        LOG_FATAL("Unhandled exception in main: " + std::string(e.what()), "App");
        return 1;
    } catch (...) {
        LOG_FATAL("Unknown exception in main", "App");
        return 1;
    }
}

