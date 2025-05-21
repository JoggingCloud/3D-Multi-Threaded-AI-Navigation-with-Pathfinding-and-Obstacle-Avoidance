//------------------------------------------------------------------------------------------------
Texture2D textureMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specularMap : register(t2);

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
float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 color;
	
    float ambient = AmbientIntensity * renderAmbientFlag;
    
    float3 worldNormal = normalize(input.normal.xyz);
    float3 pixelWorldNormal = worldNormal;

    if(useNormalMapFlag)
    {
        float3 tangentNormal = 2.0f * normalMap.Sample(textureSampler, input.uv).rgb - 1.0f;
        float3x3 tbnMat = float3x3(normalize(input.tangent.xyz), normalize(input.bitangent.xyz), normalize(input.normal.xyz));
        pixelWorldNormal = mul(tangentNormal, tbnMat);
    }
    
    float specularIntensity = 0.0f;
    float specularPower = 1.0f;
    float emissive = 0.0f;
    
    if(useSpecularMapFlag)
    {
        float4 specularColor = specularMap.Sample(textureSampler, input.uv);
        specularIntensity = specularColor.r;
        
        if(useGlossinessMapFlag)
        {
            specularPower = 1.0f + 31.0f * specularColor.g;
        }
        
        emissive = specularColor.b * (renderEmissiveFlag && useEmissiveMapFlag);
    }
    
    float3 viewVector = normalize(WorldEyePosition - input.worldPosition.xyz);
    float3 halfwayVector = normalize(-SunDirection + viewVector);

    float dotProd = dot(worldNormal, -SunDirection);
    
    float falloff = clamp(dotProd, MinFalloff, MaxFalloff);
    float falloffT = (falloff - MinFalloff) / (MaxFalloff - MinFalloff);
    float falloffMultiplier = lerp(MinFalloffMultiplier, MaxFalloffMultiplier, falloffT);
    
    float diffuse = SunIntensity * falloffMultiplier * saturate(dot(normalize(pixelWorldNormal), -SunDirection)) * renderDiffuseFlag;
   
    float nDotH = saturate(dot(pixelWorldNormal, halfwayVector));
	
    float specular = specularIntensity * pow(nDotH, specularPower) * falloffMultiplier * renderSpecularFlag;
	    
    float4 lightColor = float4((ambient + diffuse + specular + emissive).xxx, 1);
	float4 vertexColor = input.color;
    float4 modelColor = ModelColor;
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
	
    if(useDiffuseMapFlag)
    {
        textureColor = textureMap.Sample(textureSampler, input.uv);
    }
    
    color = lightColor * textureColor * vertexColor * modelColor;
    
    clip(color.a - 0.01f);
	
    return color;
}