#include "EnginePCH.h"
#include "ShadowRenderer.h"
#include "RenderResourceManager.h"
#include "RenderCommand.h"
#include "Renderer.h"

bool FShadowRenderer::Init(uint32 InResolution)
{
	ShadowMapShader = FRenderResourceManager::GetShaderProgram("Resources/Shader/ShadowDepth.hlsl");
	if (!ShadowMapShader)
	{
		HTR_LOG(Error, "[Shadow] shader not found");
		return false;
	}

	Resolution = InResolution;

	// 섀도우 맵
	D3D11_TEXTURE2D_DESC Desc{};
	Desc.Width = Resolution;
	Desc.Height = Resolution;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	ShadowMap = RenderCommand::CreateTexture2D(Desc);

	// 깊이 패스 -> 컬러타깃 X
	PassInfo.ViewportSetting.Width = Resolution;
	PassInfo.ViewportSetting.Height = Resolution;
	PassInfo.DepthStencil.Texture = ShadowMap.get();
	PassInfo.DepthStencil.LoadOp = ERenderTargetLoadOp::Clear;
	PassInfo.DepthStencil.ClearValue.DepthClearValue = 1.0f;

	PipelineState.Shader = ShadowMapShader;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PipelineState.RasterizerState = ERasterizerState::ShadowDepth;
	PipelineState.BlendState = EBlendState::Opaque;
	PipelineState.DepthStencilState = EDepthStencilState::Default;

    return true;
}

FMatrix FShadowRenderer::BuildDirectionalShadowVP(const FVector& CameraPosition, const FVector& CameraForward, const FVector& LightDirection, float ShadowDistance, float CameraNear, float CameraFar) const
{
	const float Radius = ShadowDistance;

	const FVector Forward = LightDirection.GetSafeNormal();
	const FVector UpGuide = (std::abs(Forward.Z) > 0.99f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, 0.0f, 1.0f);
	const FVector Right = FVector::Cross(UpGuide, Forward).GetSafeNormal();
	const FVector Up = FVector::Cross(Forward, Right);

	const FVector Center = CameraPosition + CameraForward * (ShadowDistance * 0.5f);

	// center를 텍셀 크기에 맞게 보정
	const float TexelSize = (Radius * 2.0f) / static_cast<float>(Resolution);
	const float CenterF = FVector::Dot(Center, Forward);
	const float CenterR = std::floor(FVector::Dot(Center, Right) / TexelSize) * TexelSize;
	const float CenterU = std::floor(FVector::Dot(Center, Up) / TexelSize) * TexelSize;
	const FVector SnappedCenter = Forward * CenterF + Right * CenterR + Up * CenterU;

	const FVector Eye = SnappedCenter - Forward * Radius;
	const FMatrix Rotation(
		FVector4(Forward.X, Forward.Y, Forward.Z, 0.0f),
		FVector4(Right.X, Right.Y, Right.Z, 0.0f),
		FVector4(Up.X, Up.Y, Up.Z, 0.0f),
		FVector4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	const FMatrix LightView = FMatrix::MakeTranslation(Eye * -1.0f) * Rotation.GetTransposed();

	const float S = 1.0f / Radius;
	const FMatrix LightOrtho(
		0.0f, 0.0f, S * 0.5f, 0.0f,
		S, 0.0f, 0.0f, 0.0f,
		0.0f, S, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return LightView * LightOrtho;
}

void FShadowRenderer::RenderDepth(FRenderer& Renderer, const FMatrix& ShadowVP)
{
	RenderCommand::BeginRenderPass(PassInfo);
	RenderCommand::BindPipelineState(PipelineState);
	Renderer.DrawDepthOnly(ShadowVP);
	RenderCommand::EndRenderPass(PassInfo);
}

void FShadowRenderer::BindShading()
{
	RenderCommand::BindShaderResource(ShadowMapSlot, ShadowMap.get(), EShaderBindFlagBits::Pixel);
	RenderCommand::BindSamplerState(ShadowSamplerSlot, ESamplerState::ShadowCompare, EShaderBindFlagBits::Pixel);
}

void FShadowRenderer::UnbindShading()
{
	RenderCommand::UnbindShaderResource(ShadowMapSlot, EShaderBindFlagBits::Pixel);
}
