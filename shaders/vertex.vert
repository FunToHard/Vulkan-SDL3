#version 450

// Vertex input attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// Uniform buffer object containing transformation matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;      // Model transformation matrix (object to world space)
    mat4 view;       // View transformation matrix (world to camera space)
    mat4 projection; // Projection transformation matrix (camera to clip space)
} ubo;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    // Transform vertex position through the complete MVP pipeline
    // This transforms from object space -> world space -> camera space -> clip space
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Pass through color and texture coordinates to fragment shader
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}