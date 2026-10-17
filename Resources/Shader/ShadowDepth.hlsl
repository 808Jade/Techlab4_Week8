#pragma pack_matrix(row_major)

cbuffer ViewConstants : register(b0)
{
    matrix ViewProjection;
    float3 CameraPosition;
    float ViewPadding;
};

cbuffer PerObjectConstants : register(b2)
{
    matrix World;
    matrix NormalMatrix;
};

struct FVSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float4 Color : COLOR; // 지금은 쓰지 않지만 나중에 쓰게 될 수 있음
    float2 UV : TEXCOORD;
};

float4 mainVS(FVSInput Input) : SV_POSITION
{
    float4 WorldPos = mul(float4(Input.Position, 1.0f), World);
    return mul(WorldPos, ViewProjection);
}

void mainPS(float4 Position: SV_POSITION)
{
}