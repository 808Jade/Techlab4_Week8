#pragma once

#include "Buffer.h"
#include "PipelineState.h"
#include "Texture2D.h"
#include "RenderingInfo.h"
#include "RenderPacket.h"
#include "LightInfo.h"
#include "Renderer.h"

class FShadowRenderer
{
public:
	FShadowRenderer() = default;
	~FShadowRenderer() = default;

	FShadowRenderer(const FShadowRenderer&) = delete;
	FShadowRenderer& operator=(const FShadowRenderer&) = delete;

	bool Init(uint32 InResolution = 2048);
	void OnRender(const FDirectionalLightInfo& DirectionalLight, const FVector& FocusPosition,
		const FRenderQueue& RenderQueue, const std::vector<const FStaticDrawGroup*>& StaticGroups);

	FTexture2D* GetShadowMap() const { return ShadowMap.get(); }
	const FMatrix& GetLightViewProjection() const { return LightViewProjection; }
	const FMatrix& GetLightView() const { return LightViewMatrix; }
	const FMatrix& GetLightProj() const { return LightProjMatrix; }
	const FVector& GetLightPosition() const { return LightPosition; }

	float GetShadowBias() const { return ShadowBias; }
	void SetShadowBias(float InBias) { ShadowBias = InBias; }

	float GetOrthoWidth() const { return OrthoWidth; }
	void SetOrthoWidth(float InWidth) { OrthoWidth = InWidth; }

	float GetOrthoHeight() const { return OrthoHeight; }
	void SetOrthoHeight(float InHeight) { OrthoHeight = InHeight; }

	uint32 GetResolution() const { return Resolution; }

private:
	void UpdateLightMatrix(const FVector& LightDirection, const FVector& FocusPosition);

	uint32 Resolution = 2048;

	// @todo : Light로부터 가져오도록 변경해야함
	float OrthoWidth = 4000.0f;
	float OrthoHeight = 4000.0f;
	float NearClip = 10.0f;
	float FarClip = 10000.0f;

	float ShadowBias = 0.001f;

	FVector LightPosition = FVector::ZeroVector;
	FMatrix LightViewMatrix = FMatrix::Identity;
	FMatrix LightProjMatrix = FMatrix::Identity;
	FMatrix LightViewProjection = FMatrix::Identity;

	TUniquePtr<FTexture2D> ShadowMap;
	FRenderingInfo ShadowPassInfo;

	FShaderProgram* ShadowShader = nullptr;
	TUniquePtr<FConstantBuffer> ShadowViewCB;
	TUniquePtr<FConstantBuffer> PerObjectCB;
};
