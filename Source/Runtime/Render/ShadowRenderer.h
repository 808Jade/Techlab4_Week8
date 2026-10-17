#pragma once
#include "LightInfo.h"
#include "PipelineState.h"
#include "Render/Buffer.h"
#include "RenderingInfo.h"

class FTexture2D;
class FRenderer;

class FShadowRenderer
{
public:
	FShadowRenderer() = default;
	~FShadowRenderer() = default;

	FShadowRenderer(const FShadowRenderer&) = delete;
	FShadowRenderer operator= (const FShadowRenderer&) = delete;

	bool Init(uint32 InResolution = 2048);

	FMatrix BuildDirectionalShadowVP(
		const FVector& CameraPosition, const FVector& CameraForward, const FVector& LightDirection,
		float ShadowDistance, float CameraNear, float CameraFar) const;

	void RenderDepth(FRenderer& Renderer, const FMatrix& ShadowVP);

	void BindShading();
	void UnbindShading();

	FTexture2D* GetShadowMap() const { return ShadowMap.get(); }

private: 
	static constexpr uint32 ShadowMapSlot = 12;
	static constexpr uint32 ShadowSamplerSlot = 1;

	uint32 Resolution = 2048;
	TUniquePtr<FTexture2D> ShadowMap;
	FRenderingInfo PassInfo;

	FShaderProgram* ShadowMapShader = nullptr;
	FPipelineState PipelineState;
};
