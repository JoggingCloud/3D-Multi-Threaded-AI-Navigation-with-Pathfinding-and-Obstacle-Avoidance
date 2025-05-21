//------------------------------------------------------------------------------------------------

static const float epsilon = 0.01f; // Set a small value to compare colors

//------------------------------------------------------------------------------------------------
Texture2D grassDiffuse : register(t0);
Texture2D rockDiffuse : register(t1);
Texture2D snowDiffuse : register(t2);

Texture2D grassNormal : register(t3);
Texture2D rockNormal : register(t5);
Texture2D snowNormal : register(t6);

Texture2D grassSpecular : register(t7);
Texture2D rockSpecular : register(t8);
Texture2D snowSpecular : register(t9);

SamplerState textureSampler : register(s0);

//------------------------------------------------------------------------------------------------
cbuffer DirectionalLight : register(b1)
{
	float3 SunDirection;
	float SunIntensity;
    float AmbientIntensity;
    float3 WorldEyePosition;
    float MinFalloff;
    float MaxFalloff;
    float MinFalloffMultiplier;
    float MaxFalloffMultiplier;
    int renderAmbientFlag;
    int renderDiffuseFlag;
    int renderSpecularFlag;
    int renderEmissiveFlag;
    int useDiffuseMapFlag;
    int useNormalMapFlag;
    int useSpecularMapFlag;
    int useGlossinessMapFlag;
    int useEmissiveMapFlag;
    float3 padding;
};

//------------------------------------------------------------------------------------------------

cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
};

//------------------------------------------------------------------------------------------------

cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

//------------------------------------------------------------------------------------------------

struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 localTangent : TANGENT;
	float3 localBitangent : BITANGENT;
	float3 localNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------

struct v2p_t
{
	float4 clipPosition : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 tangent : TANGENT;
	float4 bitangent : BITANGENT;
	float4 normal : NORMAL;
    float4 worldPosition : POSITION;
};

//------------------------------------------------------------------------------------------------

v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);
	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);
	float4 localNormal = float4(input.localNormal, 0);
    float4 worldNormal = mul(ModelMatrix, localNormal);
    float4 localTangent = float4(input.localTangent, 0);
    float4 worldTangent = mul(ModelMatrix, localTangent);
    float4 localBiTangent = float4(input.localBitangent, 0);
    float4 worldBiTangent = mul(ModelMatrix, localBiTangent);
	
	v2p_t v2p;
    v2p.clipPosition = clipPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.tangent = worldTangent;
	v2p.bitangent = worldBiTangent;
	v2p.normal = worldNormal;
    v2p.worldPosition = worldPosition;
	return v2p;
}

//------------------------------------------------------------------------------------------------

bool ColorIsClose(float4 a, float4 b, float epsilon) // For comparing colors
{
    return abs(a.r - b.r) < epsilon &&
           abs(a.g - b.g) < epsilon &&
           abs(a.b - b.b) < epsilon &&
           abs(a.a - b.a) < epsilon;
}

//------------------------------------------------------------------------------------------------

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 color;

    float ambient = AmbientIntensity * renderAmbientFlag;

    // Normal calculation
    float3 worldNormal = normalize(input.normal.xyz);
    float3 pixelWorldNormal = worldNormal;

    if (useNormalMapFlag)
    {
        float3 blendedNormal;
    
        if (input.color.b > 0.0f && input.color.g > 0.0f && input.color.r == 0.0f) // Blend between grass and rock
        {
            float3 grassNormalSample = 2.0f * grassNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            float3 rockNormalSample = 2.0f * rockNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            blendedNormal = lerp(grassNormalSample, rockNormalSample, input.color.g);
        }
        else if (input.color.g > 0.0f && input.color.r > 0.0f) // Blend between rock and snow
        {
            float3 rockNormalSample = 2.0f * rockNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            float3 snowNormalSample = 2.0f * snowNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            blendedNormal = lerp(rockNormalSample, snowNormalSample, input.color.r);
        }
        else
        {
            if (ColorIsClose(input.color, float4(0.0f, 0.0f, 1.0f, 1.0f), epsilon)) // Blue -> Ground
            {
                blendedNormal = 2.0f * grassNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            }
            else if (ColorIsClose(input.color, float4(0.0f, 1.0f, 0.0f, 1.0f), epsilon)) // Green -> Hill
            {
                blendedNormal = 2.0f * rockNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            }
            else if (ColorIsClose(input.color, float4(1.0f, 0.0f, 0.0f, 1.0f), epsilon)) // Red -> Snow
            {
                blendedNormal = 2.0f * snowNormal.Sample(textureSampler, input.uv).rgb - 1.0f;
            }
            else
            {
                blendedNormal = 2.0f * grassNormal.Sample(textureSampler, input.uv).rgb - 1.0f; // Default case
            }
        }

        float3x3 tbnMat = float3x3(normalize(input.tangent.xyz), normalize(input.bitangent.xyz), normalize(input.normal.xyz));
        pixelWorldNormal = mul(blendedNormal, tbnMat);
    }   

    // Diffuse Calculation
    float4 grassDiffuseSample = grassDiffuse.Sample(textureSampler, input.uv);
    float4 rockDiffuseSample = rockDiffuse.Sample(textureSampler, input.uv);
    float4 snowDiffuseSample = snowDiffuse.Sample(textureSampler, input.uv);
    
    // Blend between the diffuse textures based on the vertex color
    float4 blendedDiffuse;
    
    if (input.color.b > 0.0f && input.color.g > 0.0f && input.color.r == 0.0f) // Blend between grass (Blue -> Ground) and rock (Green -> Hill) 
    {
        blendedDiffuse = lerp(grassDiffuseSample, rockDiffuseSample, input.color.g);
    }
    else if (input.color.g > 0.0f && input.color.r > 0.0f) // Blend between rock (Green -> Hill) and snow (Red -> Snow)
    {
        blendedDiffuse = lerp(rockDiffuseSample, snowDiffuseSample, input.color.r);
    }
    else
    {
        if (ColorIsClose(input.color, float4(0.0f, 0.0f, 1.0f, 1.0f), epsilon)) // Blue -> Ground
        {
            blendedDiffuse = grassDiffuse.Sample(textureSampler, input.uv);
        }
        else if (ColorIsClose(input.color, float4(0.0f, 1.0f, 0.0f, 1.0f), epsilon)) // Green -> Hill
        {
            blendedDiffuse = rockDiffuse.Sample(textureSampler, input.uv);
        }
        else if (ColorIsClose(input.color, float4(1.0f, 0.0f, 0.0f, 1.0f), epsilon)) // Red -> Snow
        {
            blendedDiffuse = snowDiffuse.Sample(textureSampler, input.uv);
        }
        else 
        {
            blendedDiffuse = grassDiffuseSample; // Default case
        }
    }
    
    // Specular calculation and blending for specular map
    float specularIntensity = 0.0f;
    float specularPower = 1.0f;
    float emissive = 0.0f;

    if (useSpecularMapFlag)
    {
        float4 blendedSpecular;
        
        if (input.color.b > 0.0f && input.color.g > 0.0f && input.color.r == 0.0f) // Blend between grass and rock
        {
            float4 grassSpecularSample = grassSpecular.Sample(textureSampler, input.uv);
            float4 rockSpecularSample = rockSpecular.Sample(textureSampler, input.uv);
            blendedSpecular = lerp(grassSpecularSample, rockSpecularSample, input.color.g);
        }
        else if (input.color.g > 0.0f && input.color.r > 0.0f) // Blend between rock and snow
        {
            float4 rockSpecularSample = rockSpecular.Sample(textureSampler, input.uv);
            float4 snowSpecularSample = snowSpecular.Sample(textureSampler, input.uv);
            blendedSpecular = lerp(rockSpecularSample, snowSpecularSample, input.color.r);
        }
        else
        {
            if (ColorIsClose(input.color, float4(0.0f, 0.0f, 1.0f, 1.0f), epsilon)) // Blue -> Ground
            {
                blendedSpecular = grassSpecular.Sample(textureSampler, input.uv);
            }
            else if (ColorIsClose(input.color, float4(0.0f, 1.0f, 0.0f, 1.0f), epsilon)) // Green -> Hill
            {
                blendedSpecular = rockSpecular.Sample(textureSampler, input.uv);
            }
            else if (ColorIsClose(input.color, float4(1.0f, 0.0f, 0.0f, 1.0f), epsilon)) // Red -> Snow
            {
                blendedSpecular = snowSpecular.Sample(textureSampler, input.uv);
            }
            else
            {
                blendedSpecular = grassSpecular.Sample(textureSampler, input.uv); // Default case
            }
        }
    
        specularIntensity = blendedSpecular.r;
    
        if (useGlossinessMapFlag)
        {
            specularPower = 1.0f + 31.0f * blendedSpecular.g;
        }
    
        emissive = blendedSpecular.b * (renderEmissiveFlag && useEmissiveMapFlag);
    }

    // Lighting Calculations
    float3 viewVector = normalize(WorldEyePosition - input.worldPosition.xyz);
    float3 halfwayVector = normalize(-SunDirection + viewVector);

    float dotProd = dot(worldNormal, -SunDirection);

    float falloff = clamp(dotProd, MinFalloff, MaxFalloff);
    float falloffT = (falloff - MinFalloff) / (MaxFalloff - MinFalloff);
    float falloffMultiplier = lerp(MinFalloffMultiplier, MaxFalloffMultiplier, falloffT);

    float diffuse = SunIntensity * falloffMultiplier * saturate(dot(normalize(pixelWorldNormal), -SunDirection)) * renderDiffuseFlag;

    float nDotH = saturate(dot(pixelWorldNormal, halfwayVector));

    float specular = specularIntensity * pow(nDotH, specularPower) * falloffMultiplier * renderSpecularFlag;

    // Final light calculation
    float4 lightColor = float4((ambient + diffuse + specular + emissive).xxx, 1);
    float4 vertexColor = input.color;
    float4 modelColor = ModelColor;

    // Final color calculation
    color = lightColor * blendedDiffuse * modelColor;

    clip(color.a - 0.01f);

    return color;
}