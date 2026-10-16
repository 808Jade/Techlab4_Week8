#pragma pack_matrix(row_major)

cbuffer ViewConstants : register(b0)
{
    matrix LightViewProjection;
    float3 CameraPosition;
    float Padding;
};

cbuffer PerObjectConstants : register(b2)
{
    matrix World;
    matrix NormalMatrix;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
};

struct PSInput
{
    float4 Position : SV_POSITION;
};

PSInput mainVS(VSInput Input)
{
    PSInput Output;
    Output.Position = mul(mul(float4(Input.Position, 1.0f), World), LightViewProjection);
    return Output;
}

void mainPS(PSInput Input)
{
    // Depth 는 GPU 하드웨어에 의해 자동으로 Depth Buffer 에 기록됨
}
