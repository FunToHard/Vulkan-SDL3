#version 450

// Input from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

// Output color to framebuffer
layout(location = 0) out vec4 outColor;

void main() {
    // For now, output the interpolated vertex color
    // This creates a simple colored rendering without textures
    // The alpha channel is set to 1.0 for full opacity
    outColor = vec4(fragColor, 1.0);
    
    // Note: In future iterations, this could be enhanced with:
    // - Texture sampling: texture(texSampler, fragTexCoord)
    // - Lighting calculations
    // - Material properties
    // - Normal mapping, etc.
}