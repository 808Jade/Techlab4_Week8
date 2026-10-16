#include "EnginePCH.h"
#include "ShadowRenderer.h"
#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Engine/PrimitiveSceneProxy.h"

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

	ShadowMap = RenderCommand::CreateTexture2D(Desc);
	if (!ShadowMap || !ShadowMap->GetDSV() || !ShadowMap->GetSRV())
	{
		HTR_LOG(Error, "[ShadowRenderer] Failed to create shadow map texture (Resolution: {})", Resolution);
		return false;
	}

	ShadowPassInfo.ViewportSetting.StartX = 0;
	ShadowPassInfo.ViewportSetting.StartY = 0;
	ShadowPassInfo.ViewportSetting.Width = Resolution;
	ShadowPassInfo.ViewportSetting.Height = Resolution;
	ShadowPassInfo.ViewportSetting.MinDepth = 0.0f;
	ShadowPassInfo.ViewportSetting.MaxDepth = 1.0f;

	ShadowPassInfo.ColorRenderTargets.Empty();
	ShadowPassInfo.DepthStencil.Texture = ShadowMap.get();
	ShadowPassInfo.DepthStencil.LoadOp = ERenderTargetLoadOp::Clear;
	ShadowPassInfo.DepthStencil.StoreOp = ERenderTargetStoreOp::Store;
	ShadowPassInfo.DepthStencil.ClearValue.DepthClearValue = 1.0f;
	ShadowPassInfo.DepthStencil.ClearValue.StencilClearValue = 0;

	ShadowShader = FRenderResourceManager::GetShaderProgram("Resources/Shader/ShadowDepthShader.hlsl");
	if (!ShadowShader)
	{
		HTR_LOG(Error, "[ShadowRenderer] ShadowDepthShader.hlsl not found");
		return false;
	}

	ShadowViewCB = RenderCommand::CreateConstantBuffer(sizeof(FViewConstants));
	PerObjectCB = RenderCommand::CreateConstantBuffer(sizeof(FPerObjectConstants));

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

void FShadowRenderer::OnRender(
	const FDirectionalLightInfo& DirectionalLight,
	const FVector& FocusPosition,
	const FRenderQueue& RenderQueue,
	const std::vector<const FStaticDrawGroup*>& StaticGroups)
{
	if (!ShadowMap || !ShadowShader)
		return;

	if (DirectionalLight.Direction.SizeSquared() < 0.0001f)
		return;

	UpdateLightMatrix(DirectionalLight.Direction, FocusPosition);

	FViewConstants ViewConstants;
	ViewConstants.ViewProjection = LightViewProjection;
	ViewConstants.CameraPosition = LightPosition;
	RenderCommand::UpdateBufferData(ShadowViewCB.get(), &ViewConstants);

	RenderCommand::BeginRenderPass(ShadowPassInfo);

	RenderCommand::BindShaderProgram(ShadowShader);
	RenderCommand::SetRasterizerState(ERasterizerState::SolidFront); // 뒷면만 깊이 맵에 그려 Shadow Acne 방지
	RenderCommand::SetBlendState(EBlendState::NoColorWrite);
	RenderCommand::SetDepthStencilState(EDepthStencilState::Default);
	RenderCommand::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

	RenderCommand::EndRenderPass(ShadowPassInfo);

	RenderCommand::SetRasterizerState(ERasterizerState::SolidBack);
}
