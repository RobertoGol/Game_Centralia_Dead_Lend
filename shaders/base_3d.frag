#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// Global engine uniforms synchronized with Engine::Render variable states
uniform vec3 cameraPos;
uniform samplerCube environmentMap;
uniform vec3 stealthAlpha; // FIXED: Changed to vec3 to handle your vector data maps safely

// Multi-layered color parameters from your MaterialSystem database map
uniform vec3 materialParams; // X = Roughness, Y = PaintAlpha, Z = RustIntensity
uniform vec3 materialGlow;   // X = RadGlowIntensity

void main() {
    // Core base material color sets
    vec3 metalPrimerColor = vec3(0.25, 0.25, 0.27);  // Bare exposed raw primer substrate
    vec3 factoryPaintColor = vec3(0.15, 0.35, 0.65); // Factory paint coat finish
    vec3 rustColor = vec3(0.45, 0.22, 0.12);         // Degradation rust layer parameters
    vec3 radGlowColor = vec3(0.0, 0.95, 0.1);        // Neon bio-luminescence green phosphor layer

    // Dynamic linear interpolation mix calculations for material overlay stacks
    vec3 mixedPaint = mix(metalPrimerColor, factoryPaintColor, materialParams.y); 
    vec3 finalSurfaceColor = mix(mixedPaint, rustColor, materialParams.z);        

    // Old-school specular reflectance vector environment evaluations
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(FragPos - cameraPos);
    vec3 reflectDir = reflect(viewDir, norm);
    
    // Extract pixel color properties from your active cubemap channel
    vec3 dynamicReflection = texture(environmentMap, reflectDir).rgb;

    // Scale mirroring traits relative to surface roughness attributes
    float reflectionStrength = 0.4 * (1.0 - materialParams.x);
    vec3 shadedColor = mix(finalSurfaceColor, dynamicReflection, reflectionStrength);

    // Composite your emissive radioactive illumination properties onto the surface color profile
    vec3 finalCalculatedColor = shadedColor + (radGlowColor * materialGlow.x);

    // FIXED: Passed stealthAlpha vector parameter tracking index map through into target buffer channels
    FragColor = vec4(finalCalculatedColor, stealthAlpha.x);
}
