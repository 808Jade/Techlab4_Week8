#include "EnginePCH.h"
#include "ShadowRenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Engine/PrimitiveSceneProxy.h"
#include "Engine/LightSceneProxy.h"

bool FShadowRenderer::Init(uint32 InResolution)
{
	Resolution = InResolution;

	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Width = Resolution;
	Desc.Height = Resolution;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;

	DirectionalShadowMap = RenderCommand::CreateTexture2D(Desc);
	if (!DirectionalShadowMap || !DirectionalShadowMap->GetDSV() || !DirectionalShadowMap->GetSRV())
	{
		HTR_LOG(Error, "[ShadowRenderer] Failed to create Directional shadow map texture (Resolution: {})", Resolution);
		return false;
	}

	DirectionalShadowPassInfo.ViewportSetting.StartX = 0;
	DirectionalShadowPassInfo.ViewportSetting.StartY = 0;
	DirectionalShadowPassInfo.ViewportSetting.Width = Resolution;
	DirectionalShadowPassInfo.ViewportSetting.Height = Resolution;
	DirectionalShadowPassInfo.ViewportSetting.MinDepth = 0.0f;
	DirectionalShadowPassInfo.ViewportSetting.MaxDepth = 1.0f;

	DirectionalShadowPassInfo.ColorRenderTargets.Empty();
	DirectionalShadowPassInfo.DepthStencil.Texture = DirectionalShadowMap.get();
	DirectionalShadowPassInfo.DepthStencil.LoadOp = ERenderTargetLoadOp::Clear;
	DirectionalShadowPassInfo.DepthStencil.StoreOp = ERenderTargetStoreOp::Store;
	DirectionalShadowPassInfo.DepthStencil.ClearValue.DepthClearValue = 1.0f;
	DirectionalShadowPassInfo.DepthStencil.ClearValue.StencilClearValue = 0;

	SpotShadowMap = RenderCommand::CreateTexture2D(Desc);
	if (!SpotShadowMap || !SpotShadowMap->GetDSV() || !SpotShadowMap->GetSRV())
	{
		HTR_LOG(Error, "[ShadowRenderer] Failed to create spot shadow map texture (Resolution: {})", Resolution);
		return false;
	}

	SpotShadowPassInfo.ViewportSetting.StartX = 0;
	SpotShadowPassInfo.ViewportSetting.StartY = 0;
	SpotShadowPassInfo.ViewportSetting.Width = Resolution;
	SpotShadowPassInfo.ViewportSetting.Height = Resolution;
	SpotShadowPassInfo.ViewportSetting.MinDepth = 0.0f;
	SpotShadowPassInfo.ViewportSetting.MaxDepth = 1.0f;

	SpotShadowPassInfo.ColorRenderTargets.Empty();
	SpotShadowPassInfo.DepthStencil.Texture = SpotShadowMap.get();
	SpotShadowPassInfo.DepthStencil.LoadOp = ERenderTargetLoadOp::Clear;
	SpotShadowPassInfo.DepthStencil.StoreOp = ERenderTargetStoreOp::Store;
	SpotShadowPassInfo.DepthStencil.ClearValue.DepthClearValue = 1.0f;
	SpotShadowPassInfo.DepthStencil.ClearValue.StencilClearValue = 0;

	ShadowShader = FRenderResourceManager::GetShaderProgram("Resources/Shader/ShadowDepthShader.hlsl");
	if (!ShadowShader)
	{
		HTR_LOG(Error, "[ShadowRenderer] ShadowDepthShader.hlsl not found");
		return false;
	}

	PipelineState.Shader = ShadowShader;
	PipelineState.RasterizerState = ERasterizerState::SolidBack;
	PipelineState.BlendState = EBlendState::NoColorWrite;
	PipelineState.DepthStencilState = EDepthStencilState::Default;
	PipelineState.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	ShadowViewCB = RenderCommand::CreateConstantBuffer(sizeof(FViewConstants));
	PerObjectCB = RenderCommand::CreateConstantBuffer(sizeof(FPerObjectConstants));
	DirectionalShadowConstantsCB = RenderCommand::CreateConstantBuffer(sizeof(FDirectionalShadowConstants));
	SpotShadowConstantsCB = RenderCommand::CreateConstantBuffer(sizeof(FSpotShadowConstants));

	DisableDirectionalShadow();
	DisableSpotShadow();

	return true;
}

// LightDirection : 태양이 내리쬐는 각도 / FocusPosition : 내 카메라의 위치
void FShadowRenderer::UpdateLightMatrix(const FVector& LightDirection, const FVector& FocusPosition)
{
	FVector LightForward = LightDirection.GetSafeNormal();
	if (LightForward.SizeSquared() < 0.0001f)
	{
		LightForward = FVector(0.0f, 0.0f, -1.0f);
	}

	const float Distance = (FarClip - NearClip) * 0.5f;
	LightPosition = FocusPosition - LightForward * Distance;

	FVector WorldUp = (std::abs(LightForward.Z) > 0.95f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, 0.0f, 1.0f);
	FVector Right = (WorldUp ^ LightForward).GetSafeNormal();
	FVector Up = (LightForward ^ Right).GetSafeNormal();

	FMatrix RotMatrix;
	RotMatrix.SetIdentity();
	RotMatrix.M[0][0] = LightForward.X; RotMatrix.M[0][1] = LightForward.Y; RotMatrix.M[0][2] = LightForward.Z;
	RotMatrix.M[1][0] = Right.X;        RotMatrix.M[1][1] = Right.Y;        RotMatrix.M[1][2] = Right.Z;
	RotMatrix.M[2][0] = Up.X;           RotMatrix.M[2][1] = Up.Y;           RotMatrix.M[2][2] = Up.Z;

	FMatrix InvTranslation = FMatrix::MakeTranslation(LightPosition * -1.0f);
	FMatrix InvRotation = RotMatrix.GetTransposed();
	LightViewMatrix = InvTranslation * InvRotation;

	// CameraComponent::GetOrthogonalMatrix와 동일한 직교 투영 행렬 구성
	const float Width = OrthoWidth;
	const float Height = OrthoHeight;
	const float XScale = 2.0f / Width;
	const float YScale = 2.0f / Height;
	const float ZScale = 1.0f / (FarClip - NearClip);
	const float ZOffset = -NearClip / (FarClip - NearClip);

	LightProjMatrix = FMatrix(
		0.0f, 0.0f, ZScale, 0.0f,
		XScale, 0.0f, 0.0f, 0.0f,
		0.0f, YScale, 0.0f, 0.0f,
		0.0f, 0.0f, ZOffset, 1.0f
	);

	LightViewProjection = LightViewMatrix * LightProjMatrix;
}

void FShadowRenderer::RenderShadowMap(
	const FProjectedShadowInfo& ShadowInfo,
	const FRenderQueue& RenderQueue,
	const TArray<const FStaticDrawGroup*>& StaticGroups)
{
	if (!ShadowInfo.bCastShadow || !ShadowInfo.TargetShadowMap)
	{
		return;
	}

	FViewConstants ViewConstants;
	ViewConstants.ViewProjection = ShadowInfo.LightViewProjection;
	ViewConstants.CameraPosition = ShadowInfo.LightPosition;
	RenderCommand::UpdateBufferData(ShadowViewCB.get(), &ViewConstants);

	RenderCommand::BindPipelineState(PipelineState);
	RenderCommand::BindConstantBuffer(0, ShadowViewCB.get(), EShaderBindFlagBits::Vertex);
	RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex);

	// 불투명 스태틱 물체
	for (const FStaticDrawGroup* Group : StaticGroups)
	{
		if (!Group || !Group->Mesh) continue;

		RenderCommand::BindMesh(Group->Mesh, Group->LODIndex);

		for (const FStaticDrawItem& Item : Group->Items)
		{
			if (!Item.Proxy) continue;

			FPerObjectConstants ObjectConstants;
			ObjectConstants.World = Item.Proxy->GetLocalToWorld();
			ObjectConstants.NormalMatrix = Item.Proxy->GetNormalMatrix();
			RenderCommand::UpdateBufferData(PerObjectCB.get(), &ObjectConstants);

			RenderCommand::DrawIndexed(Item.IndexCount, Item.StartIndex);
		}
	}

	// 동적/특수/반투명 물체
	for (const FRenderPacket& Packet : RenderQueue)
	{
		if (!Packet.Mesh) continue;

		// 반투명 머티리얼은 그림자X
		if (Packet.Material && Packet.Material->BlendState != EBlendState::Opaque) continue;

		RenderCommand::BindMesh(Packet.Mesh, Packet.LODIndex);

		FPerObjectConstants ObjectConstants;
		if (Packet.Proxy)
		{
			ObjectConstants.World = Packet.Proxy->GetLocalToWorld();
			ObjectConstants.NormalMatrix = Packet.Proxy->GetNormalMatrix();
		}
		else
		{
			ObjectConstants.World = Packet.Model ? *Packet.Model : FMatrix::Identity;
			ObjectConstants.NormalMatrix = ObjectConstants.World.Inverse().GetTransposed();
		}
		RenderCommand::UpdateBufferData(PerObjectCB.get(), &ObjectConstants);

		uint32 IndexCount = Packet.IndexCount ? Packet.IndexCount : Packet.Mesh->GetIndexBuffer(Packet.LODIndex)->GetIndexCount();
		RenderCommand::DrawIndexed(IndexCount, Packet.StartIndex);
	}

	RenderCommand::SetRasterizerState(ERasterizerState::SolidBack);
}

void FShadowRenderer::BindForShading(const FProjectedShadowInfo& ShadowInfo)
{
	if (!ShadowInfo.TargetShadowMap || !ShadowInfo.ConstantBuffer) return;

	if (ShadowInfo.ProjectionType == EShadowProjectionType::Directional)
	{
		FDirectionalShadowConstants Constants;		
		Constants.DirectionalLightViewProjection = ShadowInfo.LightViewProjection;
		Constants.DirectionalShadowBias = ShadowInfo.ShadowBias;
		Constants.bCastDirectionalShadow = ShadowInfo.bCastShadow ? 1 : 0;
		Constants.DirectionalShadowTexelSize = ShadowInfo.ShadowTexelSize;
		Constants.DirectionalNormalBias = ShadowInfo.ShadowNormalBias;
		RenderCommand::UpdateBufferData(ShadowInfo.ConstantBuffer, &Constants);
		RenderCommand::BindConstantBuffer(ShadowInfo.ConstantBufferSlot, ShadowInfo.ConstantBuffer, EShaderBindFlagBits::Pixel);
		RenderCommand::BindShaderResource(ShadowInfo.ShaderResourceSlot, ShadowInfo.TargetShadowMap, EShaderBindFlagBits::Pixel);
		RenderCommand::BindSamplerState(1, ESamplerState::ShadowMap, EShaderBindFlagBits::Pixel);
	}
	else if (ShadowInfo.ProjectionType == EShadowProjectionType::Spot)
	{
		FSpotShadowConstants Constants;
		Constants.SpotLightViewProjection = ShadowInfo.LightViewProjection;
		Constants.SpotShadowBias = ShadowInfo.ShadowBias;
		Constants.ShadowSpotLightIndex = ShadowInfo.bCastShadow ? ShadowInfo.LightIndex : -1;
		Constants.SpotShadowTexelSize = ShadowInfo.ShadowTexelSize;
		Constants.SpotNormalBias = ShadowInfo.ShadowNormalBias;
		RenderCommand::UpdateBufferData(ShadowInfo.ConstantBuffer, &Constants);
		RenderCommand::BindConstantBuffer(ShadowInfo.ConstantBufferSlot, ShadowInfo.ConstantBuffer, EShaderBindFlagBits::Pixel);
		RenderCommand::BindShaderResource(ShadowInfo.ShaderResourceSlot, ShadowInfo.TargetShadowMap, EShaderBindFlagBits::Pixel);
		RenderCommand::BindSamplerState(1, ESamplerState::ShadowMap, EShaderBindFlagBits::Pixel);
	}
}

void FShadowRenderer::UnbindForShading(const FProjectedShadowInfo& ShadowInfo)
{
	RenderCommand::UnbindShaderResource(ShadowInfo.ShaderResourceSlot, EShaderBindFlagBits::Pixel);
}

void FShadowRenderer::DisableDirectionalShadow()
{
	if (!DirectionalShadowConstantsCB) return;

	FDirectionalShadowConstants Constants;
	Constants.bCastDirectionalShadow = 0;
	RenderCommand::UpdateBufferData(DirectionalShadowConstantsCB.get(), &Constants);
	RenderCommand::BindConstantBuffer(5, DirectionalShadowConstantsCB.get(), EShaderBindFlagBits::Pixel);
}

void FShadowRenderer::DisableSpotShadow()
{
	if (!SpotShadowConstantsCB) return;

	FSpotShadowConstants Constants;
	Constants.ShadowSpotLightIndex = -1;
	RenderCommand::UpdateBufferData(SpotShadowConstantsCB.get(), &Constants);
	RenderCommand::BindConstantBuffer(6, SpotShadowConstantsCB.get(), EShaderBindFlagBits::Pixel);
}

FProjectedShadowInfo FShadowRenderer::SetupDirectionalShadow(const FDirectionalLightSceneProxy& DirProxy, const FVector& FocusPosition)
{
	FProjectedShadowInfo Info;
	Info.ProjectionType = EShadowProjectionType::Directional;

	const FDirectionalLightInfo& DirectionalLight = DirProxy.GetInfo();
	FVector LightForward = DirectionalLight.Direction.GetSafeNormal();
	if (LightForward.SizeSquared() < 0.0001f)
	{
		LightForward = FVector(0.0f, 0.0f, -1.0f);
	}
	const float Distance = (FarClip - NearClip) * 0.5f;
	Info.LightPosition = FocusPosition - LightForward * Distance;

	FVector WorldUp = (std::abs(LightForward.Z) > 0.95f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, 0.0f, 1.0f);
	FVector Right = (WorldUp ^ LightForward).GetSafeNormal();
	FVector Up = (LightForward ^ Right).GetSafeNormal();

	FMatrix RotMatrix;
	RotMatrix.SetIdentity();
	RotMatrix.M[0][0] = LightForward.X; RotMatrix.M[0][1] = LightForward.Y; RotMatrix.M[0][2] = LightForward.Z;
	RotMatrix.M[1][0] = Right.X;        RotMatrix.M[1][1] = Right.Y;        RotMatrix.M[1][2] = Right.Z;
	RotMatrix.M[2][0] = Up.X;           RotMatrix.M[2][1] = Up.Y;           RotMatrix.M[2][2] = Up.Z;

	FMatrix InvTranslation = FMatrix::MakeTranslation(Info.LightPosition * -1.0f);
	FMatrix InvRotation = RotMatrix.GetTransposed();
	Info.LightViewMatrix = InvTranslation * InvRotation;

	const float Width = DirProxy.GetShadowOrthoSize();
	const float Height = DirProxy.GetShadowOrthoSize();
	const float XScale = 2.0f / Width;
	const float YScale = 2.0f / Height;
	const float ZScale = 1.0f / (FarClip - NearClip);
	const float ZOffset = -NearClip / (FarClip - NearClip);

	Info.LightProjMatrix = FMatrix(
		0.0f, 0.0f, ZScale, 0.0f,
		XScale, 0.0f, 0.0f, 0.0f,
		0.0f, YScale, 0.0f, 0.0f,
		0.0f, 0.0f, ZOffset, 1.0f
	);

	Info.LightViewProjection = Info.LightViewMatrix * Info.LightProjMatrix;

	Info.ShadowBias = DirProxy.GetShadowBias();
	Info.ShadowNormalBias = DirProxy.GetShadowNormalBias();
	Info.ShadowTexelSize = (Resolution > 0) ? (Width / static_cast<float>(Resolution)) : 1.0f;
	Info.bCastShadow = DirProxy.GetCastShadows();

	Info.TargetShadowMap = DirectionalShadowMap.get();
	Info.ShadowPassInfo = DirectionalShadowPassInfo;

	Info.ShaderResourceSlot = 2;
	Info.ConstantBufferSlot = 5;
	Info.ConstantBuffer = DirectionalShadowConstantsCB.get();

	LightPosition = Info.LightPosition;
	LightViewMatrix = Info.LightViewMatrix;
	LightProjMatrix = Info.LightProjMatrix;
	LightViewProjection = Info.LightViewProjection;
	OrthoWidth = Width;
	OrthoHeight = Height;
	ShadowBias = Info.ShadowBias;
	ShadowNormalBias = Info.ShadowNormalBias;

	return Info;
}

FProjectedShadowInfo FShadowRenderer::SetupSpotShadow(const FSpotLightSceneProxy& SpotProxy, int32 InLightIndex)
{
	FProjectedShadowInfo Info;
	Info.ProjectionType = EShadowProjectionType::Spot;

	const FSpotLightInfo& SpotLight = SpotProxy.GetInfo();
	FVector LightForward = SpotLight.Direction.GetSafeNormal();
	if (LightForward.SizeSquared() < 0.0001f)
	{
		LightForward = FVector(0.0f, 0.0f, -1.0f);
	}
	Info.LightPosition = SpotLight.Position;

	FVector WorldUp = (std::abs(LightForward.Z) > 0.95f) ? FVector(0.0f, 1.0f, 0.0f) : FVector(0.0f, 0.0f, 1.0f);
	FVector Right = (WorldUp ^ LightForward).GetSafeNormal();
	FVector Up = (LightForward ^ Right).GetSafeNormal();

	FMatrix RotMatrix;
	RotMatrix.SetIdentity();
	RotMatrix.M[0][0] = LightForward.X; RotMatrix.M[0][1] = LightForward.Y; RotMatrix.M[0][2] = LightForward.Z;
	RotMatrix.M[1][0] = Right.X;        RotMatrix.M[1][1] = Right.Y;        RotMatrix.M[1][2] = Right.Z;
	RotMatrix.M[2][0] = Up.X;           RotMatrix.M[2][1] = Up.Y;           RotMatrix.M[2][2] = Up.Z;

	FMatrix InvTranslation = FMatrix::MakeTranslation(Info.LightPosition * -1.0f);
	FMatrix InvRotation = RotMatrix.GetTransposed();
	Info.LightViewMatrix = InvTranslation * InvRotation;

	const float HalfFOV = FMath::DegreesToRadians(SpotProxy.GetOuterConeAngle());
	const float YScale = 1.0f / std::tan(HalfFOV);
	const float XScale = YScale; // 1.0 Aspect ratio for square shadow map

	const float NearZ = 0.1f;
	const float FarZ = FMath::Max(SpotProxy.GetAttenuationRadius(), NearZ + 10.0f);
	const float ZScale = FarZ / (FarZ - NearZ);
	const float ZOffset = -NearZ * FarZ / (FarZ - NearZ);

	Info.LightProjMatrix = FMatrix(
		0.0f, 0.0f, ZScale, 1.0f,
		XScale, 0.0f, 0.0f, 0.0f,
		0.0f, YScale, 0.0f, 0.0f,
		0.0f, 0.0f, ZOffset, 0.0f
	);

	Info.LightViewProjection = Info.LightViewMatrix * Info.LightProjMatrix;

	Info.ShadowBias = SpotProxy.GetShadowBias();
	Info.ShadowNormalBias = SpotProxy.GetShadowNormalBias();
	const float ConeWidthAtRadius = 2.0f * std::tan(HalfFOV) * SpotProxy.GetAttenuationRadius();
	Info.ShadowTexelSize = (Resolution > 0) ? (ConeWidthAtRadius / static_cast<float>(Resolution)) : 1.0f;
	Info.bCastShadow = SpotProxy.GetCastShadows();

	Info.TargetShadowMap = SpotShadowMap.get();
	Info.ShadowPassInfo = SpotShadowPassInfo;

	Info.ShaderResourceSlot = 3;
	Info.ConstantBufferSlot = 6;
	Info.ConstantBuffer = SpotShadowConstantsCB.get();
	Info.LightIndex = InLightIndex;

	return Info;
}
