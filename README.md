# Echo Protocol

A Vulkan + SDL3 C++ example project demonstrating basic graphics API integration and modern C++ development practices.

## Features

- SDL3 integration for cross-platform window management
- Vulkan API setup with proper initialization and cleanup
- Modern C++ (C++17) architecture with clean separation of concerns
- CMake build system configuration
- Validation layer support for development debugging
- Proper resource management and error handling

## Dependencies

- **SDL3**: Cross-platform development library for window management, input, and audio
- **Vulkan SDK**: Graphics and compute API for high-performance rendering
- **GLM**: Mathematics library for graphics programming (vec3, mat4, etc.)
- **CMake**: Build system generator (version 3.15 or higher)

## Requirements

- C++17 compatible compiler (MSVC, GCC, or Clang)
- Vulkan SDK installed on your system
- CMake 3.15 or higher
- Windows, Linux, or macOS

## Building

1. Clone the repository
2. Ensure Vulkan SDK is installed and accessible
3. Create a build directory:
   ```
   mkdir build
   cd build
   ```
4. Generate build files with CMake:
   ```
   cmake ..
   ```
5. Build the project:
   ```
   cmake --build .
   ```

## Project Structure

```
Echo Protocol/
├── src/                    # Source files
│   └── main.cpp           # Main application entry point
├── headers/               # Header files
│   ├── Common.h          # Shared definitions and includes
│   └── VulkanPipeline.h  # Vulkan pipeline management
├── libs/                  # Third-party libraries
│   └── SDL3/             # SDL3 library files
├── CMakeLists.txt        # CMake configuration
└── README.md            # This file
```

## Usage

Run the executable to see a basic SDL3 window with Vulkan initialization. The application demonstrates:

- SDL3 window creation with Vulkan support
- Vulkan instance creation with extension loading
- Vulkan surface creation for rendering
- Basic event handling and application lifecycle
- Proper cleanup of graphics resources

## Development

This project uses validation layers in debug builds to help catch Vulkan API usage errors. Make sure to install the Vulkan SDK with validation layers for the best development experience.

## Advice
For production game development, it is recommended to utilize established game engines rather than building from scratch. This project serves as an educational resource for understanding Vulkan and SDL3 integration patterns. Developing a complete 3D game from the ground up, particularly as a solo developer, may require several years to achieve a playable state. Additionally, low-level graphics programming introduces significant debugging complexity and potential system instability. Modern game engines provide mature, battle-tested frameworks that represent decades of collective development effort and industry expertise.🙂

## Contribution
Feel free to extend the project by adding rendering code, shaders, and more complex graphics features.