cmake_minimum_required(VERSION 3.24)
project(game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# SDL3 Dependency via FetchContent
# ---------------------------------------------------------------------------
set(SDL_TEST OFF CACHE BOOL "Disable SDL test suite" FORCE)
set(SDL_SHARED ON CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build SDL static library" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.18
    GIT_SHALLOW    TRUE
)

# ---------------------------------------------------------------------------
# GLM Dependency via FetchContent
# ---------------------------------------------------------------------------
set(GLM_BUILD_TESTS OFF CACHE BOOL "Disable GLM tests" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

# Populate dependencies
FetchContent_MakeAvailable(SDL3 glm)

# ---------------------------------------------------------------------------
# Vulkan SDK
# ---------------------------------------------------------------------------
# Auto-detect Vulkan SDK if session environment variable hasn't refreshed yet
if(WIN32 AND NOT DEFINED ENV{VULKAN_SDK})
    file(GLOB VULKAN_SDK_DIRS "C:/VulkanSDK/*")
    if(VULKAN_SDK_DIRS)
        list(REVERSE VULKAN_SDK_DIRS)
        list(GET VULKAN_SDK_DIRS 0 LATEST_VULKAN_SDK)
        set(ENV{VULKAN_SDK} "${LATEST_VULKAN_SDK}")
        set(Vulkan_ROOT "${LATEST_VULKAN_SDK}")
    endif()
endif()

find_package(Vulkan REQUIRED)

# Find glslc compiler
find_program(GLSL_VALIDATOR glslc
    HINTS
        ${Vulkan_GLSLC_EXECUTABLE}
        "$ENV{VULKAN_SDK}/Bin"
        "$ENV{VULKAN_SDK}/bin"
        /usr/bin
        /usr/local/bin
)

if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "glslc not found! Make sure Vulkan SDK is installed and glslc is in PATH or VULKAN_SDK is set.")
endif()

# ---------------------------------------------------------------------------
# Executable Target
# ---------------------------------------------------------------------------
file(GLOB SOURCES src/*.cpp)
add_executable(game ${SOURCES})

target_include_directories(game PRIVATE ${CMAKE_SOURCE_DIR}/headers)

target_link_libraries(game PRIVATE
    SDL3::SDL3
    glm::glm
    Vulkan::Vulkan
)

# ---------------------------------------------------------------------------
# Shader Compilation
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

function(add_shader TARGET SHADER)
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)
    string(SUBSTRING ${SHADER_EXT} 1 -1 SHADER_EXT_NO_DOT)
    
    set(SHADER_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.${SHADER_EXT_NO_DOT}.spv)
    
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSL_VALIDATOR} ${CMAKE_SOURCE_DIR}/shaders/${SHADER} -o ${SHADER_OUTPUT}
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${SHADER}
        COMMENT "Compiling shader ${SHADER} to SPIR-V"
    )
    
    target_sources(${TARGET} PRIVATE ${SHADER_OUTPUT})
endfunction()

add_shader(game vertex.vert)
add_shader(game fragment.frag)

# ---------------------------------------------------------------------------
# Post-build: Copy DLLs, Shaders, and Assets
# ---------------------------------------------------------------------------
# Copy runtime DLLs (e.g. SDL3.dll) next to the executable
if(WIN32)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:game>
            $<TARGET_FILE_DIR:game>
        COMMAND_EXPAND_LISTS
    )
endif()

# Copy compiled shaders to the target directory
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:game>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/shaders/vertex.vert.spv
        ${CMAKE_BINARY_DIR}/shaders/fragment.frag.spv
        $<TARGET_FILE_DIR:game>/shaders/
)

# Copy assets to the target directory if they exist
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:game>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:game>/assets
    )
endif()
