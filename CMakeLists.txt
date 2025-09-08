cmake_minimum_required(VERSION 3.15)
project(game)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/headers)

# Path to SDL
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/libs/SDL3)
include_directories(${SDL3_DIR}/include)

# Vulkan (assumes Vulkan SDK is installed and environment variable is set)
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

# GLM mathematics library
find_package(glm QUIET)
if(NOT glm_FOUND)
    # If GLM is not found via find_package, try to use it as header-only
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        HINTS
            ${CMAKE_SOURCE_DIR}/libs/glm
            ${CMAKE_SOURCE_DIR}/external/glm
            ENV GLM_ROOT
        PATH_SUFFIXES include
    )
    
    if(GLM_INCLUDE_DIR)
        message(STATUS "Found GLM: ${GLM_INCLUDE_DIR}")
        include_directories(${GLM_INCLUDE_DIR})
    else()
        message(WARNING "GLM not found. Please install GLM or place it in libs/glm directory")
    endif()
else()
    message(STATUS "Found GLM via find_package")
endif()

# Executable
file(GLOB SOURCES src/*.cpp)
add_executable(game ${SOURCES})

# Include headers directory for the target
target_include_directories(game PRIVATE ${CMAKE_SOURCE_DIR}/headers)

# Link SDL3 and Vulkan
target_link_libraries(game
    ${Vulkan_LIBRARIES}
    ${SDL3_DIR}/lib/x64/SDL3.lib
)

# Link GLM if found via find_package
if(TARGET glm::glm)
    target_link_libraries(game glm::glm)
endif()

# Shader compilation
# Find glslc compiler (part of Vulkan SDK)
find_program(GLSL_VALIDATOR glslc HINTS ${Vulkan_GLSLC_EXECUTABLE} /usr/bin /usr/local/bin ${VULKAN_SDK_PATH}/Bin ${VULKAN_SDK_PATH}/Bin32)

if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed and glslc is in PATH")
endif()

# Create shaders output directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Function to compile shaders
function(add_shader TARGET SHADER)
    # Get shader name without extension
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)
    
    # Remove the dot from extension for output naming
    string(SUBSTRING ${SHADER_EXT} 1 -1 SHADER_EXT_NO_DOT)
    
    # Set output file path
    set(SHADER_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.${SHADER_EXT_NO_DOT}.spv)
    
    # Add custom command to compile shader
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSL_VALIDATOR} ${CMAKE_SOURCE_DIR}/shaders/${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${SHADER}
        COMMENT "Compiling shader ${SHADER} to SPIR-V"
    )
    
    # Add the compiled shader as a dependency of the target
    target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT})
endfunction()

# Compile shaders
add_shader(game vertex.vert)
add_shader(game fragment.frag)

# DLLs for runtime (optional, for dev convenience)
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL3_DIR}/lib/x64/SDL3.dll
        $<TARGET_FILE_DIR:game>
)
