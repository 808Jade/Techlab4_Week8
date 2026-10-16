#pragma pack_matrix(row_major) 

cbuffer FProjectionConstant : register(b0)
{
    float4x4 LightProjection;
};

cbuffer FWorldConstant : register(b2)
{
    float4x4 World;
};

struct VSInput
{
    float3 Position : POSITION;
};

struct PSInput
{
    float4 Position : SV_POSITION;
};

PSInput mainVS(VSInput input)
{
    PSInput Output;
    float4 position = float4(input.Position, 1.0f);
    Output.Position = mul(mul(position, World), LightProjection);
    return Output;
}

void mainPS(PSInput input)
{
}