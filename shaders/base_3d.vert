#version 330 core

// Input vertex arrays mapped via Renderer3D VBO specifications
layout (location = 0) in vec3 aPos;     // Vertex position data coordinates
layout (location = 1) in vec3 aNormal;  // Surface normal coordinates (Reflectance inputs)

// Output layout parameters routed directly to fragment block evaluation streams
out vec3 FragPos;  
out vec3 Normal;   

// Standard transformation layout passed continuously from your Engine loop ticks
uniform mat4 projectionViewMatrix;

void main() {
    // Spatial coordinates calculation mapped straight to world coordinate lines
    FragPos = aPos;
    
    // Pass vertex normal information down the pixel processing channel
    Normal = aNormal;  
    
    // Project the model vertex data directly to your target viewport bounds
    gl_Position = projectionViewMatrix * vec4(aPos, 1.0);
}
