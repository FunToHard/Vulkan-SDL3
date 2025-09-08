#include "../headers/MainCharacter.h"
#include "../headers/VulkanUtils.h"
#include "../headers/Logger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cmath>

namespace VulkanGameEngine {

MainCharacter::MainCharacter()
    : m_isLoaded(false)
    , m_vertexCount(0)
    , m_indexCount(0)
    , m_transformMatrix(1.0f)
    , m_position(0.0f)
    , m_rotation(0.0f)
    , m_scale(1.0f)
    , m_colorMode(ColorMode::RAINBOW) {
    
    LOG_DEBUG("MainCharacter instance created", "MainCharacter");
}

MainCharacter::~MainCharacter() {
    cleanup();
    LOG_DEBUG("MainCharacter instance destroyed", "MainCharacter");
}

bool MainCharacter::loadFromOBJ(const std::string& filePath,
                                VkDevice device, 
                                VkPhysicalDevice physicalDevice,
                                VkCommandPool commandPool, 
                                VkQueue graphicsQueue) {
    
    LOG_INFO("Loading character model from: " + filePath, "MainCharacter");
    
    // Clean up any existing data
    cleanup();
    
    try {
        // Parse the OBJ file
        OBJData objData;
        if (!parseOBJFile(filePath, objData)) {
            LOG_ERROR("Failed to parse OBJ file: " + filePath, "MainCharacter");
            return false;
        }
        
        LOG_DEBUG("OBJ parsing completed - Positions: " + std::to_string(objData.positions.size()) +
                 ", Normals: " + std::to_string(objData.normals.size()) +
                 ", TexCoords: " + std::to_string(objData.texCoords.size()), "MainCharacter");
        
        // Convert OBJ data to vertex format
        convertOBJToVertices(objData);
        
        if (!validateModelData()) {
            LOG_ERROR("Model data validation failed", "MainCharacter");
            return false;
        }
        
        // Create Vulkan buffers
        if (!createBuffers(device, physicalDevice, commandPool, graphicsQueue)) {
            LOG_ERROR("Failed to create Vulkan buffers", "MainCharacter");
            return false;
        }
        
        m_isLoaded = true;
        m_vertexCount = static_cast<uint32_t>(m_vertices.size());
        m_indexCount = static_cast<uint32_t>(m_indices.size());
        
        // Initialize transform
        updateTransformMatrix();
        
        LOG_INFO("Character model loaded successfully - Vertices: " + std::to_string(m_vertexCount) +
                ", Triangles: " + std::to_string(m_indexCount / 3), "MainCharacter");
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during model loading: " + std::string(e.what()), "MainCharacter");
        cleanup();
        return false;
    }
}

void MainCharacter::setTransform(const glm::vec3& position, 
                                const glm::vec3& rotation, 
                                float scale) {
    m_position = position;
    m_rotation = rotation;
    m_scale = scale;
    updateTransformMatrix();
    
    LOG_DEBUG("Transform updated - Position: (" + 
             std::to_string(position.x) + ", " + 
             std::to_string(position.y) + ", " + 
             std::to_string(position.z) + ")", "MainCharacter");
}

void MainCharacter::cleanup() {
    if (m_isLoaded) {
        LOG_DEBUG("Cleaning up MainCharacter resources", "MainCharacter");
        
        m_vertexBuffer.cleanup();
        m_indexBuffer.cleanup();
        
        m_vertices.clear();
        m_indices.clear();
        
        m_isLoaded = false;
        m_vertexCount = 0;
        m_indexCount = 0;
    }
}

void MainCharacter::getModelStats(uint32_t& vertexCount, uint32_t& triangleCount) const {
    vertexCount = m_vertexCount;
    triangleCount = m_indexCount / 3;
}

bool MainCharacter::parseOBJFile(const std::string& filePath, OBJData& objData) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open OBJ file: " + filePath, "MainCharacter");
        return false;
    }
    
    objData.clear();
    std::string line;
    uint32_t lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        try {
            if (prefix == "v") {
                // Vertex position
                float x, y, z;
                if (iss >> x >> y >> z) {
                    objData.positions.emplace_back(x, y, z);
                } else {
                    LOG_WARN("Invalid vertex position at line " + std::to_string(lineNumber), "MainCharacter");
                }
            }
            else if (prefix == "vn") {
                // Vertex normal
                float x, y, z;
                if (iss >> x >> y >> z) {
                    objData.normals.emplace_back(x, y, z);
                } else {
                    LOG_WARN("Invalid vertex normal at line " + std::to_string(lineNumber), "MainCharacter");
                }
            }
            else if (prefix == "vt") {
                // Texture coordinate
                float u, v;
                if (iss >> u >> v) {
                    objData.texCoords.emplace_back(u, v);
                } else {
                    LOG_WARN("Invalid texture coordinate at line " + std::to_string(lineNumber), "MainCharacter");
                }
            }
            else if (prefix == "f") {
                // Face definition
                if (!parseFaceLine(line, objData)) {
                    LOG_WARN("Invalid face definition at line " + std::to_string(lineNumber), "MainCharacter");
                }
            }
            // Ignore other OBJ elements (materials, groups, etc.)
            
        } catch (const std::exception& e) {
            LOG_WARN("Error parsing line " + std::to_string(lineNumber) + ": " + e.what(), "MainCharacter");
        }
    }
    
    file.close();
    
    // Validate parsed data
    if (objData.positions.empty()) {
        LOG_ERROR("No vertex positions found in OBJ file", "MainCharacter");
        return false;
    }
    
    if (objData.indices.empty()) {
        LOG_ERROR("No faces found in OBJ file", "MainCharacter");
        return false;
    }
    
    LOG_DEBUG("OBJ file parsed successfully", "MainCharacter");
    return true;
}

bool MainCharacter::parseFaceLine(const std::string& line, OBJData& objData) {
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix; // Skip "f"
    
    std::vector<std::string> faceVertices;
    std::string vertex;
    
    // Read all vertex definitions in the face
    while (iss >> vertex) {
        faceVertices.push_back(vertex);
    }
    
    // We need at least 3 vertices for a triangle
    if (faceVertices.size() < 3) {
        return false;
    }
    
    // For now, we only support triangular faces
    // If the face has more than 3 vertices, we triangulate it by creating a fan
    for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
        // Create triangle: vertex 0, vertex i, vertex i+1
        std::vector<std::string> triangle = {faceVertices[0], faceVertices[i], faceVertices[i + 1]};
        
        for (const std::string& vertexStr : triangle) {
            // Parse vertex string (format: v/vt/vn or v//vn or v/vt or v)
            std::vector<int> indices(3, -1); // [position, texcoord, normal]
            
            size_t pos = 0;
            int component = 0;
            
            while (pos < vertexStr.length() && component < 3) {
                size_t nextSlash = vertexStr.find('/', pos);
                std::string indexStr;
                
                if (nextSlash == std::string::npos) {
                    indexStr = vertexStr.substr(pos);
                    pos = vertexStr.length();
                } else {
                    indexStr = vertexStr.substr(pos, nextSlash - pos);
                    pos = nextSlash + 1;
                }
                
                if (!indexStr.empty()) {
                    try {
                        indices[component] = std::stoi(indexStr) - 1; // OBJ indices are 1-based
                    } catch (const std::exception&) {
                        return false;
                    }
                }
                
                component++;
            }
            
            // Validate position index (required)
            if (indices[0] < 0 || indices[0] >= static_cast<int>(objData.positions.size())) {
                return false;
            }
            
            objData.indices.push_back(static_cast<uint32_t>(indices[0]));
        }
    }
    
    return true;
}

void MainCharacter::convertOBJToVertices(const OBJData& objData) {
    m_vertices.clear();
    m_indices.clear();
    
    // Create a map to avoid duplicate vertices
    std::unordered_map<uint32_t, uint32_t> vertexMap;
    
    for (size_t i = 0; i < objData.indices.size(); ++i) {
        uint32_t posIndex = objData.indices[i];
        
        // Check if we've already processed this vertex
        if (vertexMap.find(posIndex) != vertexMap.end()) {
            m_indices.push_back(vertexMap[posIndex]);
            continue;
        }
        
        // Create new vertex
        Vertex vertex{};
        
        // Position (required)
        if (posIndex < objData.positions.size()) {
            vertex.position = objData.positions[posIndex];
        } else {
            LOG_WARN("Invalid position index: " + std::to_string(posIndex), "MainCharacter");
            vertex.position = glm::vec3(0.0f);
        }
        
        // Color (generate default since OBJ doesn't typically have colors)
        vertex.color = generateDefaultColor(static_cast<uint32_t>(m_vertices.size()), vertex.position);
        
        // Texture coordinates (use default if not available)
        vertex.texCoord = glm::vec2(0.0f, 0.0f);
        
        // Add vertex to list
        uint32_t vertexIndex = static_cast<uint32_t>(m_vertices.size());
        m_vertices.push_back(vertex);
        m_indices.push_back(vertexIndex);
        vertexMap[posIndex] = vertexIndex;
    }
    
    LOG_DEBUG("Converted OBJ to " + std::to_string(m_vertices.size()) + " vertices and " + 
             std::to_string(m_indices.size()) + " indices", "MainCharacter");
}

bool MainCharacter::createBuffers(VkDevice device, 
                                 VkPhysicalDevice physicalDevice,
                                 VkCommandPool commandPool, 
                                 VkQueue graphicsQueue) {
    
    try {
        // Create vertex buffer
        m_vertexBuffer = BufferUtils::createVertexBuffer(
            device, physicalDevice, commandPool, graphicsQueue, m_vertices);
        
        // Create index buffer
        m_indexBuffer = BufferUtils::createIndexBuffer(
            device, physicalDevice, commandPool, graphicsQueue, m_indices);
        
        LOG_DEBUG("Vulkan buffers created successfully", "MainCharacter");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create buffers: " + std::string(e.what()), "MainCharacter");
        return false;
    }
}

void MainCharacter::updateTransformMatrix() {
    // Create transformation matrix: T * R * S
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_position);
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 rotation = rotationZ * rotationY * rotationX;
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(m_scale));
    
    m_transformMatrix = translation * rotation * scale;
}

glm::vec3 MainCharacter::generateDefaultColor(uint32_t vertexIndex, const glm::vec3& position) const {
    switch (m_colorMode) {
        case ColorMode::RAINBOW:
            return generateRainbowColor(vertexIndex);
        case ColorMode::GRADIENT:
            return generateGradientColor(position);
        case ColorMode::ANATOMICAL:
            return generateAnatomicalColor(position);
        case ColorMode::METALLIC:
            return generateMetallicColor(vertexIndex);
        case ColorMode::PASTEL:
            return generatePastelColor(vertexIndex);
        default:
            return generateRainbowColor(vertexIndex);
    }
}

glm::vec3 MainCharacter::generateRainbowColor(uint32_t vertexIndex) const {
    // Generate a simple color pattern based on vertex index
    float hue = (vertexIndex * 0.618033988749895f); // Golden ratio for good distribution
    hue = hue - std::floor(hue); // Keep fractional part
    
    // Simple HSV to RGB conversion for variety
    float r, g, b;
    if (hue < 1.0f / 3.0f) {
        r = 1.0f - 3.0f * hue;
        g = 3.0f * hue;
        b = 0.0f;
    } else if (hue < 2.0f / 3.0f) {
        r = 0.0f;
        g = 2.0f - 3.0f * hue;
        b = 3.0f * hue - 1.0f;
    } else {
        r = 3.0f * hue - 2.0f;
        g = 0.0f;
        b = 3.0f - 3.0f * hue;
    }
    
    // Clamp and add some brightness
    r = glm::clamp(r * 0.7f + 0.3f, 0.0f, 1.0f);
    g = glm::clamp(g * 0.7f + 0.3f, 0.0f, 1.0f);
    b = glm::clamp(b * 0.7f + 0.3f, 0.0f, 1.0f);
    
    return glm::vec3(r, g, b);
}

glm::vec3 MainCharacter::generateGradientColor(const glm::vec3& position) const {
    // Color based on Y position (height)
    float normalizedY = (position.y + 1.0f) * 0.5f; // Normalize to 0-1
    return glm::vec3(
        0.2f + normalizedY * 0.6f,  // Red increases with height
        0.8f - normalizedY * 0.3f,  // Green decreases with height
        0.9f - normalizedY * 0.4f   // Blue decreases with height
    );
}

glm::vec3 MainCharacter::generateAnatomicalColor(const glm::vec3& position) const {
    // Different colors for different body parts based on Y position
    if (position.y > 1.4f) {
        return glm::vec3(0.9f, 0.7f, 0.6f); // Head - skin tone
    } else if (position.y > 0.6f) {
        return glm::vec3(0.2f, 0.6f, 0.9f); // Torso - blue shirt
    } else if (position.y > 0.0f) {
        return glm::vec3(0.1f, 0.5f, 0.1f); // Legs - green pants
    } else {
        return glm::vec3(0.3f, 0.2f, 0.1f); // Feet - brown shoes
    }
}

glm::vec3 MainCharacter::generateMetallicColor(uint32_t vertexIndex) const {
    float phase = vertexIndex * 0.1f;
    float metallic = 0.7f + 0.3f * std::sin(phase);
    return glm::vec3(metallic * 0.8f, metallic * 0.85f, metallic * 0.9f);
}

glm::vec3 MainCharacter::generatePastelColor(uint32_t vertexIndex) const {
    float hue = (vertexIndex * 0.618033988749895f);
    hue = hue - std::floor(hue);
    
    // Pastel colors have high lightness and low saturation
    float r, g, b;
    if (hue < 1.0f / 3.0f) {
        r = 0.9f - 0.2f * hue;
        g = 0.7f + 0.2f * hue;
        b = 0.8f;
    } else if (hue < 2.0f / 3.0f) {
        r = 0.8f;
        g = 0.9f - 0.2f * (hue - 1.0f/3.0f) * 3.0f;
        b = 0.7f + 0.2f * (hue - 1.0f/3.0f) * 3.0f;
    } else {
        r = 0.7f + 0.2f * (hue - 2.0f/3.0f) * 3.0f;
        g = 0.8f;
        b = 0.9f - 0.2f * (hue - 2.0f/3.0f) * 3.0f;
    }
    
    return glm::vec3(r, g, b);
}

bool MainCharacter::validateModelData() const {
    if (m_vertices.empty()) {
        LOG_ERROR("No vertices in model data", "MainCharacter");
        return false;
    }
    
    if (m_indices.empty()) {
        LOG_ERROR("No indices in model data", "MainCharacter");
        return false;
    }
    
    if (m_indices.size() % 3 != 0) {
        LOG_ERROR("Index count is not divisible by 3 (not triangular)", "MainCharacter");
        return false;
    }
    
    // Check if all indices are valid
    for (uint32_t index : m_indices) {
        if (index >= m_vertices.size()) {
            LOG_ERROR("Invalid index found: " + std::to_string(index), "MainCharacter");
            return false;
        }
    }
    
    return true;
}

} // namespace VulkanGameEngine