// shaders/simple.slang

struct GlobalUniforms {
    float4x4 view;
    float4x4 proj;
    float4 lightDir;
    float4 cameraPos;
};

[[vk::binding(0, 0)]]
ConstantBuffer<GlobalUniforms> gUniforms;

// Define the struct for push constants
struct ModelPushConstantData { // <<< Use this name
    float4x4 model;
};

// Declare the push constant variable using the correct struct name
[[vk::push_constant]]
ModelPushConstantData pushConstants; // <<< Match struct name here

struct VertexInput {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 color    : COLOR0;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 worldNormal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 color    : COLOR0;
    float3 worldPos : TEXCOORD1;
};

// Vertex Shader
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    // Use the push constant variable
    float4 worldPos = mul(pushConstants.model, float4(input.position, 1.0));

    output.position = mul(gUniforms.proj, mul(gUniforms.view, worldPos));

    // Transform normal using the push constant variable
    // <<< FIX HERE: Use pushConstants.model >>>
    output.worldNormal = normalize(mul((float3x3)pushConstants.model, input.normal));

    output.texCoord = input.texCoord;
    output.color = input.color;
    output.worldPos = worldPos.xyz;
    return output;
}

// Fragment Shader
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Basic Lambertian lighting
    float3 lightDirection = normalize(gUniforms.lightDir.xyz);
    // Ensure normal is normalized after interpolation
    float3 normal = normalize(input.worldNormal);
    // Add ambient term 0.1 to avoid pure black
    float lightIntensity = max(dot(normal, -lightDirection), 0.1);

    float3 finalColor = input.color * lightIntensity;

    // Optional: Add simple specular highlight (Blinn-Phong style)
    float3 viewDir = normalize(gUniforms.cameraPos.xyz - input.worldPos);
    float3 halfVec = normalize(-lightDirection + viewDir);
    float specAngle = max(dot(normal, halfVec), 0.0);
    // Increase shininess factor for sharper highlights
    float specular = pow(specAngle, 64.0);
    finalColor += float3(1.0, 1.0, 1.0) * specular * 0.5; // White specular highlights, reduce intensity a bit

    // Clamp final color to avoid over-brightening
    finalColor = clamp(finalColor, 0.0, 1.0);

    return float4(finalColor, 1.0);
}