//------------------------------------------------------------------------------------------------
Texture2D srcTexture : register(t0);
SamplerState srcSampler : register(s0);

struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

struct BlurSample
{
    float2 offset;
    float weight;
    int padding;
};

static const int MaxSamples = 64;

cbuffer BlurConstants : register(b5)
{
    float2 texelSize;
    float lerpT;
    int numSamples;
    BlurSample samples[MaxSamples];
};

v2p_t VertexMain(vs_input_t input)
{
    float4 localPosition = float4(input.localPosition, 1);

    v2p_t v2p;
    v2p.position = localPosition;
    v2p.uv = input.uv;

    return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    for (int i = 0; i < numSamples; i++)
    {
        color.rgb += srcTexture.Sample(srcSampler, input.uv + samples[i].offset * texelSize).rgb * samples[i].weight;
    }
    
    return float4(color);
}