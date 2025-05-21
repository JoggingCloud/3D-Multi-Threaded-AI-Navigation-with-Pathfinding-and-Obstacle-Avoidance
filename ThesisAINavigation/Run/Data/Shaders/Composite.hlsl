//------------------------------------------------------------------------------------------------
Texture2D bloomTexture : register(t0);
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
    float4 color = bloomTexture.Sample(srcSampler, input.uv);
    
    return float4(color);
}